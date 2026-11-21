// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Settings > Web view: the page as the settings menu loads it (qrc:/settings/settings/WebView.qml) with the real
// WebViewController behind WebView.Controller, on a core that is not connected, the real InputController behind
// ui.inputController (qml/SettingsHost.qml) and the real resources with the embedded icon font. The text fonts are the
// firmware's (Poppins, Space Mono) and not part of the repository: UC_TEST_FONT_DIR names a directory with their .ttf
// files, else the system's fallback font draws the texts. The global web view settings show what the core reports and a
// change goes to the controller; the clear buttons run the controller's clear, Clear website data after a question;
// without a library that can clear, both are disabled; Buttons opens the page of the remote's buttons in a full-screen
// web page and Web view test the test page, each over the page, and BACK closes them.

#include <QDirIterator>
#include <QFontDatabase>
#include <QJSValue>
#include <QPointer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QtTest>

#include <algorithm>
#include <functional>
#include <utility>

#include "config/config.h"
#include "core/core.h"
#include "hardware/hardwareModel.h"
#include "ui/colors.h"
#include "ui/entity/entityController.h"
#include "ui/entity/webviewController.h"
#include "ui/fonts.h"
#include "ui/iconFont.h"
#include "ui/inputController.h"
#include "ui/notification.h"
#include "ui/resources.h"
#include "webview/webviewitem.h"
#include "webview/webviewlibrary.h"

using uc::ui::Colors;
using uc::ui::EntityController;
using uc::ui::Fonts;
using uc::ui::InputController;
using uc::ui::WebViewController;
using uc::webview::WebViewFunctions;
using uc::webview::WebViewItem;
using uc::webview::WebViewLibrary;

namespace {

// The `ui` context property as far as the pages and their components use it (uiController.h).
class UiStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *inputController READ inputController CONSTANT)
    Q_PROPERTY(int cornerRadiusSmall READ cornerRadiusSmall CONSTANT)
    Q_PROPERTY(int cornerRadiusLarge READ cornerRadiusLarge CONSTANT)
    Q_PROPERTY(bool keyNavigationActive READ keyNavigationActive CONSTANT)
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(bool webViewAvailable READ webViewAvailable CONSTANT)

 public:
    explicit UiStub(QObject *parent = nullptr) : QObject(parent), m_input(uc::hw::HardwareModel::DEV) {}

    QObject *inputController() { return &m_input; }
    int      cornerRadiusSmall() const { return 10; }
    int      cornerRadiusLarge() const { return 20; }
    bool     keyNavigationActive() const { return true; }
    int      width() const { return 480; }
    bool     webViewAvailable() const { return true; }

    Q_INVOKABLE void setTimeOut(int msec, QJSValue callback) {
        QTimer::singleShot(msec, this, [callback]() mutable {
            if (callback.isCallable()) {
                callback.call();
            }
        });
    }

    // the toast at the top of the screen
    Q_INVOKABLE void createNotification(const QString &message, bool warning = false) {
        notifications.append((warning ? QStringLiteral("warning: ") : QString()) + message);
    }

    // the question with Cancel and the action (uiController.h): the test answers it with confirm()
    Q_INVOKABLE void createActionableWarningNotification(const QString &title, const QString &message,
                                                         const QString &icon, QJSValue action,
                                                         const QString &actionLabel) {
        Q_UNUSED(icon)
        questions.append(title + QStringLiteral(" | ") + message + QStringLiteral(" | ") + actionLabel);
        m_action = action;
    }

    void confirm() {
        if (m_action.isCallable()) {
            m_action.call();
        }
        m_action = QJSValue();
    }

    InputController *input() { return &m_input; }

    QStringList notifications;
    QStringList questions;

 private:
    InputController m_input;
    QJSValue        m_action;
};

class HapticStub : public QObject {
    Q_OBJECT
 public:
    enum Effects { Click = 0, Buzz = 1, Error = 2, Bump = 3 };
    Q_ENUM(Effects)

    Q_INVOKABLE void play(Effects) {}

    static QObject *qmlInstance(QQmlEngine *, QJSEngine *) { return new HapticStub; }
};

// libucwebview with nothing behind it, for the WebView item of the test page
namespace lib {

uc_webview *const kView = reinterpret_cast<uc_webview *>(0x1234);

int views = 0;
int apiVersion() {
    return UC_WEBVIEW_API_VERSION;
}
int globalInit(void *, const char *, const char *) {
    return 0;
}
void        globalShutdown() {}
uc_webview *create(int, int, const uc_webview_callbacks *, void *) {
    views++;
    return kView;
}
void destroy(uc_webview *) {}
void resize(uc_webview *, int, int) {}
void setInt(uc_webview *, int) {}
void setDouble(uc_webview *, double) {}
void setString(uc_webview *, const char *) {}
void loadHtml(uc_webview *, const char *, const char *) {}
void noArgs(uc_webview *) {}
int  canGo(uc_webview *) {
    return 0;
}
const char *none(uc_webview *) {
    return nullptr;
}
void setBackground(uc_webview *, float, float, float, float) {}
void frame(uc_webview *, void *) {}
void touch(uc_webview *, int, int, double, double, uint32_t) {}
void pointerButton(uc_webview *, int, int, double, double, unsigned, uint32_t) {}
void pointerMove(uc_webview *, double, double, unsigned, uint32_t) {}
void scroll(uc_webview *, double, double, double, double, int, uint32_t) {}
void key(uc_webview *, uint32_t, uint32_t, int, unsigned, uint32_t) {}
void imePreedit(uc_webview *, const char *, int) {}
void imeDeleteSurrounding(uc_webview *, int, int) {}

WebViewFunctions functions() {
    WebViewFunctions f;
    f.api_version = &apiVersion;
    f.global_init = &globalInit;
    f.global_shutdown = &globalShutdown;
    f.create = &create;
    f.destroy = &destroy;
    f.resize = &resize;
    f.set_mapped = &setInt;
    f.set_focused = &setInt;
    f.set_scale = &setDouble;
    f.load_uri = &setString;
    f.load_html = &loadHtml;
    f.reload = &noArgs;
    f.stop = &noArgs;
    f.go_back = &noArgs;
    f.go_forward = &noArgs;
    f.can_go_back = &canGo;
    f.can_go_forward = &canGo;
    f.get_uri = &none;
    f.get_title = &none;
    f.run_javascript = &setString;
    f.set_zoom = &setDouble;
    f.set_background_rgba = &setBackground;
    f.frame_rendered = &frame;
    f.frame_release = &frame;
    f.touch = &touch;
    f.pointer_button = &pointerButton;
    f.pointer_move = &pointerMove;
    f.scroll = &scroll;
    f.key = &key;
    f.ime_commit = &setString;
    f.ime_preedit = &imePreedit;
    f.ime_delete_surrounding = &imeDeleteSurrounding;
    return f;
}

}  // namespace lib

// a Components.Button by its text
QQuickItem *buttonOf(QQuickItem *page, const QString &text) {
    for (QQuickItem *item : page->findChildren<QQuickItem *>()) {
        if (item->metaObject()->indexOfProperty("trigger") >= 0 && item->property("text").toString() == text) {
            return item;
        }
    }
    return nullptr;
}

// an item by its object name, also among the delegates of a Repeater, which are no QObject children of it
QQuickItem *itemNamed(QQuickItem *in, const QString &name) {
    if (!in) {
        return nullptr;
    }
    for (QQuickItem *child : in->childItems()) {
        if (child->objectName() == name) {
            return child;
        }
        if (QQuickItem *found = itemNamed(child, name)) {
            return found;
        }
    }
    return in->findChild<QQuickItem *>(name);
}

// every item under in, the delegates of a Repeater too
QList<QQuickItem *> allItems(QQuickItem *in) {
    QList<QQuickItem *> items;
    for (QQuickItem *child : in->childItems()) {
        items << child << allItems(child);
    }
    for (QQuickItem *child : in->findChildren<QQuickItem *>()) {
        if (!items.contains(child)) {
            items << child;
        }
    }
    return items;
}

// the text of a QML Text item with that text, to find the help texts
bool hasText(QQuickItem *page, const QString &text) {
    for (QQuickItem *item : page->findChildren<QQuickItem *>()) {
        if (item->inherits("QQuickText") && item->property("text").toString() == text && item->isVisible()) {
            return true;
        }
    }
    return false;
}

}  // namespace

class testWebViewSettings : public QObject {
    Q_OBJECT

 private slots:
    void init();
    void cleanup();

    void settingsShowWhatTheCoreReports();
    void dpadWalksTheRowsAndButtons();
    void colorSchemeAndWebsiteDataArePickedFromAList();
    void refusedSwitchShowsTheSettingAsItIs();
    void clearCacheRunsTheClearAndTellsTheOutcome();
    void clearWebsiteDataAsksFirst();
    void clearButtonsAreDisabledWithAnOlderLibrary();
    void webViewTestOpensOverThePageAndBackClosesIt();
    void webViewTestCoversThePageBelow();
    void noTitleOrTextIsCutOff_data();
    void noTitleOrTextIsCutOff();
    void buttonsOpensItsPageOverThePageAndBackClosesIt();
    void buttonsPageListsTheButtonsOfTheModel_data();
    void buttonsPageListsTheButtonsOfTheModel();
    void anActionIsPickedFromItsListAndAKeyFromTheKeyList();
    void buttonsPageShowsAChangeMadeElsewhereAndCutsNoText();

 private:
    // the controller for the remote model, with the test's library hooks
    void createController(uc::hw::HardwareModel::Enum model);
    // creates the page: after the controller's library hooks are set, which the page reads once
    void showPage();
    // opens Buttons with OK: the buttons page, or null
    QQuickItem *openButtonsPage();
    // the rows of a PopupList: "<name>|<value>|<secondary>"
    QStringList listRows(QQuickItem *list) const;
    bool pressRemoteButton(Qt::Key key);
    // the PopupList the page opened, or null
    QQuickItem *popupList() const;
    void        reportSettings(const char *json);
    QQuickItem *row(const char *name) const { return page->findChild<QQuickItem *>(name); }

    using ClearStart = WebViewController::ClearStart;

    // the controller on a core that is never connected: a change is not sent and reported back as it is
    QScopedPointer<uc::ui::Notification> notification;
    QScopedPointer<uc::core::Api>        api;
    QScopedPointer<EntityController>     entities;
    QScopedPointer<WebViewController>    controller;
    bool                                 clearSupported = true;
    ClearStart                           clearStart = ClearStart::Started;
    QList<bool>                          clears;
    QList<std::function<void(bool)>>     clearDone;

    QQuickView        *view = nullptr;
    UiStub            *ui = nullptr;
    uc::ui::Resources *resource = nullptr;
    Colors             colors;
    Fonts              fonts;
    QQuickItem        *page = nullptr;
};

void testWebViewSettings::init() {
    notification.reset(new uc::ui::Notification);
    api.reset(new uc::core::Api(QStringLiteral("ws://127.0.0.1:1/ws")));
    entities.reset(new EntityController(api.data(), QStringLiteral("en"), uc::Config::UnitSystems::Metric, 0));
    createController(uc::hw::HardwareModel::UCR3);
    clearSupported = true;
    clearStart = ClearStart::Started;
    clears.clear();
    clearDone.clear();
    lib::views = 0;
    ui = new UiStub(this);
    // the icons of the rows (chevrons) as the app draws them: the embedded icon font
    const QString iconFamily = uc::ui::IconFont::load(QString()).family;
    fonts.setIconFamily(iconFamily);
    resource = new uc::ui::Resources(QString(), QString(), this);
    resource->setIconFont(iconFamily);
}

void testWebViewSettings::createController(uc::hw::HardwareModel::Enum model) {
    // one controller at a time: it is the QML singleton
    controller.reset();
    controller.reset(new WebViewController(api.data(), entities.data(), QStringLiteral("1.2.3"), model));
    controller->setGlobalSettingsForTest([](const QString &, bool) {});
    controller->setAvailabilityForTest([] { return true; });
    controller->setClearSupportedForTest([this] { return clearSupported; });
    controller->setClearWebsiteDataForTest([this](bool all, std::function<void(bool)> done) {
        clears.append(all);
        if (clearStart == ClearStart::Started) {
            clearDone.append(std::move(done));
        }
        return clearStart;
    });
}

void testWebViewSettings::showPage() {
    view = new QQuickView;
    view->rootContext()->setContextProperty("ui", ui);
    view->rootContext()->setContextProperty("colors", &colors);
    view->rootContext()->setContextProperty("fonts", &fonts);
    view->rootContext()->setContextProperty("resource", resource);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->resize(480, 850);
    view->setSource(QUrl("qrc:/test/SettingsHost.qml"));
    QStringList errors;
    for (const QQmlError &error : view->errors()) {
        errors.append(error.toString());
    }
    QVERIFY2(view->status() == QQuickView::Ready, qPrintable(errors.join("\n")));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    page = view->rootObject()->findChild<QQuickItem *>("page");
    QVERIFY(page);

    // the settings container hands the input to the page; its ButtonNavigation then claims the focus
    ui->input()->setSource(view);
    ui->input()->takeControl(page);
    QTRY_VERIFY(row("colorSchemeRow")->hasActiveFocus());
}

void testWebViewSettings::cleanup() {
    delete view;
    view = nullptr;
    page = nullptr;
    delete ui;
    ui = nullptr;
    delete resource;
    resource = nullptr;
    controller.reset();
    entities.reset();
    api.reset();
    notification.reset();
}

bool testWebViewSettings::pressRemoteButton(Qt::Key key) {
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &release);
    return press.isAccepted();
}

QQuickItem *testWebViewSettings::popupList() const {
    for (QQuickItem *item : page->findChildren<QQuickItem *>()) {
        if (QString::fromLatin1(item->metaObject()->className()).startsWith(QLatin1String("PopupList_QMLTYPE"))) {
            return item;
        }
    }
    return nullptr;
}

void testWebViewSettings::reportSettings(const char *json) {
    emit api->cfgWebviewChanged(uc::core::Api::parseWebviewCfg(QJsonDocument::fromJson(json).toVariant().toMap()));
}

void testWebViewSettings::settingsShowWhatTheCoreReports() {
    showPage();
    QQuickItem *colorScheme = row("colorSchemeRow");
    QQuickItem *websiteData = row("websiteDataRow");
    QQuickItem *certificates = row("certificatesSwitch");
    QVERIFY(colorScheme && websiteData && certificates);

    // the Core-API's defaults until the core answers
    QCOMPARE(colorScheme->property("value").toString(), QStringLiteral("Dark"));
    QCOMPARE(websiteData->property("value").toString(), QStringLiteral("Keep"));
    QVERIFY(!certificates->property("checked").toBool());

    // get_webview_cfg, or a change made in the web configurator (configuration_change)
    reportSettings(
        R"({"color_scheme": "light", "website_data": "clear_all_on_close", "allow_invalid_certificates": true})");
    QCOMPARE(colorScheme->property("value").toString(), QStringLiteral("Light"));
    QCOMPARE(websiteData->property("value").toString(), QStringLiteral("Clear all data"));
    QVERIFY(certificates->property("checked").toBool());
    reportSettings(R"({"color_scheme": "dark", "website_data": "clear_cache_on_close"})");
    QCOMPARE(websiteData->property("value").toString(), QStringLiteral("Clear cache"));
    QCOMPARE(colorScheme->property("value").toString(), QStringLiteral("Dark"));
    QVERIFY(!certificates->property("checked").toBool());
    QVERIFY(hasText(page, QStringLiteral("All web pages share their cache, cookies and stored data.")));
}

void testWebViewSettings::dpadWalksTheRowsAndButtons() {
    showPage();
    const QList<QQuickItem *> chain{row("colorSchemeRow"),     row("websiteDataRow"),   row("buttonsRow"),
                                    row("certificatesSwitch"), row("clearCacheButton"), row("clearDataButton"),
                                    row("testPageRow")};
    for (QQuickItem *item : chain) {
        QVERIFY(item);
    }
    for (int i = 1; i < chain.size(); ++i) {
        QTest::keyClick(view, Qt::Key_Down);
        QTRY_VERIFY2(chain.at(i)->hasActiveFocus(), qPrintable(chain.at(i)->objectName()));
    }
    for (int i = chain.size() - 2; i >= 0; --i) {
        QTest::keyClick(view, Qt::Key_Up);
        QTRY_VERIFY2(chain.at(i)->hasActiveFocus(), qPrintable(chain.at(i)->objectName()));
    }
}

void testWebViewSettings::colorSchemeAndWebsiteDataArePickedFromAList() {
    showPage();
    QSignalSpy changed(controller.data(), &WebViewController::webviewCfgChanged);

    // OK on the row opens the list with the current value; a choice goes to the controller, which sends it to the
    // core. Not connected here: it is reported back as it is, the row keeps its value.
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(popupList() != nullptr);
    QCOMPARE(popupList()->property("currentValue").toString(), QStringLiteral("dark"));
    QVERIFY(QMetaObject::invokeMethod(popupList(), "itemSelected", Q_ARG(QVariant, QStringLiteral("light"))));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(row("colorSchemeRow")->property("value").toString(), QStringLiteral("Dark"));
    QVERIFY(QMetaObject::invokeMethod(popupList(), "done"));
    QTRY_VERIFY(popupList() == nullptr);
    QTRY_VERIFY(row("colorSchemeRow")->hasActiveFocus());

    // the website data list
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(row("websiteDataRow")->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(popupList() != nullptr);
    QCOMPARE(popupList()->property("currentValue").toString(), QStringLiteral("keep"));
    QVERIFY(
        QMetaObject::invokeMethod(popupList(), "itemSelected", Q_ARG(QVariant, QStringLiteral("clear_all_on_close"))));
    QCOMPARE(changed.count(), 2);
    QVERIFY(QMetaObject::invokeMethod(popupList(), "done"));
    QTRY_VERIFY(popupList() == nullptr);

    // the core's answer shows
    reportSettings(R"({"color_scheme": "light", "website_data": "clear_all_on_close"})");
    QCOMPARE(row("colorSchemeRow")->property("value").toString(), QStringLiteral("Light"));
    QCOMPARE(row("websiteDataRow")->property("value").toString(), QStringLiteral("Clear all data"));
}

void testWebViewSettings::refusedSwitchShowsTheSettingAsItIs() {
    showPage();
    QQuickItem *certificates = row("certificatesSwitch");
    certificates->forceActiveFocus();
    QTRY_VERIFY(certificates->hasActiveFocus());

    // OK toggles the switch, the controller cannot send the change: the switch shows the setting again
    QSignalSpy changed(controller.data(), &WebViewController::webviewCfgChanged);
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(changed.count(), 1);
    QVERIFY(!controller->allowInvalidCertificates());
    QTRY_VERIFY(!certificates->property("checked").toBool());

    // a change the core accepted: the switch follows it
    reportSettings(R"({"allow_invalid_certificates": true})");
    QVERIFY(certificates->property("checked").toBool());
}

void testWebViewSettings::clearCacheRunsTheClearAndTellsTheOutcome() {
    showPage();
    QQuickItem *clearCache = row("clearCacheButton");
    QQuickItem *clearData = row("clearDataButton");
    QVERIFY(clearCache->isEnabled() && clearData->isEnabled());

    // no question for the cache; the buttons do nothing until the clear ends, and keep the d-pad
    clearCache->forceActiveFocus();
    QTRY_VERIFY(clearCache->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(clears, QList<bool>{false});
    QVERIFY(ui->questions.isEmpty());
    QTest::keyClick(view, Qt::Key_Return);
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier,
                      clearData->mapToScene(QPointF(clearData->width() / 2, clearData->height() / 2)).toPoint());
    QCOMPARE(clears, QList<bool>{false});
    QVERIFY(ui->questions.isEmpty());
    QVERIFY(clearCache->hasActiveFocus());
    clearDone.takeFirst()(true);
    QCOMPARE(ui->notifications, QStringList{QStringLiteral("Cache cleared")});
    clearCache->forceActiveFocus();

    // a failure is told as a warning
    QTest::keyClick(view, Qt::Key_Return);
    clearDone.takeFirst()(false);
    QCOMPARE(ui->notifications.last(), QStringLiteral("warning: The cache could not be cleared"));

    // a refused request, at once
    clearStart = ClearStart::Refused;
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(ui->notifications.last(), QStringLiteral("warning: The cache could not be cleared"));
    QCOMPARE(ui->notifications.size(), 3);
    QVERIFY(clearCache->isEnabled());
}

void testWebViewSettings::clearWebsiteDataAsksFirst() {
    showPage();
    QQuickItem *clearData = row("clearDataButton");
    clearData->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Return);

    // the question first: it signs the user out of pages with a cookie login
    QCOMPARE(ui->questions.size(), 1);
    QVERIFY(ui->questions.first().startsWith(QStringLiteral("Clear website data | ")));
    QVERIFY(ui->questions.first().contains(QStringLiteral("sign in again")));
    QVERIFY(ui->questions.first().endsWith(QStringLiteral(" | Clear")));
    QVERIFY(clears.isEmpty());

    ui->confirm();
    QCOMPARE(clears, QList<bool>{true});
    clearDone.takeFirst()(true);
    QCOMPARE(ui->notifications, QStringList{QStringLiteral("Website data cleared")});
}

void testWebViewSettings::clearButtonsAreDisabledWithAnOlderLibrary() {
    clearSupported = false;
    showPage();
    QVERIFY(!row("clearCacheButton")->isEnabled());
    QVERIFY(!row("clearDataButton")->isEnabled());
    QVERIFY(hasText(page, QStringLiteral("Clearing needs a newer firmware.")));

    // the d-pad passes over them
    row("certificatesSwitch")->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(row("testPageRow")->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Up);
    QTRY_VERIFY(row("certificatesSwitch")->hasActiveFocus());
}

void testWebViewSettings::webViewTestOpensOverThePageAndBackClosesIt() {
    showPage();
    QQuickItem *testRow = row("testPageRow");
    testRow->forceActiveFocus();
    QTRY_VERIFY(testRow->hasActiveFocus());

    // OK opens the test page over the whole page; it has the input and its own focus
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(buttonOf(page, QStringLiteral("Load")) != nullptr);
    QVERIFY(!page->findChildren<WebViewItem *>().isEmpty());
    QQuickItem *loadButton = buttonOf(page, QStringLiteral("Load"));
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QCOMPARE(lib::views, 1);
    QVERIFY(ui->input()->activeItem() != page);

    // BACK closes it: the settings page has the input again, on the row the user came from; the settings stay open
    QVERIFY(!pressRemoteButton(Qt::Key_Exit));
    QTRY_VERIFY(page->findChildren<WebViewItem *>().isEmpty());
    QTRY_COMPARE(ui->input()->activeItem(), page);
    QTRY_VERIFY(testRow->hasActiveFocus());
    QCOMPARE(view->rootObject()->findChild<QObject *>("profileRoot")->property("backCalls").toInt(), 0);

    // the title bar's arrow closes it as well
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(buttonOf(page, QStringLiteral("Load")) != nullptr);
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(40, 40));
    QTRY_VERIFY(page->findChildren<WebViewItem *>().isEmpty());
    QTRY_COMPARE(ui->input()->activeItem(), page);
    QCOMPARE(view->rootObject()->findChild<QObject *>("profileRoot")->property("backCalls").toInt(), 0);
    QCOMPARE(lib::views, 2);
}

void testWebViewSettings::webViewTestCoversThePageBelow() {
    // Regression: the test page opened transparent over this page, whose rows, help texts and buttons showed through
    // between its own controls. A settings page draws no background of its own (the settings overlay does).
    showPage();
    QQuickItem *testRow = row("testPageRow");
    QQuickItem *websiteData = row("websiteDataRow");
    QQuickItem *clearData = row("clearDataButton");
    testRow->forceActiveFocus();
    QTRY_VERIFY(testRow->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(buttonOf(page, QStringLiteral("Load")) != nullptr);

    // the test page has an opaque background in the theme's background colour, over the whole window and below every
    // control of its own
    QQuickItem *background = page->findChild<QQuickItem *>("webViewTestBackground");
    QVERIFY(background);
    QVERIFY(background->isVisible());
    const QColor color = background->property("color").value<QColor>();
    QCOMPARE(color, colors.property("bg").value<QColor>());
    QCOMPARE(color.alphaF(), 1.0);
    QCOMPARE(background->opacity(), 1.0);
    QVERIFY(background->z() < 0);
    QCOMPARE(background->mapRectToScene(QRectF(0, 0, background->width(), background->height())),
             QRectF(0, 0, view->width(), view->height()));

    // where the web page paints nothing yet (no frame here), the web view shows its background, white by default
    QQuickItem *webViewBackground = page->findChild<QQuickItem *>("webViewBackground");
    QVERIFY(webViewBackground);
    QVERIFY(webViewBackground->isVisible());
    QCOMPARE(webViewBackground->property("color").value<QColor>(), QColor(Qt::white));
    WebViewItem *webView = page->findChildren<WebViewItem *>().value(0);
    QVERIFY(webView);
    QCOMPARE(webViewBackground->mapRectToScene(QRectF(0, 0, webViewBackground->width(), webViewBackground->height())),
             webView->mapRectToScene(QRectF(0, 0, webView->width(), webView->height())));

    // the page below is hidden while the test page is open
    QVERIFY(!websiteData->isVisible());
    QVERIFY(!clearData->isVisible());
    QVERIFY(!testRow->isVisible());

    // and takes no touch: a tap on the test page's status line, above the Website data row of the page below, opens no
    // list, and the test page stays
    const QPoint overWebsiteData =
        websiteData->mapToScene(QPointF(websiteData->width() - 60, websiteData->height() / 2)).toPoint();
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, overWebsiteData);
    QTest::qWait(100);
    QVERIFY(popupList() == nullptr);
    QVERIFY(buttonOf(page, QStringLiteral("Load")) != nullptr);
    QVERIFY(ui->questions.isEmpty());

    // closed again, the page is back with its rows
    QVERIFY(!pressRemoteButton(Qt::Key_Exit));
    QTRY_VERIFY(page->findChildren<WebViewItem *>().isEmpty());
    QVERIFY(websiteData->isVisible() && clearData->isVisible() && testRow->isVisible());
    QTRY_VERIFY(testRow->hasActiveFocus());
}

void testWebViewSettings::noTitleOrTextIsCutOff_data() {
    QTest::addColumn<QByteArray>("settings");
    QTest::addColumn<bool>("clearSupported");

    // the longest values of the rows: "Clear cache", "Light"; and the help text of an older library
    QTest::newRow("defaults") << QByteArray(R"({})") << true;
    QTest::newRow("longest values") << QByteArray(
                                           R"({"color_scheme": "light", "website_data": "clear_cache_on_close"})")
                                    << true;
    QTest::newRow("clear all, older library")
        << QByteArray(R"({"website_data": "clear_all_on_close", "allow_invalid_certificates": true})") << false;
}

void testWebViewSettings::noTitleOrTextIsCutOff() {
    // Regression: the Website data row showed "Website data" and the value, and only the list said what it is about,
    // "Website data on close". Every row shows its whole title, the value and a line that says what it does. At the
    // panel's 480 px no text of the page may be elided or cut at its line limit.
    QFETCH(QByteArray, settings);
    QFETCH(bool, clearSupported);
    this->clearSupported = clearSupported;
    showPage();
    QCOMPARE(view->width(), 480);
    reportSettings(settings.constData());

    int texts = 0;
    for (QQuickItem *item : page->findChildren<QQuickItem *>()) {
        if (!item->inherits("QQuickText") || !item->isVisible() || item->property("text").toString().isEmpty()) {
            continue;
        }
        texts++;
        QVERIFY2(!item->property("truncated").toBool(), qPrintable(item->property("text").toString()));
    }
    QVERIFY(texts >= 10);

    // the picker rows and the test page row: the whole title, the value at its end, and the description under it
    const QList<QPair<const char *, QString>> rows{{"colorSchemeRow", QStringLiteral("Color scheme")},
                                                   {"websiteDataRow", QStringLiteral("Website data on close")},
                                                   {"buttonsRow", QStringLiteral("Buttons")},
                                                   {"testPageRow", QStringLiteral("Web view test")}};
    for (const auto &entry : rows) {
        QQuickItem *menuRow = row(entry.first);
        QVERIFY(menuRow);
        QCOMPARE(menuRow->property("text").toString(), entry.second);
        QVERIFY2(!menuRow->property("description").toString().isEmpty(), entry.first);
        QQuickItem *label = menuRow->findChild<QQuickItem *>("menuRowLabel");
        QQuickItem *description = menuRow->findChild<QQuickItem *>("menuRowDescription");
        QVERIFY(label && description && description->isVisible());
        QVERIFY2(!label->property("truncated").toBool(), entry.first);
        QVERIFY2(!description->property("truncated").toBool(), entry.first);
        // in the firmware's font (UC_TEST_FONT_DIR) the description is one line on the 480 px panel; the colour
        // scheme's has the web configurator's text, which takes two
        if (QFontInfo(fonts.help()).family() == QLatin1String("Poppins")) {
            QVERIFY2(description->property("lineCount").toInt()
                         == (qstrcmp(entry.first, "colorSchemeRow") == 0 ? 2 : 1),
                     entry.first);
        }
        // inside the row, the description under the title
        QVERIFY(description->y() >= label->y() + label->height());
        QVERIFY(description->y() + description->height() <= menuRow->height());
        QVERIFY(label->x() + label->width() <= menuRow->width());
    }
    QCOMPARE(row("websiteDataRow")->property("value").toString(),
             settings.contains("clear_cache") ? QStringLiteral("Clear cache")
             : settings.contains("clear_all") ? QStringLiteral("Clear all data")
                                              : QStringLiteral("Keep"));
}

QQuickItem *testWebViewSettings::openButtonsPage() {
    QQuickItem *buttons = row("buttonsRow");
    buttons->forceActiveFocus();
    if (!QTest::qWaitFor([buttons] { return buttons->hasActiveFocus(); })) {
        return nullptr;
    }
    QTest::keyClick(view, Qt::Key_Return);
    QQuickItem *buttonsPage = nullptr;
    QTest::qWaitFor([this, &buttonsPage] {
        buttonsPage = page->findChild<QQuickItem *>("webViewButtonsBackground");
        return buttonsPage != nullptr;
    });
    return buttonsPage ? buttonsPage->parentItem() : nullptr;
}

QStringList testWebViewSettings::listRows(QQuickItem *list) const {
    QStringList rows;
    QObject    *model = list ? list->property("listModel").value<QObject *>() : nullptr;
    if (!model) {
        return rows;
    }
    for (int i = 0; i < model->property("count").toInt(); ++i) {
        QJSValue entry;
        QMetaObject::invokeMethod(model, "get", Q_RETURN_ARG(QJSValue, entry), Q_ARG(int, i));
        rows << entry.property("name").toString() + "|" + entry.property("value").toString() + "|" +
                    entry.property("secondary").toString();
    }
    return rows;
}

void testWebViewSettings::buttonsOpensItsPageOverThePageAndBackClosesIt() {
    showPage();
    QQuickItem *buttonsRow = row("buttonsRow");
    QVERIFY(buttonsRow);
    QCOMPARE(buttonsRow->property("text").toString(), QStringLiteral("Buttons"));

    // OK opens the buttons page over the whole page: opaque, it has the input and the focus on its first row
    QQuickItem *buttonsPage = openButtonsPage();
    QVERIFY(buttonsPage);
    QQuickItem *background = page->findChild<QQuickItem *>("webViewButtonsBackground");
    QCOMPARE(background->property("color").value<QColor>(), colors.property("bg").value<QColor>());
    QCOMPARE(background->mapRectToScene(QRectF(0, 0, background->width(), background->height())),
             QRectF(0, 0, view->width(), view->height()));
    QTRY_COMPARE(ui->input()->activeItem(), buttonsPage);
    QQuickItem *backRow = itemNamed(page, QStringLiteral("buttonRow_BACK"));
    QVERIFY(backRow);
    QTRY_VERIFY(backRow->hasActiveFocus());
    QCOMPARE(buttonsPage->property("topNavigationText").toString(), QStringLiteral("Buttons"));
    // the page below is hidden
    QVERIFY(!row("websiteDataRow")->isVisible());

    // the d-pad walks the rows
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(itemNamed(page, QStringLiteral("buttonRow_DPAD_UP"))->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Up);
    QTRY_VERIFY(backRow->hasActiveFocus());

    // BACK closes it: the settings page has the input again, on Buttons; the settings stay open
    QVERIFY(!pressRemoteButton(Qt::Key_Exit));
    QTRY_VERIFY(page->findChild<QQuickItem *>("webViewButtonsBackground") == nullptr);
    QTRY_COMPARE(ui->input()->activeItem(), page);
    QTRY_VERIFY(buttonsRow->hasActiveFocus());
    QVERIFY(row("websiteDataRow")->isVisible());
    QCOMPARE(view->rootObject()->findChild<QObject *>("profileRoot")->property("backCalls").toInt(), 0);

    // the title bar's arrow closes it as well
    QVERIFY(openButtonsPage());
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(40, 40));
    QTRY_VERIFY(page->findChild<QQuickItem *>("webViewButtonsBackground") == nullptr);
    QTRY_COMPARE(ui->input()->activeItem(), page);
    QCOMPARE(view->rootObject()->findChild<QObject *>("profileRoot")->property("backCalls").toInt(), 0);
}

void testWebViewSettings::buttonsPageListsTheButtonsOfTheModel_data() {
    QTest::addColumn<int>("model");
    QTest::addColumn<QStringList>("buttons");

    const QStringList common{"BACK",        "DPAD_UP",   "DPAD_DOWN",   "DPAD_LEFT",  "DPAD_RIGHT",
                             "DPAD_MIDDLE", "VOLUME_UP", "VOLUME_DOWN", "CHANNEL_UP", "CHANNEL_DOWN",
                             "MUTE",        "PREV",      "PLAY",        "NEXT"};
    QTest::newRow("Remote 3") << static_cast<int>(uc::hw::HardwareModel::UCR3)
                              << common + QStringList{"STOP", "RECORD", "MENU"};
    QTest::newRow("Remote Two") << static_cast<int>(uc::hw::HardwareModel::UCR2)
                                << common + QStringList{"RED", "GREEN", "YELLOW", "BLUE"};
}

void testWebViewSettings::buttonsPageListsTheButtonsOfTheModel() {
    // the buttons of the remote model, in order, each with its action under its name; HOME, POWER and VOICE are not
    // listed, neither are the buttons of the other model
    QFETCH(int, model);
    QFETCH(QStringList, buttons);
    createController(static_cast<uc::hw::HardwareModel::Enum>(model));
    showPage();
    QVERIFY(openButtonsPage());

    QList<QQuickItem *> rows;
    for (QQuickItem *item : allItems(page)) {
        if (item->objectName().startsWith(QLatin1String("buttonRow_"))) {
            rows.append(item);
        }
    }
    std::sort(rows.begin(), rows.end(),
              [](QQuickItem *a, QQuickItem *b) { return a->mapToScene(QPointF()).y() < b->mapToScene(QPointF()).y(); });
    QStringList listed;
    for (QQuickItem *item : rows) {
        listed << item->objectName().mid(10);
    }
    QCOMPARE(listed, buttons);

    // the default actions, as the rows say them
    auto description = [this](const char *button) {
        QQuickItem *item = itemNamed(page, QStringLiteral("buttonRow_") + QString::fromLatin1(button));
        return item ? item->property("description").toString() : QStringLiteral("(no row)");
    };
    QCOMPARE(itemNamed(page, QStringLiteral("buttonRow_VOLUME_UP"))->property("text").toString(),
             QStringLiteral("Volume up"));
    QCOMPARE(description("BACK"), QStringLiteral("Close"));
    QCOMPARE(description("DPAD_UP"), QStringLiteral("Sends ArrowUp"));
    QCOMPARE(description("DPAD_MIDDLE"), QStringLiteral("Sends Enter"));
    QCOMPARE(description("VOLUME_UP"), QStringLiteral("Zoom in"));
    QCOMPARE(description("CHANNEL_DOWN"), QStringLiteral("Page down"));
    QCOMPARE(description("MUTE"), QStringLiteral("Normal function"));
    if (model == uc::hw::HardwareModel::UCR3) {
        QCOMPARE(description("MENU"), QStringLiteral("Sends ContextMenu"));
    } else {
        QCOMPARE(description("RED"), QStringLiteral("Normal function"));
    }
    // the last row is reachable with the d-pad and shown
    QQuickItem *last = itemNamed(page, QStringLiteral("buttonRow_") + buttons.last());
    for (int i = 1; i < buttons.size(); ++i) {
        QTest::keyClick(view, Qt::Key_Down);
    }
    QTRY_VERIFY(last->hasActiveFocus());
    QQuickItem *flickable = last->parentItem()->parentItem()->parentItem()->parentItem();
    QVERIFY(flickable && flickable->inherits("QQuickFlickable"));
    QTRY_VERIFY2(last->mapToItem(flickable, QPointF(0, last->height())).y() <= flickable->height() + 1,
                 qPrintable(QStringLiteral("bottom %1 height %2 contentY %3 contentHeight %4")
                                .arg(last->mapToItem(flickable, QPointF(0, last->height())).y())
                                .arg(flickable->height())
                                .arg(flickable->property("contentY").toReal())
                                .arg(flickable->property("contentHeight").toReal())));
}

void testWebViewSettings::anActionIsPickedFromItsListAndAKeyFromTheKeyList() {
    showPage();
    QVERIFY(openButtonsPage());
    QQuickItem *volumeUp = itemNamed(page, QStringLiteral("buttonRow_VOLUME_UP"));
    volumeUp->forceActiveFocus();
    QTRY_VERIFY(volumeUp->hasActiveFocus());
    QSignalSpy changed(controller.data(), &WebViewController::webviewCfgChanged);

    // OK opens the list of the actions on the current one: Normal function, the functions, Send key to page; the
    // default marked
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(popupList() != nullptr);
    QCOMPARE(popupList()->property("title").toString(), QStringLiteral("Volume up"));
    QCOMPARE(popupList()->property("currentValue").toString(), QStringLiteral("function:zoom_in"));
    const QStringList actions = listRows(popupList());
    QCOMPARE(actions.size(), 21);
    QCOMPARE(actions.first(), QStringLiteral("Normal function|pass|"));
    QCOMPARE(actions.at(1), QStringLiteral("Zoom in|function:zoom_in|Default"));
    QCOMPARE(actions.at(16), QStringLiteral("Show or hide navigation|function:toggle_navigation|"));
    QCOMPARE(actions.at(19), QStringLiteral("Nothing|function:none|"));
    QCOMPARE(actions.last(), QStringLiteral("Send key to page|key|"));

    // a function: to the controller, which sends it to the core (not connected here: reported back as it is)
    QVERIFY(QMetaObject::invokeMethod(popupList(), "itemSelected", Q_ARG(QVariant, QStringLiteral("pass"))));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(volumeUp->property("description").toString(), QStringLiteral("Zoom in"));
    QVERIFY(QMetaObject::invokeMethod(popupList(), "done"));
    QTRY_VERIFY(popupList() == nullptr);
    QTRY_VERIFY(volumeUp->hasActiveFocus());
    // the core's answer shows
    reportSettings(R"({"buttons": {"VOLUME_UP": {"action": "pass"}}})");
    QCOMPARE(volumeUp->property("description").toString(), QStringLiteral("Normal function"));

    // Send key to page: the list of the keys follows, on the current key, the default marked
    QQuickItem *dpadUp = itemNamed(page, QStringLiteral("buttonRow_DPAD_UP"));
    dpadUp->forceActiveFocus();
    QTRY_VERIFY(dpadUp->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(popupList() != nullptr);
    QCOMPARE(popupList()->property("currentValue").toString(), QStringLiteral("key"));
    QCOMPARE(listRows(popupList()).last(), QStringLiteral("Send key to page|key|ArrowUp"));
    changed.clear();
    QVERIFY(QMetaObject::invokeMethod(popupList(), "itemSelected", Q_ARG(QVariant, QStringLiteral("key"))));
    QCOMPARE(changed.count(), 0);
    QVERIFY(QMetaObject::invokeMethod(popupList(), "done"));
    QTRY_VERIFY(popupList() != nullptr && popupList()->property("title").toString() == QStringLiteral("Key for Up"));
    QCOMPARE(popupList()->property("currentValue").toString(), QStringLiteral("ArrowUp"));
    const QStringList keys = listRows(popupList());
    QCOMPARE(keys.size(), 36);
    QCOMPARE(keys.first(), QStringLiteral("ArrowUp|ArrowUp|Default"));
    QVERIFY(keys.contains(QStringLiteral("ChannelUp|ChannelUp|")));
    QCOMPARE(keys.last(), QStringLiteral("ColorF3Blue|ColorF3Blue|"));
    QVERIFY(QMetaObject::invokeMethod(popupList(), "itemSelected", Q_ARG(QVariant, QStringLiteral("PageUp"))));
    QCOMPARE(changed.count(), 1);
    QVERIFY(QMetaObject::invokeMethod(popupList(), "done"));
    QTRY_VERIFY(popupList() == nullptr);
    QTRY_VERIFY(dpadUp->hasActiveFocus());
    reportSettings(R"({"buttons": {"DPAD_UP": {"action": "key", "key": "PageUp"}}})");
    QCOMPARE(dpadUp->property("description").toString(), QStringLiteral("Sends PageUp"));

    // the action a button has already sends nothing
    changed.clear();
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(popupList() != nullptr);
    QVERIFY(QMetaObject::invokeMethod(popupList(), "itemSelected", Q_ARG(QVariant, QStringLiteral("key"))));
    QVERIFY(QMetaObject::invokeMethod(popupList(), "done"));
    QTRY_VERIFY(popupList() != nullptr && popupList()->property("currentValue").toString() == "PageUp");
    QVERIFY(QMetaObject::invokeMethod(popupList(), "itemSelected", Q_ARG(QVariant, QStringLiteral("PageUp"))));
    QCOMPARE(changed.count(), 0);
}

void testWebViewSettings::buttonsPageShowsAChangeMadeElsewhereAndCutsNoText() {
    showPage();
    QVERIFY(openButtonsPage());
    // the longest actions on every row: a change made in the web configurator shows at once
    reportSettings(R"({"buttons": {
        "BACK": {"action": "function", "function": "toggle_navigation"},
        "DPAD_UP": {"action": "key", "key": "MediaTrackPrevious"}, "DPAD_DOWN": {"action": "key", "key": "MediaFastForward"},
        "DPAD_LEFT": {"action": "function", "function": "focus_previous"},
        "DPAD_RIGHT": {"action": "function", "function": "bottom"}, "DPAD_MIDDLE": {"action": "key", "key": "ColorF2Yellow"},
        "VOLUME_UP": {"action": "key", "key": "AudioVolumeDown"}, "VOLUME_DOWN": {"action": "key", "key": "MediaPlayPause"},
        "CHANNEL_UP": {"action": "function", "function": "back_or_close"},
        "CHANNEL_DOWN": {"action": "key", "key": "BrowserRefresh"}, "MUTE": {"action": "function", "function": "zoom_reset"},
        "PREV": {"action": "function", "function": "stop"}, "PLAY": {"action": "key", "key": "MediaTrackNext"},
        "NEXT": {"action": "function", "function": "focus_next"}, "STOP": {"action": "function", "function": "top"},
        "RECORD": {"action": "key", "key": "BrowserForward"}, "MENU": {"action": "function", "function": "none"}
    }})");
    QCOMPARE(itemNamed(page, QStringLiteral("buttonRow_BACK"))->property("description").toString(),
             QStringLiteral("Show or hide navigation"));
    QCOMPARE(itemNamed(page, QStringLiteral("buttonRow_DPAD_UP"))->property("description").toString(),
             QStringLiteral("Sends MediaTrackPrevious"));

    int texts = 0;
    for (QQuickItem *item : allItems(page)) {
        if (!item->inherits("QQuickText") || !item->isVisible() || item->property("text").toString().isEmpty()) {
            continue;
        }
        texts++;
        QVERIFY2(!item->property("truncated").toBool(), qPrintable(item->property("text").toString()));
        if (item->objectName() == QLatin1String("menuRowDescription") &&
            QFontInfo(fonts.help()).family() == QLatin1String("Poppins")) {
            // in the firmware's font every action is one line on the 480 px panel
            QVERIFY2(item->property("lineCount").toInt() == 1, qPrintable(item->property("text").toString()));
        }
    }
    QVERIFY(texts >= 2 * 17);
}

int main(int argc, char *argv[]) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) {
        qputenv("QT_QUICK_BACKEND", QByteArray("software"));
    }
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    // the firmware's text fonts, when the caller has them (they are not in the repository)
    const QString fontDir = qEnvironmentVariable("UC_TEST_FONT_DIR");
    if (!fontDir.isEmpty()) {
        QDirIterator fonts(fontDir, {QStringLiteral("*.ttf")}, QDir::Files, QDirIterator::Subdirectories);
        while (fonts.hasNext()) {
            QFontDatabase::addApplicationFont(fonts.next());
        }
    }

    WebViewLibrary::instance()->setFunctionsForTest(lib::functions());
    WebViewItem::registerQmlTypes();
    qmlRegisterSingletonType<HapticStub>("Haptic", 1, 0, "Haptic", &HapticStub::qmlInstance);
    qmlRegisterSingletonType<WebViewController>("WebView.Controller", 1, 0, "WebViewController",
                                                &WebViewController::qmlInstance);

    testWebViewSettings test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "test_webview_settings.moc"
