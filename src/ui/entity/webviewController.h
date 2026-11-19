// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>

#include <functional>
#include <utility>

#include "../../core/core.h"
#include "../../hardware/hardwareModel.h"

namespace uc {
namespace ui {

class EntityController;

/**
 * The UI side of the web view entity that is not a single page (core-api doc/entities/entity_webview.md): the UI
 * executes the commands of web views, the core only routes them to it.
 *
 * - Declaration: after every authentication the UI sends set_ui_info with its version. It names `webview` as an entity
 *   type it executes when the firmware's web view library is installed (WebViewLibrary::isInstalled(), which does not
 *   load it), before any web view entity is known. The library loads at its first use: a command, a page, the Settings
 *   menu. From then on the outcome of that load decides: a library that fails to load or to initialise withdraws the
 *   declaration (set_ui_info without `webview`), one that loads although the check missed it declares. The core
 *   reports web views as ON only while a UI declared them.
 * - Commands: the core's entity_command requests for web views are answered when the command is accepted, not when a
 *   page has loaded: 200, 400 for an unknown command or entity type, 404 for an unknown web view, 503 without a web
 *   view in this UI. `open` and `reload` are passed to QML (openRequested(), reloadRequested()) and answered at once.
 *   `clear_cache` and `clear_data` are answered when the library reports the end of the clear (library 0.8.0): 200
 *   and the shown pages reload, or 500 when it failed or was refused; 501 only when the library cannot clear website
 *   data (older than 0.8.0).
 * - Global settings: get_webview_cfg after authentication and every `webview` configuration change; the colour scheme
 *   and the certificate setting go to the web view library, the website data setting is applied when a web view
 *   closes (viewClosed()), the buttons are the actions of the remote's buttons in the fullscreen web view. The
 *   Settings page shows them and changes them with set_webview_cfg (admin scope, the UI's session has it): a write
 *   sends only that setting, or that button, and the property changes when the core answers, like the settings of
 *   Config. A refused change is told with a notification, and the property reports its value again so that the
 *   control shows the setting as it is.
 * - Clearing from the Settings page: clearData() runs the clear of the `clear_cache` and `clear_data` commands and
 *   reports its end with dataCleared().
 * - The fullscreen web view: QML records which web view it shows (fullscreenEntityId); while it is open the web view
 *   widgets unload their page, the UI shows one page at a time.
 */
class WebViewController : public QObject {
    Q_OBJECT

    // the web view the fullscreen screen shows, empty when it is closed; written by that screen
    Q_PROPERTY(
        QString fullscreenEntityId READ fullscreenEntityId WRITE setFullscreenEntityId NOTIFY fullscreenEntityIdChanged)

    // The global web view settings of the core (core-api entity_webview.md "Global web view settings"). A write asks
    // the core to change the setting, the value follows its answer. The colour scheme is "light" or "dark".
    Q_PROPERTY(QString colorScheme READ colorScheme WRITE setColorScheme NOTIFY webviewCfgChanged)
    Q_PROPERTY(bool allowInvalidCertificates READ allowInvalidCertificates WRITE setAllowInvalidCertificates NOTIFY
                   webviewCfgChanged)
    // "keep", "clear_cache_on_close" or "clear_all_on_close"
    Q_PROPERTY(QString websiteData READ websiteData WRITE setWebsiteData NOTIFY webviewCfgChanged)
    // The buttons of the fullscreen web view (core-api "Button settings", entity::WebviewButtons): the buttons of this
    // remote model that a mapping changes, in the order of the settings page, and every one of them with its action,
    // the core's or the default (button identifier -> {"action": ..., "function"/"key": ...}). A web view's own buttons
    // (entity::Webview::buttons) go before these. setButtonAction() changes one.
    Q_PROPERTY(QStringList buttonIds READ buttonIds CONSTANT)
    Q_PROPERTY(QVariantMap buttons READ buttons NOTIFY webviewCfgChanged)
    // the functions and the keys a button can have, in the order of the Core-API
    Q_PROPERTY(QStringList buttonFunctions READ buttonFunctions CONSTANT)
    Q_PROPERTY(QStringList buttonKeys READ buttonKeys CONSTANT)

 public:
    // whether the UI can execute web view commands, without loading the library; by default whether the build has the
    // web view and the firmware's library is installed
    using Installed = std::function<bool()>;
    // whether the web view can show pages; by default the firmware's library (loaded by the first call)
    using Availability = std::function<bool()>;
    enum class ClearStart {
        Started,      // `done` reports the outcome later, from the event loop
        Unsupported,  // the library cannot clear website data
        Refused,      // the library refused the request; `done` is not called
    };
    // clears the website data of all web views: the HTTP cache (all false) or everything (all true); done(ok) when the
    // library is done, only after Started
    using ClearWebsiteData = std::function<ClearStart(bool all, std::function<void(bool ok)> done)>;
    // the global settings for the library: colour scheme ("light", "dark"), invalid certificates of all hosts
    using GlobalSettings = std::function<void(const QString &colorScheme, bool allowInvalidCertificates)>;
    // whether the library can clear website data (library 0.8.0), loading it on the first call
    using ClearSupported = std::function<bool()>;

    // the outcome of clearData()
    enum ClearResult {
        Cleared,      // the library is done, the shown pages reload
        Failed,       // the library refused the request or reported a failure
        Unsupported,  // the library cannot clear website data (older than 0.8.0)
    };
    Q_ENUM(ClearResult)

    // model: the remote model, whose buttons the button settings list
    explicit WebViewController(core::Api *core, EntityController *entityController, const QString &version,
                               hw::HardwareModel::Enum model, QObject *parent = nullptr);
    ~WebViewController();

    QString fullscreenEntityId() const { return m_fullscreenEntityId; }
    void    setFullscreenEntityId(const QString &entityId);

    // the UI declared `webview` to the core with its last set_ui_info
    bool declaresWebviews() const { return m_declaredTypes.contains(QStringLiteral("webview")); }

    // The entity types set_ui_info names: `webview` while the library is installed, and once a use loaded it, while
    // that load succeeded. Does not load the library. Public for the tests.
    QStringList entityTypesToDeclare() const;

    // The outcome of a load of the library, reported by the library (a page or the Settings menu loaded it) or found by
    // a command: sends the declaration again when it changes it. Public for the tests.
    void libraryLoaded(bool usable);

    core::cfgWebview webviewCfg() const { return m_cfg; }

    QString colorScheme() const { return m_cfg.colorScheme; }
    bool    allowInvalidCertificates() const { return m_cfg.allowInvalidCertificates; }
    QString websiteData() const { return m_cfg.websiteData; }
    QStringList buttonIds() const;
    QVariantMap buttons() const { return m_buttons; }
    QStringList buttonFunctions() const;
    QStringList buttonKeys() const;
    // set_webview_cfg with this setting only; a value the Core-API does not define is ignored
    void setColorScheme(const QString &colorScheme);
    void setAllowInvalidCertificates(bool allow);
    void setWebsiteData(const QString &websiteData);
    /**
     * @brief set_webview_cfg with the action of this one button, which the core merges into its buttons. A button that
     * is not one of this model's, an invalid action or the button's action as it is sends nothing.
     */
    Q_INVOKABLE void setButtonAction(const QString &button, const QVariantMap &action);
    // the action of a button when neither the setting nor a web view changes it
    Q_INVOKABLE QVariantMap defaultButtonAction(const QString &button) const;
    // whether a held button repeats the action (key events, the scroll and page functions)
    Q_INVOKABLE bool buttonRepeats(const QVariantMap &action) const;

    // The Settings page: whether clearData() can clear, false with a library older than 0.8.0 and without the web view.
    Q_INVOKABLE bool canClearWebsiteData();
    /**
     * @brief Clears the HTTP cache (all false) or all website data (all true) of every web view, from the Settings
     * page: what the `clear_cache` and `clear_data` commands do, the shown pages reload. dataCleared() reports the
     * outcome, from inside the call when the library cannot start the clear.
     */
    Q_INVOKABLE void clearData(bool all);

    /**
     * @brief A web view closed: the fullscreen web view or the page of a widget. Applies the global website data
     * setting: the HTTP cache or all website data of every web view is cleared when the setting asks for it.
     */
    Q_INVOKABLE void viewClosed();

    // tests: replace the library behind the installation check, availability, clearing and the global settings
    void setInstalledForTest(Installed installed) { m_installed = std::move(installed); }
    void setAvailabilityForTest(Availability availability) { m_available = std::move(availability); }
    void setClearWebsiteDataForTest(ClearWebsiteData clear) { m_clearWebsiteData = std::move(clear); }
    void setGlobalSettingsForTest(GlobalSettings settings) { m_globalSettings = std::move(settings); }
    void setClearSupportedForTest(ClearSupported supported) { m_clearSupported = std::move(supported); }

    // the answer to an entity_command request: the result code and its message
    using Answer = std::function<void(int code, const QString &message)>;
    // Executes an entity_command request (signals, the library) and answers it exactly once: at once, or for
    // `clear_cache` and `clear_data` when the library is done. Public for the tests.
    void execute(const core::UiEntityCommand &command, const Answer &answer);

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void fullscreenEntityIdChanged();
    // `open`: show the web view fullscreen, on top of the current screen (main.qml)
    void openRequested(QString entityId);
    // `reload`, after `clear_cache` and `clear_data`: the shown page of the web view reloads; an empty entityId means
    // every shown web view
    void reloadRequested(QString entityId);
    // the global web view settings changed: get_webview_cfg, set_webview_cfg, a configuration change, or a refused
    // change (the value is the one before)
    void webviewCfgChanged();
    // the end of clearData(): the cache (all false) or all website data, ClearResult
    void dataCleared(bool all, int result);

 private:
    void onCoreConnected();
    void onUiEntityCommand(const core::UiEntityCommand &command);
    void applyCfg(const core::cfgWebview &cfg);
    void changeCfg(const QVariantMap &update);
    void clearWebsiteData(bool all, const Answer &answer);
    void sendUiInfo();
    bool available();

    static WebViewController *s_instance;

    core::Api        *m_core;
    EntityController *m_entityController;
    const QString     m_version;
    const QStringList m_buttonIds;

    Installed        m_installed;
    Availability     m_available;
    ClearWebsiteData m_clearWebsiteData;
    GlobalSettings   m_globalSettings;
    ClearSupported   m_clearSupported;

    // the library's load: none yet (the installation check decides the declaration), or its outcome, which stays for
    // the process (a failed library is not loaded again)
    enum class LibraryState { NotLoaded, Loaded, Failed };
    LibraryState m_libraryState = LibraryState::NotLoaded;
    QStringList  m_declaredTypes;
    // only the answer of the latest settings request, get or set, is applied (ADR 0017)
    int              m_cfgRequestId = -1;
    core::cfgWebview m_cfg;
    // every button of the model with its action: the core's, else the default
    QVariantMap m_buttons;
    QString     m_fullscreenEntityId;
};

}  // namespace ui
}  // namespace uc
