// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewController.h"

#include <utility>

#include "../../logging.h"
#include "../../util.h"
#include "../notification.h"
#include "entityController.h"
#include "webview.h"
#include "webviewButtons.h"

#ifdef UC_WEBVIEW
#include "../../webview/webviewlibrary.h"
#endif

namespace uc {
namespace ui {

WebViewController *WebViewController::s_instance = nullptr;

WebViewController::WebViewController(core::Api *core, EntityController *entityController, const QString &version,
                                     hw::HardwareModel::Enum model, QObject *parent)
    : QObject(parent),
      m_core(core),
      m_entityController(entityController),
      m_version(version),
      m_buttonIds(entity::WebviewButtons::ofModel(model)) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
    // the defaults until the core answers
    m_buttons = entity::WebviewButtons::withDefaults(m_buttonIds, QVariantMap());

#ifdef UC_WEBVIEW
    m_installed = [] { return webview::WebViewLibrary::instance()->isInstalled(); };
    m_available = [] { return webview::WebViewLibrary::instance()->isAvailable(); };
    m_clearWebsiteData = [this](bool all, std::function<void(bool)> done) {
        using Library = webview::WebViewLibrary;
        switch (Library::instance()->clearWebsiteData(all, this, std::move(done))) {
            case Library::ClearStart::Started:
                return ClearStart::Started;
            case Library::ClearStart::Unsupported:
                return ClearStart::Unsupported;
            case Library::ClearStart::Refused:
                break;
        }
        return ClearStart::Refused;
    };
    m_globalSettings = [](const QString &colorScheme, bool allowInvalidCertificates) {
        webview::WebViewLibrary::instance()->setGlobalSettings(colorScheme, allowInvalidCertificates);
    };
    m_clearSupported = [] { return webview::WebViewLibrary::instance()->canClearWebsiteData(); };
    // a page or the Settings menu may load the library first
    webview::WebViewLibrary::instance()->setAvailabilityHandler([this](bool usable) { libraryLoaded(usable); });
#else
    // built without the web view (CONFIG+=no-webview): the UI executes no web view command
    m_installed = [] { return false; };
    m_available = [] { return false; };
    m_clearWebsiteData = [](bool, std::function<void(bool)>) { return ClearStart::Unsupported; };
    m_globalSettings = [](const QString &, bool) {};
    m_clearSupported = [] { return false; };
#endif

    QObject::connect(m_core, &core::Api::connected, this, &WebViewController::onCoreConnected);
    QObject::connect(m_core, &core::Api::uiEntityCommand, this, &WebViewController::onUiEntityCommand);
    QObject::connect(m_core, &core::Api::cfgWebviewChanged, this, &WebViewController::applyCfg);
}

WebViewController::~WebViewController() {
#ifdef UC_WEBVIEW
    webview::WebViewLibrary::instance()->setAvailabilityHandler(nullptr);
#endif
    s_instance = nullptr;
}

QObject *WebViewController::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine)

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

void WebViewController::setFullscreenEntityId(const QString &entityId) {
    if (m_fullscreenEntityId == entityId) {
        return;
    }
    m_fullscreenEntityId = entityId;
    emit fullscreenEntityIdChanged();
}

bool WebViewController::available() {
    const bool usable = m_available && m_available();
    libraryLoaded(usable);
    return usable;
}

void WebViewController::onCoreConnected() {
    // a new session: the core knows nothing of this UI yet
    sendUiInfo();

    m_cfgRequestId = m_core->getWebviewCfg();
    const int id = m_cfgRequestId;
    m_core->onResponseWithErrorResult(
        id, &core::Api::respWebviewCfg,
        [this, id](core::cfgWebview cfg) {
            if (id == m_cfgRequestId) {
                applyCfg(cfg);
            }
        },
        [](int code, QString message) {
            qCWarning(lcWebViewEntity()) << "Cannot get the web view settings:" << code << message;
        });
}

QStringList WebViewController::entityTypesToDeclare() const {
    const bool  webviews = m_libraryState == LibraryState::NotLoaded ? m_installed && m_installed()
                                                                     : m_libraryState == LibraryState::Loaded;
    QStringList types;
    if (webviews) {
        types.append(QStringLiteral("webview"));
    }
    return types;
}

void WebViewController::libraryLoaded(bool usable) {
    const LibraryState state = usable ? LibraryState::Loaded : LibraryState::Failed;
    if (m_libraryState == state) {
        return;
    }
    m_libraryState = state;
    if (declaresWebviews() == usable) {
        return;
    }
    if (!usable) {
        qCWarning(lcWebViewEntity()) << "The web view cannot show pages: web views are no longer declared";
    }
    sendUiInfo();
}

void WebViewController::sendUiInfo() {
    const QStringList types = entityTypesToDeclare();
    const int         id = m_core->setUiInfo(m_version, types);
    if (id < 0) {
        // not connected: the next authentication sends it
        return;
    }
    m_declaredTypes = types;
    qCInfo(lcWebViewEntity()) << "Declared to the core, entity types executed by the UI:" << types;

    m_core->onResult(
        id, [] {},
        [](int code, QString message) {
            qCWarning(lcWebViewEntity()) << "The core refused the UI declaration:" << code << message;
        });
}

void WebViewController::onUiEntityCommand(const core::UiEntityCommand &command) {
    execute(command, [this, command](int code, const QString &message) {
        if (code == 200) {
            qCInfo(lcWebViewEntity()) << "Command" << command.cmdId << "for" << command.entityId;
        } else {
            qCWarning(lcWebViewEntity()) << "Command" << command.cmdId << "for" << command.entityId
                                         << "refused:" << code << message;
        }
        m_core->respondResult(command.reqId, code, message);
    });
}

void WebViewController::execute(const core::UiEntityCommand &command, const Answer &answer) {
    if (command.entityType != QLatin1String("webview")) {
        answer(
            400,
            QStringLiteral("The user interface does not execute commands of entity type %1").arg(command.entityType));
        return;
    }

    bool       converted = false;
    const auto cmd =
        Util::convertStringToEnum<entity::WebviewCommands::Enum>(Util::FirstToUpper(command.cmdId), &converted);
    if (!converted) {
        answer(400, QStringLiteral("Unknown web view command %1").arg(command.cmdId));
        return;
    }

    auto webview = qobject_cast<entity::Webview *>(m_entityController->get(command.entityId));
    if (!webview) {
        answer(404, QStringLiteral("Unknown web view %1").arg(command.entityId));
        return;
    }

    // main.qml opens no web view over the onboarding screens; checked first, the library need not load for it
    if (cmd == entity::WebviewCommands::Open && m_onboarding) {
        answer(503, QStringLiteral("The user interface can't show a web view during the onboarding of the Remote"));
        return;
    }

    if (!available()) {
        answer(503, QStringLiteral("This user interface cannot show web pages"));
        return;
    }

    switch (cmd) {
        case entity::WebviewCommands::Open:
            emit openRequested(command.entityId);
            answer(200, QString());
            return;
        case entity::WebviewCommands::Reload:
            // no effect when the web view is not shown
            emit reloadRequested(command.entityId);
            answer(200, QString());
            return;
        case entity::WebviewCommands::Clear_cache:
            clearWebsiteData(false, answer);
            return;
        case entity::WebviewCommands::Clear_data:
            clearWebsiteData(true, answer);
            return;
    }
    answer(400, QStringLiteral("Unknown web view command %1").arg(command.cmdId));
}

void WebViewController::clearWebsiteData(bool all, const Answer &answer) {
    const QString what = all ? QStringLiteral("the website data") : QStringLiteral("the cache");
    // The answer waits for the library (an idle of the GLib main context, then Qt's event loop): the clear itself
    // takes WebKit's network process a moment. The core answers its caller 408 if that took longer than 5 s.
    const ClearStart start = m_clearWebsiteData(all, [this, answer, what](bool ok) {
        if (!ok) {
            answer(500, QStringLiteral("Clearing %1 failed").arg(what));
            return;
        }
        // every web view shares the website data: the shown pages load again with what is left
        emit reloadRequested(QString());
        answer(200, QString());
    });
    switch (start) {
        case ClearStart::Started:
            return;
        case ClearStart::Unsupported:
            answer(501, QStringLiteral("Clearing %1 is not supported by this firmware").arg(what));
            return;
        case ClearStart::Refused:
            break;
    }
    answer(500, QStringLiteral("Clearing %1 was refused by the web view").arg(what));
}

void WebViewController::applyCfg(const core::cfgWebview &cfg) {
    m_cfg = cfg;
    // a button the core does not report, or with an action the UI does not know, has its default action (ADR 0018)
    m_buttons = entity::WebviewButtons::withDefaults(m_buttonIds, cfg.buttons);
    QStringList changedButtons;
    for (const QString &button : m_buttonIds) {
        if (m_buttons.value(button).toMap() != entity::WebviewButtons::defaultAction(button)) {
            changedButtons.append(button);
        }
    }
    qCInfo(lcWebViewEntity()) << "Web view settings: colour scheme" << cfg.colorScheme << "website data"
                              << cfg.websiteData << "invalid certificates"
                              << (cfg.allowInvalidCertificates ? "allowed" : "refused") << "buttons not on default"
                              << changedButtons;
    m_globalSettings(cfg.colorScheme, cfg.allowInvalidCertificates);
    emit webviewCfgChanged();
}

QStringList WebViewController::buttonIds() const {
    return m_buttonIds;
}

QStringList WebViewController::buttonFunctions() const {
    return entity::WebviewButtons::functions();
}

QStringList WebViewController::buttonKeys() const {
    return entity::WebviewButtons::keys();
}

QVariantMap WebViewController::defaultButtonAction(const QString &button) const {
    return entity::WebviewButtons::defaultAction(button);
}

bool WebViewController::buttonRepeats(const QVariantMap &action) const {
    return entity::WebviewButtons::repeats(action);
}

void WebViewController::setColorScheme(const QString &colorScheme) {
    if (colorScheme != QLatin1String("light") && colorScheme != QLatin1String("dark")) {
        qCWarning(lcWebViewEntity()) << "Ignoring the unknown web view colour scheme" << colorScheme;
        return;
    }
    if (colorScheme != m_cfg.colorScheme) {
        changeCfg({{QStringLiteral("color_scheme"), colorScheme}});
    }
}

void WebViewController::setAllowInvalidCertificates(bool allow) {
    if (allow != m_cfg.allowInvalidCertificates) {
        changeCfg({{QStringLiteral("allow_invalid_certificates"), allow}});
    }
}

void WebViewController::setWebsiteData(const QString &websiteData) {
    if (websiteData != QLatin1String("keep") && websiteData != QLatin1String("clear_cache_on_close") &&
        websiteData != QLatin1String("clear_all_on_close")) {
        qCWarning(lcWebViewEntity()) << "Ignoring the unknown web view website data setting" << websiteData;
        return;
    }
    if (websiteData != m_cfg.websiteData) {
        changeCfg({{QStringLiteral("website_data"), websiteData}});
    }
}

void WebViewController::setButtonAction(const QString &button, const QVariantMap &action) {
    const QVariantMap valid = entity::WebviewButtons::validAction(action);
    if (!m_buttonIds.contains(button) || valid.isEmpty()) {
        qCWarning(lcWebViewEntity()) << "Ignoring the web view button setting of" << button << "with the action"
                                     << action.value(QStringLiteral("action")).toString()
                                     << action.value(QStringLiteral("function")).toString()
                                     << action.value(QStringLiteral("key")).toString();
        return;
    }
    if (valid != m_buttons.value(button).toMap()) {
        changeCfg({{QStringLiteral("buttons"), QVariantMap{{button, valid}}}});
    }
}

void WebViewController::changeCfg(const QVariantMap &update) {
    const int id = m_core->setWebviewCfg(update);
    if (id < 0) {
        qCWarning(lcWebViewEntity()) << "Cannot change the web view settings: not connected to the core";
        // the control that asked shows the setting as it is
        emit webviewCfgChanged();
        return;
    }
    m_cfgRequestId = id;
    m_core->onResponseWithErrorResult(
        id, &core::Api::respWebviewCfg,
        [this, id](core::cfgWebview cfg) {
            if (id == m_cfgRequestId) {
                applyCfg(cfg);
            }
        },
        [this](int code, QString message) {
            qCWarning(lcWebViewEntity()) << "Cannot change the web view settings:" << code << message;
            //: Notification: a setting of the web pages (Settings > Web view) could not be changed. %1 is the core's
            //: error message
            Notification::createNotification(tr("Error setting web view settings: %1").arg(message), true);
            // the control that asked shows the setting as it is
            emit webviewCfgChanged();
        });
}

bool WebViewController::canClearWebsiteData() {
    return m_clearSupported && m_clearSupported();
}

void WebViewController::clearData(bool all) {
    clearWebsiteData(all, [this, all](int code, const QString &message) {
        ClearResult result = Failed;
        if (code == 200) {
            result = Cleared;
            qCInfo(lcWebViewEntity()) << "Cleared" << (all ? "the website data" : "the cache") << "from the settings";
        } else {
            result = code == 501 ? Unsupported : Failed;
            qCWarning(lcWebViewEntity()) << "Clearing from the settings:" << code << message;
        }
        emit dataCleared(all, result);
    });
}

void WebViewController::viewClosed() {
    const bool cache = m_cfg.websiteData == QLatin1String("clear_cache_on_close");
    const bool all = m_cfg.websiteData == QLatin1String("clear_all_on_close");
    if (!cache && !all) {
        return;
    }
    m_clearWebsiteData(all, [all](bool ok) {
        if (!ok) {
            qCWarning(lcWebViewEntity()) << "Clearing" << (all ? "the website data" : "the cache")
                                         << "after a web view closed failed";
        }
    });
}

}  // namespace ui
}  // namespace uc
