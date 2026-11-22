// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The keys of the fullscreen web view of a web view entity: the screen as main.qml loads it
// (qrc:/components/entities/webview/deviceclass/Webview.qml with WebViewPage.qml and WebViewRequests.qml) for a real
// entity::Webview, opened on its own (a profile page tile, the `open` command on the main screen) or on top of an
// activity screen (qml/EntityScreenHost.qml), with the real InputController behind ui.inputController and a stand-in
// web view library.
//
// Regression: VOLUME up and down did not zoom a fullscreen web view. The screen mapped no VOLUME at all: on top of an
// activity it inherited the activity's mapping (the receiver's volume), elsewhere the buttons did nothing. They now
// zoom the page like on the web view test page, one level of the WebView item per press, from the entity's zoom, for
// the visit of the page only; the other buttons keep the activity's mapping.
//
// The button mapping of the fullscreen web view (core-api entity_webview.md "Buttons"): every button of the remote model
// has an action, the web view's own before the global setting (WebViewControllerStub), with the defaults of
// entity::WebviewButtons: keys go to the page with their keycodes (the stand-in library records them), functions run,
// `pass` reaches the activity's mapping or the page header's; held buttons, the models, typing and dialogs.
//
// The load indicator of the fullscreen web view and of the web view widget (WebViewLoadIndicator.qml): a slow page
// showed a white screen only, a failed one WebKit's error page with the raw error text. The stand-in library's load
// events, progress and frames drive it. UC_TEST_FONT_DIR names a directory with the firmware's text fonts (Poppins,
// Space Mono, not in the repository) for the checks of the message's lines; else the system's fallback font draws it.

#include <QDirIterator>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QFontInfo>
#include <QJsonDocument>
#include <QPointer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QtTest>

#include <algorithm>
#include <cstring>
#include <utility>

#include "hardware/hardwareModel.h"
#include "ui/colors.h"
#include "ui/entity/webview.h"
#include "ui/entity/webviewButtons.h"
#include "ui/fonts.h"
#include "ui/inputController.h"
#include "webview/webviewitem.h"
#include "webview/webviewlibrary.h"

using uc::hw::HardwareModel;
using uc::ui::Colors;
using uc::ui::Fonts;
using uc::ui::InputController;
using uc::ui::entity::Webview;
using uc::ui::entity::WebviewButtons;
using uc::webview::WebViewFunctions;
using uc::webview::WebViewItem;
using uc::webview::WebViewLibrary;

namespace {

const QString kEntityId = QStringLiteral("uc_web.main.1");

// The `ui` context property as far as the screen and its components use it (uiController.h).
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

    Q_INVOKABLE void createNotification(const QString &message, bool warning = false) {
        notifications.append((warning ? QStringLiteral("warning: ") : QString()) + message);
    }

    InputController *input() { return &m_input; }

    QStringList notifications;

 private:
    InputController m_input;
};

// the singletons the screen imports, as far as it uses them
class HapticStub : public QObject {
    Q_OBJECT
 public:
    enum Effects { Click = 0, Buzz = 1, Error = 2, Bump = 3 };
    Q_ENUM(Effects)

    Q_INVOKABLE void play(Effects) {}

    static QObject *qmlInstance(QQmlEngine *, QJSEngine *) { return new HapticStub; }
};

// Entity.Controller: BaseDetail fetches the entity when its screen opens, the widget gets it by its id
class EntityControllerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool resumePending READ resumePending CONSTANT)

 public:
    bool             resumePending() const { return false; }
    Q_INVOKABLE void refreshEntity(const QString &) {}
    Q_INVOKABLE QObject *get(const QString &id) {
        if (!s_entity || id != s_entity->property("id").toString()) {
            return nullptr;
        }
        // the test owns it
        QQmlEngine::setObjectOwnership(s_entity, QQmlEngine::CppOwnership);
        return s_entity;
    }

    // the entity of the current test
    static QObject *s_entity;

    static QObject *qmlInstance(QQmlEngine *, QJSEngine *) { return new EntityControllerStub; }
};

QObject *EntityControllerStub::s_entity = nullptr;

// Integration.Controller: imported by BaseDetail
class IntegrationControllerStub : public QObject {
    Q_OBJECT
 public:
    static QObject *qmlInstance(QQmlEngine *, QJSEngine *) { return new IntegrationControllerStub; }
};

// WebView.Controller (webviewController.h): the open fullscreen web view, the reload requests and the global button
// setting, applied like the controller applies the core's (every button of the model, its action or the default)
class WebViewControllerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString fullscreenEntityId MEMBER fullscreenEntityId NOTIFY fullscreenEntityIdChanged)
    Q_PROPERTY(QStringList buttonIds READ buttonIds CONSTANT)
    Q_PROPERTY(QVariantMap buttons READ buttons NOTIFY webviewCfgChanged)

 public:
    QString fullscreenEntityId;
    // how often a web view closed: the website data setting applies then
    int              viewsClosed = 0;
    Q_INVOKABLE void viewClosed() { ++viewsClosed; }

    QStringList      buttonIds() const { return WebviewButtons::ofModel(s_model); }
    QVariantMap      buttons() const { return WebviewButtons::withDefaults(buttonIds(), m_configured); }
    Q_INVOKABLE bool buttonRepeats(const QVariantMap &action) const { return WebviewButtons::repeats(action); }

    // the global setting `buttons` as the core reports it (its changed buttons are enough here)
    void setButtons(const QVariantMap &configured) {
        m_configured = configured;
        emit webviewCfgChanged();
    }

    // the instance of the current engine
    static WebViewControllerStub *s_current;
    static QObject               *qmlInstance(QQmlEngine *, QJSEngine *) {
        s_current = new WebViewControllerStub;
        return s_current;
    }
    // the remote model of the next engine
    static HardwareModel::Enum s_model;

 signals:
    void fullscreenEntityIdChanged();
    void reloadRequested(QString entityId);
    void webviewCfgChanged();

 private:
    QVariantMap m_configured;
};

WebViewControllerStub *WebViewControllerStub::s_current = nullptr;
HardwareModel::Enum    WebViewControllerStub::s_model = HardwareModel::UCR3;

// Power (power.h): the reload interval of the page runs while the window is shown
class PowerStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool windowShown READ windowShown CONSTANT)

 public:
    bool windowShown() const { return true; }

    static QObject *qmlInstance(QQmlEngine *, QJSEngine *) { return new PowerStub; }
};

// `resource.getIcon()` of Components.Icon
class ResourceStub : public QObject {
    Q_OBJECT
 public:
    Q_INVOKABLE QString getIcon(const QString &, const QString &) { return QString(); }
};

// libucwebview with nothing behind it: the page creates its WebView item like on the device, no page is loaded.
namespace lib {

uc_webview *const kView = reinterpret_cast<uc_webview *>(0x1234);

uc_webview_callbacks callbacks{};
void                *user = nullptr;
int                  views = 0;  // views created
// "set_zoom(<zoom>)", "load_uri(<uri>)", "reload", "stop", "go_back", "go_forward", "pointer_button",
// "scroll(<dx>,<dy>)"
QStringList     calls;
QList<uint32_t> keys;  // keysyms forwarded to the page (uc_webview_key presses)
// every uc_webview_key() call: "<down|up> 0x<keysym> <keycode> <mods>"
QStringList keyEvents;
bool        canGoBack = false;
bool        canGoForward = false;
// the phases of every uc_webview_touch() call (UC_WEBVIEW_TOUCH_DOWN, ...)
QList<int> touches;
// uc_webview_when_rendered() requests (library 0.5.0) the page waits for, oldest first: the test ends them
QList<QPair<uc_webview_rendered_cb, void *>> renders;

int apiVersion() {
    return UC_WEBVIEW_API_VERSION;
}
int globalInit(void *, const char *, const char *) {
    return 0;
}
void        globalShutdown() {}
uc_webview *create(int, int, const uc_webview_callbacks *cb, void *u) {
    callbacks = *cb;
    user = u;
    views++;
    return kView;
}
uc_webview *createSized(int, int, const uc_webview_callbacks *cb, size_t size, void *u) {
    callbacks = uc_webview_callbacks{};
    memcpy(&callbacks, cb, std::min(size, sizeof callbacks));
    user = u;
    views++;
    return kView;
}
void destroy(uc_webview *) {
    // like uc_webview_free(): pending render requests end as cancelled inside the call
    const auto pending = std::exchange(renders, {});
    for (const auto &render : pending) {
        render.first(kView, UC_WEBVIEW_RENDERED_CANCELLED, render.second);
    }
}
int whenRendered(uc_webview *, uc_webview_rendered_cb cb, void *u) {
    renders.append(qMakePair(cb, u));
    return 0;
}
// the page rendered: the oldest request ends
void finishRender() {
    QVERIFY(!renders.isEmpty());
    const auto render = renders.takeFirst();
    render.first(kView, UC_WEBVIEW_RENDERED_DONE, render.second);
}
void resize(uc_webview *, int, int) {}
void setInt(uc_webview *, int) {}
void setDouble(uc_webview *, double) {}
void setString(uc_webview *, const char *) {}
void loadHtml(uc_webview *, const char *, const char *) {}
void loadUri(uc_webview *, const char *uri) {
    calls.append(QStringLiteral("load_uri(%1)").arg(uri));
}
void setZoom(uc_webview *, double zoom) {
    calls.append(QStringLiteral("set_zoom(%1)").arg(zoom));
}
int backInHistory(uc_webview *) {
    return canGoBack ? 1 : 0;
}
int forwardInHistory(uc_webview *) {
    return canGoForward ? 1 : 0;
}
void reload(uc_webview *) {
    calls.append(QStringLiteral("reload"));
}
void stop(uc_webview *) {
    calls.append(QStringLiteral("stop"));
}
void goBack(uc_webview *) {
    calls.append(QStringLiteral("go_back"));
}
void goForward(uc_webview *) {
    calls.append(QStringLiteral("go_forward"));
}
const char *none(uc_webview *) {
    return nullptr;
}
void setBackground(uc_webview *, float, float, float, float) {}
void frame(uc_webview *, void *) {}
void touch(uc_webview *, int, int phase, double, double, uint32_t) {
    touches.append(phase);
}
void pointerButton(uc_webview *, int, int pressed, double, double, unsigned, uint32_t) {
    if (pressed) {
        calls.append(QStringLiteral("pointer_button"));
    }
}
void pointerMove(uc_webview *, double, double, unsigned, uint32_t) {}
void scroll(uc_webview *, double dx, double dy, double, double, int, uint32_t) {
    calls.append(QStringLiteral("scroll(%1,%2)").arg(dx).arg(dy));
}
void key(uc_webview *, uint32_t keysym, uint32_t keycode, int pressed, unsigned mods, uint32_t) {
    if (pressed) {
        keys.append(keysym);
    }
    keyEvents.append(
        QStringLiteral("%1 0x%2 %3 %4").arg(pressed ? "down" : "up").arg(keysym, 0, 16).arg(keycode).arg(mods));
}
void imePreedit(uc_webview *, const char *, int) {}
void imeDeleteSurrounding(uc_webview *, int, int) {}
int  setMagnification(uc_webview *, double, double, double) {
    return 0;
}
double getMagnification(uc_webview *) {
    return 1.0;
}
int requestReply(uc_webview *, uint64_t, int, const char *) {
    return 0;
}
int requestAuth(uc_webview *, uint64_t, const char *, const char *) {
    return 0;
}

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
    f.load_uri = &loadUri;
    f.load_html = &loadHtml;
    f.reload = &reload;
    f.stop = &stop;
    f.go_back = &goBack;
    f.go_forward = &goForward;
    f.can_go_back = &backInHistory;
    f.can_go_forward = &forwardInHistory;
    f.get_uri = &none;
    f.get_title = &none;
    f.run_javascript = &setString;
    f.set_zoom = &setZoom;
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
    f.when_rendered = &whenRendered;
    f.create_sized = &createSized;
    f.set_magnification = &setMagnification;
    f.get_magnification = &getMagnification;
    f.request_reply = &requestReply;
    f.request_auth = &requestAuth;
    return f;
}

// the page's load events, progress and frames as the library reports them
void fireLoad(int event, const char *uri) {
    callbacks.load_changed(kView, event, uri, user);
}
void fireProgress(double progress) {
    callbacks.progress_changed(kView, progress, user);
}
void fireFrame() {
    static intptr_t  handle = 0;
    uc_webview_frame frame{};
    frame.width = 480;
    frame.height = 800;
    frame.handle = reinterpret_cast<void *>(++handle);
    callbacks.frame_ready(kView, &frame, user);
}
// a page that loads at once: started, committed, shown and finished before the load indicator's delay
void loadQuickly(const char *uri) {
    fireLoad(UC_WEBVIEW_LOAD_STARTED, uri);
    fireLoad(UC_WEBVIEW_LOAD_COMMITTED, uri);
    fireFrame();
    fireLoad(UC_WEBVIEW_LOAD_FINISHED, uri);
}
// why the next load fails (library 0.10.0), right before its FAILED event
void fireFailDetail(const char *uri, int reason, const char *domain, int code, unsigned tlsErrors,
                    const char *message) {
    QVERIFY(callbacks.load_failed_detail != nullptr);
    callbacks.load_failed_detail(kView, uri, reason, domain, code, tlsErrors, message, user);
}
void fireTerminated(int reason) {
    callbacks.web_process_terminated(kView, reason, user);
}
// a load that fails, and WebKit's error page that follows for the same address
void fail(const char *uri) {
    fireLoad(UC_WEBVIEW_LOAD_STARTED, uri);
    fireLoad(UC_WEBVIEW_LOAD_FAILED, uri);
    fireLoad(UC_WEBVIEW_LOAD_FINISHED, uri);
    loadQuickly(uri);
}

// an alert() of the page as the library reports it (library 0.7.0)
void fireAlert(uint64_t id) {
    QVERIFY(callbacks.script_dialog != nullptr);
    callbacks.script_dialog(kView, id, UC_WEBVIEW_DIALOG_ALERT, "https://a.example", "Hello", nullptr, user);
}

}  // namespace lib

// the zoom calls of the library, in order
QStringList zoomCalls() {
    return lib::calls.filter(QStringLiteral("set_zoom("));
}

int callCount(const char *call) {
    return lib::calls.count(QString::fromLatin1(call));
}

}  // namespace

class testWebViewEntityScreen : public QObject {
    Q_OBJECT

 private slots:
    void init();
    void cleanup();

    void volumeZoomsTheFullscreenWebView();
    void volumeZoomsOnTopOfAnActivityAndTheOtherButtonsKeepItsMapping();
    void zoomStepsFromTheEntitysZoom_data();
    void zoomStepsFromTheEntitysZoom();
    void heldVolumeZoomsOneLevel();
    void keyZoomLastsForTheVisitAndLeavesTheEntityAsItIs();
    void anotherWebViewStartsAtItsZoom();
    void anotherWebViewReplacesThePageLikeACloseAndAnOpen();
    void zoomLabelShowsTheLevel();
    void volumeDoesNothingWhileADialogOrTheKeyboardIsUp();
    void screenIsOpaqueAndThePageShowsItsBackgroundBeforeTheFirstFrame_data();
    void passKeepsTheFunctionOfTheLayerBelow_data();
    void passKeepsTheFunctionOfTheLayerBelow();
    void passOnAProfilePageWithoutAnActivityDoesNothing();
    void powerFromAProfilePageOpensTheTurnOffMenuOverTheWebView();
    void aChangeOfTheButtonSettingAppliesAtOnce();
    void channelPassKeepsTheActivitysMappingAndItsRepeat();
    void channelPassFromAProfilePageGoesToThePageHeader();
    void defaultsSendTheDpadKeysAndPassTheOtherButtons();
    void heldButtonsRepeatKeysAndScrollingOnly();
    void theWebViewsButtonsGoBeforeTheGlobalSetting();
    void functionsZoomScrollAndGoThroughTheHistory();
    void functionsCloseToggleTheNavigationMoveTheFocusOrDoNothing();
    void backWithAnotherActionKeepsTheWebViewOpen();
    void buttonsOfTheModel_data();
    void buttonsOfTheModel();
    void whileTypingTheDpadAndOkGoToTheTextField();
    void aDialogOfThePageOwnsTheButtons();
    void buttonsKeepTheirActionsWhileTheGarbageCollectorRuns();
    void screenIsOpaqueAndThePageShowsItsBackgroundBeforeTheFirstFrame();
    void spinnerAfterTheDelayUntilTheFirstFrameThenTheBar();
    void reloadsShowTheBarOrNothingAndBackTheSpinner();
    void failedLoadShowsTheMessageAndOkTriesAgain();
    void indicatorLetsTouchesAndKeysThroughToThePage();
    void failureSaysWhy_data();
    void failureSaysWhy();
    void detailsShowTheTechnicalErrorAndClose();
    void failureOfAnOlderLibraryHasNoReasonAndNoDetails();
    void widgetShowsTheIndicatorAndTheFailure_data();
    void widgetShowsTheIndicatorAndTheFailure();
    void swipeOnAnInteractiveWidgetDoesNotSwitchTheActivityPage_data();
    void swipeOnAnInteractiveWidgetDoesNotSwitchTheActivityPage();

 private:
    Webview     *createEntity(const QVariantMap &options);
    QQuickItem  *openScreen(bool onActivity);
    WebViewItem *webView() const;
    // a dialog or login of the page is open (Webview.qml requestShown())
    bool requestShown() const;
    bool pressRemoteButton(Qt::Key key, int repeats = 0);
    // how often the mapping of a key ran on the activity screen below ("activityPresses") or in the main screen's page
    // header ("mainScreenPresses")
    int presses(const char *where, const char *key) const;
    // the global button setting, as the core reports it
    void setButtons(const char *json) const;
    // the key events the page got since the last call: "<down|up> 0x<keysym> <keycode> <mods>"
    QStringList takeKeyEvents() const;
    // VOLUME up or down and the render of the step, which the item waits for before the next level
    void zoomKey(Qt::Key key);
    // a part of the load indicator in the screen or the widget (WebViewLoadIndicator.qml)
    QQuickItem *part(QQuickItem *in, const char *name) const;
    void        click(QQuickItem *item);

    QQuickView          *view = nullptr;
    UiStub              *ui = nullptr;
    ResourceStub         resource;
    Colors               colors;
    Fonts                fonts;
    QQuickItem          *host = nullptr;
    QPointer<QQuickItem> screen;
    Webview             *entity = nullptr;
};

void testWebViewEntityScreen::init() {
    ui = new UiStub(this);

    view = new QQuickView;
    view->rootContext()->setContextProperty("ui", ui);
    view->rootContext()->setContextProperty("colors", &colors);
    view->rootContext()->setContextProperty("fonts", &fonts);
    view->rootContext()->setContextProperty("resource", &resource);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->resize(480, 850);
    view->setSource(QUrl("qrc:/test/EntityScreenHost.qml"));
    QStringList errors;
    for (const QQmlError &error : view->errors()) {
        errors.append(error.toString());
    }
    QVERIFY2(view->status() == QQuickView::Ready, qPrintable(errors.join("\n")));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    host = view->rootObject();

    lib::calls.clear();
    lib::keys.clear();
    lib::keyEvents.clear();
    lib::renders.clear();
    lib::touches.clear();
    lib::views = 0;
    lib::canGoBack = false;
    lib::canGoForward = false;

    // both paths of a key press (docs/key-navigation.md section 1), like main.qml's
    // ui.inputController.setSource(applicationWindow)
    ui->input()->setSource(view);
}

void testWebViewEntityScreen::cleanup() {
    delete view;
    view = nullptr;
    screen = nullptr;
    EntityControllerStub::s_entity = nullptr;
    delete entity;
    entity = nullptr;
    delete ui;
    ui = nullptr;
    WebViewControllerStub::s_model = HardwareModel::UCR3;
}

Webview *testWebViewEntityScreen::createEntity(const QVariantMap &options) {
    return new Webview(kEntityId, QVariantMap{{QStringLiteral("en"), QStringLiteral("Dashboard")}},
                       QStringLiteral("en"), QString(), QString(), true,
                       QVariantMap{{QStringLiteral("state"), QStringLiteral("ON")}}, options,
                       QStringLiteral("uc_web.main"), nullptr);
}

QQuickItem *testWebViewEntityScreen::openScreen(bool onActivity) {
    QVariant item;
    if (!QMetaObject::invokeMethod(host, "openScreen", Q_RETURN_ARG(QVariant, item),
                                   Q_ARG(QVariant, QVariant::fromValue<QObject *>(entity)),
                                   Q_ARG(QVariant, onActivity))) {
        return nullptr;
    }
    screen = qobject_cast<QQuickItem *>(item.value<QObject *>());
    return screen;
}

WebViewItem *testWebViewEntityScreen::webView() const {
    return screen ? qobject_cast<WebViewItem *>(screen->property("webView").value<QObject *>()) : nullptr;
}

bool testWebViewEntityScreen::requestShown() const {
    QVariant shown;
    return screen && QMetaObject::invokeMethod(screen, "requestShown", Q_RETURN_ARG(QVariant, shown)) && shown.toBool();
}

// A button of the remote as the device delivers it to the window: no text, a held button repeats presses flagged
// autoRepeat and releases once. The input controller (event filter, path 1) and the focused item (path 2) see it.
// Returns whether an item accepted the press.
bool testWebViewEntityScreen::pressRemoteButton(Qt::Key key, int repeats) {
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &press);
    for (int i = 0; i < repeats; ++i) {
        QKeyEvent repeat(QEvent::KeyPress, key, Qt::NoModifier, QString(), true);
        QCoreApplication::sendEvent(view, &repeat);
    }
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &release);
    return press.isAccepted();
}

int testWebViewEntityScreen::presses(const char *where, const char *key) const {
    QVariant n;
    QMetaObject::invokeMethod(host, where, Q_RETURN_ARG(QVariant, n), Q_ARG(QVariant, QString::fromLatin1(key)));
    return n.toInt();
}

void testWebViewEntityScreen::setButtons(const char *json) const {
    QVERIFY(WebViewControllerStub::s_current);
    WebViewControllerStub::s_current->setButtons(QJsonDocument::fromJson(json).toVariant().toMap());
}

QStringList testWebViewEntityScreen::takeKeyEvents() const {
    return std::exchange(lib::keyEvents, {});
}

void testWebViewEntityScreen::zoomKey(Qt::Key key) {
    const int renders = lib::renders.size();
    pressRemoteButton(key);
    if (lib::renders.size() > renders) {
        lib::finishRender();
    }
}

void testWebViewEntityScreen::volumeZoomsTheFullscreenWebView() {
    // a profile page tile: nothing below whose mapping the screen keeps
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());
    QCOMPARE(ui->input()->activeItem(), screen);
    QCOMPARE(webView()->zoom(), 1.0);

    // VOLUME+ one level in, VOLUME- back out: the page gets the level, not the key
    QVERIFY(!pressRemoteButton(Qt::Key_VolumeUp));
    QCOMPARE(webView()->zoom(), 1.1);
    QCOMPARE(zoomCalls(), QStringList{QStringLiteral("set_zoom(1.1)")});
    lib::finishRender();
    QVERIFY(!pressRemoteButton(Qt::Key_VolumeDown));
    QCOMPARE(webView()->zoom(), 1.0);
    QCOMPARE(zoomCalls(), (QStringList{QStringLiteral("set_zoom(1.1)"), QStringLiteral("set_zoom(1)")}));
    lib::finishRender();
    QVERIFY(lib::keys.isEmpty());
    // the d-pad stays in the page
    QVERIFY(webView()->hasActiveFocus());
}

void testWebViewEntityScreen::volumeZoomsOnTopOfAnActivityAndTheOtherButtonsKeepItsMapping() {
    // opened by the `open` command or a button of an activity: on top of its screen
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());

    // VOLUME zooms the page; the receiver's volume of the activity's mapping stays as it is
    zoomKey(Qt::Key_VolumeUp);
    zoomKey(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.25);
    zoomKey(Qt::Key_VolumeDown);
    QCOMPARE(webView()->zoom(), 1.1);
    QVariant presses;
    for (const char *key : {"VOLUME_UP", "VOLUME_DOWN"}) {
        QVERIFY(QMetaObject::invokeMethod(host, "activityPresses", Q_RETURN_ARG(QVariant, presses),
                                          Q_ARG(QVariant, QString::fromLatin1(key))));
        QVERIFY2(presses.toInt() == 0, key);
    }

    // MUTE keeps the activity's mapping
    pressRemoteButton(Qt::Key_VolumeMute);
    QVERIFY(QMetaObject::invokeMethod(host, "activityPresses", Q_RETURN_ARG(QVariant, presses),
                                      Q_ARG(QVariant, QStringLiteral("MUTE"))));
    QCOMPARE(presses.toInt(), 1);

    // CHANNEL scrolls the page and the d-pad goes to it, as before: the activity's mapping of these does not run
    pressRemoteButton(Qt::Key_ChannelDown);
    pressRemoteButton(Qt::Key_ChannelUp);
    QCOMPARE(lib::keys, (QList<uint32_t>{0xff56, 0xff55}));
    QTest::keyClick(view, Qt::Key_Up);
    QCOMPARE(lib::keys.size(), 3);
    for (const char *key : {"CHANNEL_UP", "DPAD_UP", "BACK"}) {
        QVERIFY(QMetaObject::invokeMethod(host, "activityPresses", Q_RETURN_ARG(QVariant, presses),
                                          Q_ARG(QVariant, QString::fromLatin1(key))));
        QVERIFY2(presses.toInt() == 0, key);
    }

    // BACK closes the web view, not a function of the activity
    pressRemoteButton(Qt::Key_Exit);
    QTRY_COMPARE(screen->property("state").toString(), QStringLiteral("closed"));
    QVERIFY(QMetaObject::invokeMethod(host, "activityPresses", Q_RETURN_ARG(QVariant, presses),
                                      Q_ARG(QVariant, QStringLiteral("BACK"))));
    QCOMPARE(presses.toInt(), 0);
}

void testWebViewEntityScreen::zoomStepsFromTheEntitysZoom_data() {
    QTest::addColumn<QVariantMap>("options");
    QTest::addColumn<qreal>("start");
    QTest::addColumn<qreal>("in");
    QTest::addColumn<qreal>("out");

    // the WebView item's levels: 50, 67, 75, 80, 90, 100, 110, 125, 150, 175, 200, 250, 300 %
    QTest::newRow("100 %") << QVariantMap{{"url", "https://a.example/"}} << 1.0 << 1.1 << 0.9;
    // between two levels: the next level in the key's direction
    QTest::newRow("60 %") << QVariantMap{{"url", "https://a.example/"}, {"zoom", 60}} << 0.6 << 0.67 << 0.5;
    // the entity's range is 25 to 400 %: beyond the last level a key in that direction does nothing
    QTest::newRow("400 %") << QVariantMap{{"url", "https://a.example/"}, {"zoom", 400}} << 4.0 << 4.0 << 3.0;
    QTest::newRow("25 %") << QVariantMap{{"url", "https://a.example/"}, {"zoom", 25}} << 0.25 << 0.5 << 0.25;
    // the user override wins over the option
    QTest::newRow("user override 150 %") << QVariantMap{{"url", "https://a.example/"},
                                                        {"zoom", 70},
                                                        {"user", QVariantMap{{"zoom", 150}}}}
                                         << 1.5 << 1.75 << 1.25;
}

void testWebViewEntityScreen::zoomStepsFromTheEntitysZoom() {
    QFETCH(QVariantMap, options);
    QFETCH(qreal, start);
    QFETCH(qreal, in);
    QFETCH(qreal, out);

    entity = createEntity(options);
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QCOMPARE(webView()->zoom(), start);

    zoomKey(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), in);
    // back to the start, then one level out
    webView()->setZoom(start);
    if (!lib::renders.isEmpty()) {
        lib::finishRender();
    }
    zoomKey(Qt::Key_VolumeDown);
    QCOMPARE(webView()->zoom(), out);
}

void testWebViewEntityScreen::heldVolumeZoomsOneLevel() {
    // a held button repeats every 150 ms on the device: one level per press, every level is a relayout
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    pressRemoteButton(Qt::Key_VolumeUp, 5);
    QCOMPARE(webView()->zoom(), 1.1);
    lib::finishRender();
    pressRemoteButton(Qt::Key_VolumeDown, 5);
    QCOMPARE(webView()->zoom(), 1.0);
    QCOMPARE(zoomCalls(), (QStringList{QStringLiteral("set_zoom(1.1)"), QStringLiteral("set_zoom(1)")}));
}

void testWebViewEntityScreen::keyZoomLastsForTheVisitAndLeavesTheEntityAsItIs() {
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}, {"zoom", 80}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QCOMPARE(webView()->zoom(), 0.8);

    zoomKey(Qt::Key_VolumeUp);
    zoomKey(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.0);
    // the entity's options are not written: its zoom is the option's
    QCOMPARE(entity->getZoom(), 80);

    // an options update that leaves the zoom as it is (the fetch when the screen opened, another option) keeps the
    // level
    lib::calls.clear();
    entity->updateOptions(QVariantMap{{"url", "https://a.example/"}, {"zoom", 80}, {"reload_interval", 60}});
    QCoreApplication::processEvents();
    QCOMPARE(webView()->zoom(), 1.0);
    QVERIFY(zoomCalls().isEmpty());

    // a new zoom option applies, from there the keys step again
    entity->updateOptions(QVariantMap{{"url", "https://a.example/"}, {"zoom", 150}});
    QCOMPARE(webView()->zoom(), 1.5);
    if (!lib::renders.isEmpty()) {
        lib::finishRender();
    }
    zoomKey(Qt::Key_VolumeDown);
    QCOMPARE(webView()->zoom(), 1.25);

    // the next visit starts at the entity's zoom
    pressRemoteButton(Qt::Key_Exit);
    QTRY_COMPARE(screen->property("state").toString(), QStringLiteral("closed"));
    QVERIFY(QMetaObject::invokeMethod(host, "closeScreen"));
    QTRY_VERIFY(screen.isNull());
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QCOMPARE(webView()->zoom(), 1.5);
}

void testWebViewEntityScreen::anotherWebViewStartsAtItsZoom() {
    // the `open` command of another web view while one is shown: the screen stays and shows the other page
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    zoomKey(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.1);

    // the same zoom option, 100 %: still the other page's zoom, not the level of the keys
    QScopedPointer<Webview> other(new Webview(
        QStringLiteral("uc_web.main.2"), QVariantMap{{QStringLiteral("en"), QStringLiteral("Status")}},
        QStringLiteral("en"), QString(), QString(), true, QVariantMap{{QStringLiteral("state"), QStringLiteral("ON")}},
        QVariantMap{{"url", "https://b.example/"}}, QStringLiteral("uc_web.main"), nullptr));
    QVERIFY(
        QMetaObject::invokeMethod(screen, "showEntity", Q_ARG(QVariant, QVariant::fromValue<QObject *>(other.data()))));
    QTRY_COMPARE(webView()->zoom(), 1.0);
    zoomKey(Qt::Key_VolumeDown);
    QCOMPARE(webView()->zoom(), 0.9);
    QCOMPARE(other->getZoom(), 100);
    QVERIFY(QMetaObject::invokeMethod(host, "closeScreen"));
    QTRY_VERIFY(screen.isNull());
}

void testWebViewEntityScreen::anotherWebViewReplacesThePageLikeACloseAndAnOpen() {
    // The `open` command of another web view while one is shown: like closing it and opening the other one (core-api
    // entity_webview.md "Open"). Regression: the other page was loaded into the same WebView, so BACK went back to the
    // first web view's page, and the website data setting of a closed web view was not applied.
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    QPointer<WebViewItem> first = webView();
    QTRY_VERIFY(lib::calls.contains(QStringLiteral("load_uri(https://a.example/)")));
    lib::loadQuickly("https://a.example/");
    lib::canGoBack = true;
    lib::loadQuickly("https://a.example/next");
    QVERIFY(first->canGoBack());
    QCOMPARE(lib::views, 1);
    WebViewControllerStub *controller = WebViewControllerStub::s_current;
    QVERIFY(controller);
    QCOMPARE(controller->viewsClosed, 0);

    QScopedPointer<Webview> other(new Webview(
        QStringLiteral("uc_web.main.2"), QVariantMap{{QStringLiteral("en"), QStringLiteral("Status")}},
        QStringLiteral("en"), QString(), QString(), true, QVariantMap{{QStringLiteral("state"), QStringLiteral("ON")}},
        QVariantMap{{"url", "https://b.example/"}}, QStringLiteral("uc_web.main"), nullptr));
    // the other page has a history of its own: none yet
    lib::canGoBack = false;
    lib::calls.clear();
    QVERIFY(
        QMetaObject::invokeMethod(screen, "showEntity", Q_ARG(QVariant, QVariant::fromValue<QObject *>(other.data()))));
    // the website data setting applied to the replaced web view, its page went and a new one has the other web view
    QCOMPARE(controller->viewsClosed, 1);
    QCOMPARE(controller->fullscreenEntityId, QStringLiteral("uc_web.main.2"));
    QTRY_VERIFY(first.isNull());
    QVERIFY(webView() != nullptr);
    QTRY_COMPARE(lib::views, 2);
    QTRY_VERIFY(lib::calls.contains(QStringLiteral("load_uri(https://b.example/)")));
    // the replaced page never loaded the other web view
    QTest::qWait(50);
    QCOMPARE(lib::calls.count(QStringLiteral("load_uri(https://b.example/)")), 1);
    QVERIFY(!lib::calls.contains(QStringLiteral("load_uri(https://a.example/)")));
    lib::loadQuickly("https://b.example/");
    QVERIFY(!webView()->canGoBack());
    QTRY_VERIFY(webView()->hasActiveFocus());

    // back_or_close on the other web view's first page closes it, as after an open: no way back to the first one
    setButtons(R"({"BACK": {"action": "function", "function": "back_or_close"}})");
    lib::calls.clear();
    // the same web view again: it stays as it is, no new page, no reload, the website data stays
    QVERIFY(
        QMetaObject::invokeMethod(screen, "showEntity", Q_ARG(QVariant, QVariant::fromValue<QObject *>(other.data()))));
    QTest::qWait(50);
    QCOMPARE(lib::views, 2);
    QVERIFY(lib::calls.isEmpty());
    QCOMPARE(controller->viewsClosed, 1);
    pressRemoteButton(Qt::Key_Exit);
    QVERIFY(!lib::calls.contains(QStringLiteral("go_back")));
    QTRY_COMPARE(screen->property("state").toString(), QStringLiteral("closed"));
}

void testWebViewEntityScreen::zoomLabelShowsTheLevel() {
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QQuickItem *label = screen->findChild<QQuickItem *>("zoomLabel");
    QObject    *text = screen->findChild<QObject *>("zoomText");
    QVERIFY(label && text);
    QVERIFY(!label->isVisible());

    // the new level at once; the page renders it quickly: the label fades about a second after the press
    pressRemoteButton(Qt::Key_VolumeUp);
    QTRY_VERIFY(label->isVisible());
    QCOMPARE(text->property("text").toString(), QStringLiteral("110 %"));
    lib::finishRender();
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 3000);

    // at the last level a press zooms nothing, the label shows that the zoom ends there
    webView()->setZoom(3.0);
    lib::finishRender();
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 3000);
    pressRemoteButton(Qt::Key_VolumeUp);
    QVERIFY(lib::renders.isEmpty());
    QTRY_VERIFY(label->isVisible());
    QCOMPARE(text->property("text").toString(), QStringLiteral("300 %"));
}

void testWebViewEntityScreen::volumeDoesNothingWhileADialogOrTheKeyboardIsUp() {
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());
    QVariant presses;

    // the on-screen keyboard is up on a field of the page: a relayout would move the field
    QObject *keyboard = host->findChild<QObject *>("keyboard");
    QVERIFY(keyboard);
    keyboard->setProperty("active", true);
    pressRemoteButton(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.0);
    keyboard->setProperty("active", false);

    // an alert() of the page owns the keys
    lib::fireAlert(7);
    QTRY_VERIFY(requestShown());
    pressRemoteButton(Qt::Key_VolumeUp);
    pressRemoteButton(Qt::Key_VolumeDown);
    QCOMPARE(webView()->zoom(), 1.0);
    QVERIFY(zoomCalls().isEmpty());
    // nor does the activity get them
    QVERIFY(QMetaObject::invokeMethod(host, "activityPresses", Q_RETURN_ARG(QVariant, presses),
                                      Q_ARG(QVariant, QStringLiteral("VOLUME_UP"))));
    QCOMPARE(presses.toInt(), 0);
}

void testWebViewEntityScreen::screenIsOpaqueAndThePageShowsItsBackgroundBeforeTheFirstFrame_data() {
    QTest::addColumn<bool>("onActivity");
    QTest::addColumn<QVariantMap>("options");
    QTest::addColumn<QColor>("pageBackground");

    // white without the option (core-api "Display"), else the option
    QTest::newRow("profile page") << false << QVariantMap{{"url", "https://a.example/"}} << QColor(Qt::white);
    QTest::newRow("on top of an activity")
        << true << QVariantMap{{"url", "https://a.example/"}, {"background_color", "#102030"}} << QColor("#102030");
}

void testWebViewEntityScreen::screenIsOpaqueAndThePageShowsItsBackgroundBeforeTheFirstFrame() {
    QFETCH(bool, onActivity);
    QFETCH(QVariantMap, options);
    QFETCH(QColor, pageBackground);

    entity = createEntity(options);
    QVERIFY(openScreen(onActivity));
    QTRY_VERIFY(webView() != nullptr);

    // the screen covers the whole window, opaque: nothing of the screen below shows through
    const QColor color = screen->property("color").value<QColor>();
    QCOMPARE(color.alphaF(), 1.0);
    QCOMPARE(color, colors.property("bg").value<QColor>());
    QCOMPARE(screen->opacity(), 1.0);
    QTRY_COMPARE(screen->mapRectToScene(QRectF(0, 0, screen->width(), screen->height())),
                 QRectF(0, 0, view->width(), view->height()));

    // no frame has come: the page shows its background, not the screen's black
    QQuickItem *background = screen->findChild<QQuickItem *>("webViewBackground");
    QVERIFY(background);
    QVERIFY(background->isVisible());
    QVERIFY(background->z() < 0);
    QCOMPARE(background->property("color").value<QColor>(), pageBackground);
    QCOMPARE(background->mapRectToScene(QRectF(0, 0, background->width(), background->height())),
             webView()->mapRectToScene(QRectF(0, 0, webView()->width(), webView()->height())));
}

void testWebViewEntityScreen::passKeepsTheFunctionOfTheLayerBelow_data() {
    QTest::addColumn<bool>("onActivity");
    QTest::newRow("on top of an activity") << true;
    QTest::newRow("profile page") << false;
}

void testWebViewEntityScreen::passKeepsTheFunctionOfTheLayerBelow() {
    // VOLUME with `pass`: what it does on the screen below, the activity's mapping, or on the main screen, the volume
    // of the page header's activity or media player
    QFETCH(bool, onActivity);
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(onActivity));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"VOLUME_UP": {"action": "pass"}, "VOLUME_DOWN": {"action": "pass"}})");

    const char *const below = onActivity ? "activityPresses" : "mainScreenPresses";
    const char *const other = onActivity ? "mainScreenPresses" : "activityPresses";
    pressRemoteButton(Qt::Key_VolumeUp);
    pressRemoteButton(Qt::Key_VolumeDown);
    QCOMPARE(presses(below, "VOLUME_UP"), 1);
    QCOMPARE(presses(below, "VOLUME_DOWN"), 1);
    // held: every repeat changes the volume, as without the web view
    pressRemoteButton(Qt::Key_VolumeUp, 3);
    QCOMPARE(presses(below, "VOLUME_UP"), 5);
    QCOMPARE(presses(other, "VOLUME_UP"), 0);

    // the page is not zoomed and no label shows
    QCOMPARE(webView()->zoom(), 1.0);
    QVERIFY(zoomCalls().isEmpty());
    QQuickItem *label = screen->findChild<QQuickItem *>("zoomLabel");
    QVERIFY(label);
    QTest::qWait(50);
    QVERIFY(!label->isVisible());

    // CHANNEL and the d-pad stay the web view's
    pressRemoteButton(Qt::Key_ChannelDown);
    QCOMPARE(lib::keys, QList<uint32_t>{0xff56});
}

void testWebViewEntityScreen::passOnAProfilePageWithoutAnActivityDoesNothing() {
    // the main screen does nothing with VOLUME when its page header shows no activity or media player
    host->setProperty("headerHasActivity", false);
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"VOLUME_UP": {"action": "pass"}})");
    pressRemoteButton(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.0);
    QVERIFY(zoomCalls().isEmpty());
    QCOMPARE(presses("mainScreenPresses", "VOLUME_UP"), 0);
    QVERIFY(lib::keyEvents.isEmpty());
    QCOMPARE(screen->property("state").toString(), QStringLiteral("open"));
}

void testWebViewEntityScreen::powerFromAProfilePageOpensTheTurnOffMenuOverTheWebView() {
    // POWER keeps its function (core-api entity_webview.md "Buttons"): on a profile page the "Turn off" menu of the
    // page header's activities. The main screen's menu is hidden behind the web view: the screen hands its own to the
    // header. Regression: POWER did nothing in a web view opened from a profile page.
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QObject *menu = screen->findChild<QObject *>(QStringLiteral("turnOffMenu"));
    QVERIFY(menu);

    pressRemoteButton(Qt::Key_PowerOff);
    QCOMPARE(presses("mainScreenPresses", "POWER"), 1);
    QCOMPARE(host->property("powerMenu").value<QObject *>(), menu);
    QVERIFY(lib::keyEvents.isEmpty());
    QCOMPARE(screen->property("state").toString(), QStringLiteral("open"));

    // without an activity in the header: nothing, as on the main screen
    host->setProperty("headerHasActivity", false);
    pressRemoteButton(Qt::Key_PowerOff);
    QCOMPARE(presses("mainScreenPresses", "POWER"), 1);

    // on top of an activity: the activity's POWER, not the main screen's
    host->setProperty("headerHasActivity", true);
    QVERIFY(QMetaObject::invokeMethod(host, "closeScreen"));
    QTRY_VERIFY(screen.isNull());
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    pressRemoteButton(Qt::Key_PowerOff);
    QCOMPARE(presses("activityPresses", "POWER"), 1);
    QCOMPARE(presses("mainScreenPresses", "POWER"), 1);
}

void testWebViewEntityScreen::aChangeOfTheButtonSettingAppliesAtOnce() {
    // also while the web view is open: the next press follows the setting
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    zoomKey(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.1);
    pressRemoteButton(Qt::Key_ChannelDown);
    QCOMPARE(lib::keys, QList<uint32_t>{0xff56});

    setButtons(R"({"VOLUME_UP": {"action": "pass"}, "CHANNEL_DOWN": {"action": "pass"}})");
    pressRemoteButton(Qt::Key_VolumeUp);
    pressRemoteButton(Qt::Key_ChannelDown);
    QCOMPARE(webView()->zoom(), 1.1);
    QCOMPARE(lib::keys, QList<uint32_t>{0xff56});
    QCOMPARE(presses("activityPresses", "VOLUME_UP"), 1);
    QCOMPARE(presses("activityPresses", "CHANNEL_DOWN"), 1);

    // back to the defaults: VOLUME zooms on from the level of the visit, CHANNEL scrolls
    setButtons("{}");
    zoomKey(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.25);
    pressRemoteButton(Qt::Key_ChannelUp);
    QCOMPARE(lib::keys, (QList<uint32_t>{0xff56, 0xff55}));
    QCOMPARE(presses("activityPresses", "VOLUME_UP"), 1);
    QCOMPARE(presses("activityPresses", "CHANNEL_UP"), 0);
}

void testWebViewEntityScreen::channelPassKeepsTheActivitysMappingAndItsRepeat() {
    // CHANNEL with `pass`: what the activity maps, the page is not scrolled; VOLUME still zooms
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"CHANNEL_UP": {"action": "pass"}, "CHANNEL_DOWN": {"action": "pass"}})");

    pressRemoteButton(Qt::Key_ChannelUp);
    QCOMPARE(presses("activityPresses", "CHANNEL_UP"), 1);
    // held: the activity's mapping repeats as on the activity screen (it declares no repeat of its own)
    pressRemoteButton(Qt::Key_ChannelDown, 3);
    QCOMPARE(presses("activityPresses", "CHANNEL_DOWN"), 4);
    QVERIFY(lib::keys.isEmpty());

    zoomKey(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.1);
    QCOMPARE(presses("activityPresses", "VOLUME_UP"), 0);
}

void testWebViewEntityScreen::channelPassFromAProfilePageGoesToThePageHeader() {
    // opened from a profile page: what the page header's activity or media player does with CHANNEL, nothing today
    // (Page.qml maps no CHANNEL there); the page is not scrolled either way
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"CHANNEL_UP": {"action": "pass"}, "CHANNEL_DOWN": {"action": "pass"}})");

    pressRemoteButton(Qt::Key_ChannelUp);
    pressRemoteButton(Qt::Key_ChannelDown);
    QVERIFY(lib::keys.isEmpty());
    QCOMPARE(presses("mainScreenPresses", "CHANNEL_UP"), 0);

    // a header that maps CHANNEL gets it, at every press and repeat
    host->setProperty("headerMapsChannel", true);
    pressRemoteButton(Qt::Key_ChannelUp);
    pressRemoteButton(Qt::Key_ChannelDown, 2);
    QCOMPARE(presses("mainScreenPresses", "CHANNEL_UP"), 1);
    QCOMPARE(presses("mainScreenPresses", "CHANNEL_DOWN"), 3);
    QVERIFY(lib::keys.isEmpty());

    // without an activity or media player in the header: nothing
    host->setProperty("headerHasActivity", false);
    pressRemoteButton(Qt::Key_ChannelUp);
    QCOMPARE(presses("mainScreenPresses", "CHANNEL_UP"), 1);
    QVERIFY(lib::keys.isEmpty());
}

void testWebViewEntityScreen::defaultsSendTheDpadKeysAndPassTheOtherButtons() {
    // Remote 3: the d-pad sends the arrow keys and OK Enter, from the screen's button mapping with the keycodes of a
    // keyboard (KeyboardEvent.code), once per press: the WebView does not forward them on the focus chain as well
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());
    pressRemoteButton(Qt::Key_Up);
    pressRemoteButton(Qt::Key_Down);
    pressRemoteButton(Qt::Key_Left);
    pressRemoteButton(Qt::Key_Right);
    pressRemoteButton(Qt::Key_Return);
    QCOMPARE(takeKeyEvents(),
             (QStringList{"down 0xff52 111 0", "up 0xff52 111 0", "down 0xff54 116 0", "up 0xff54 116 0",
                          "down 0xff51 113 0", "up 0xff51 113 0", "down 0xff53 114 0", "up 0xff53 114 0",
                          "down 0xff0d 36 0", "up 0xff0d 36 0"}));
    // MENU sends ContextMenu: the PC menu key
    pressRemoteButton(Qt::Key_F4);
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff67 135 0", "up 0xff67 135 0"}));
    QCOMPARE(presses("activityPresses", "DPAD_UP"), 0);
    QCOMPARE(presses("activityPresses", "MENU"), 0);

    // PLAY and MUTE pass: the activity's mapping; POWER keeps it, a fixed function
    pressRemoteButton(Qt::Key_MediaTogglePlayPause);
    pressRemoteButton(Qt::Key_VolumeMute);
    pressRemoteButton(Qt::Key_PowerOff);
    QCOMPARE(presses("activityPresses", "PLAY"), 1);
    QCOMPARE(presses("activityPresses", "MUTE"), 1);
    QCOMPARE(presses("activityPresses", "POWER"), 1);
    QVERIFY(takeKeyEvents().isEmpty());
    QVERIFY(webView()->hasActiveFocus());

    // a profile page: PLAY goes to the page header's media player, as on the page itself
    QVERIFY(QMetaObject::invokeMethod(host, "closeScreen"));
    QTRY_VERIFY(screen.isNull());
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    pressRemoteButton(Qt::Key_MediaTogglePlayPause);
    QCOMPARE(presses("mainScreenPresses", "PLAY"), 1);
    QCOMPARE(presses("activityPresses", "PLAY"), 1);
}

void testWebViewEntityScreen::heldButtonsRepeatKeysAndScrollingOnly() {
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);

    // a key: further presses while held, one release (WebKit flags the repeats)
    pressRemoteButton(Qt::Key_Down, 3);
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff54 116 0", "down 0xff54 116 0", "down 0xff54 116 0",
                                           "down 0xff54 116 0", "up 0xff54 116 0"}));
    // page_down (CHANNEL down) scrolls on with every repeat
    pressRemoteButton(Qt::Key_ChannelDown, 3);
    QCOMPARE(lib::keys.count(0xff56), 4);
    takeKeyEvents();
    // scroll_down too
    setButtons(R"({"MUTE": {"action": "function", "function": "scroll_down"},
                   "PLAY": {"action": "function", "function": "reload"}})");
    pressRemoteButton(Qt::Key_VolumeMute, 2);
    QCOMPARE(lib::calls.filter(QStringLiteral("scroll(0,-2)")).size(), 3);
    // the other functions act once per press: a held VOLUME zooms one level, a held reload reloads once
    pressRemoteButton(Qt::Key_VolumeUp, 3);
    QCOMPARE(zoomCalls(), QStringList{QStringLiteral("set_zoom(1.1)")});
    pressRemoteButton(Qt::Key_MediaTogglePlayPause, 3);
    QCOMPARE(callCount("reload"), 1);
    QCOMPARE(presses("activityPresses", "MUTE"), 0);
    QCOMPARE(presses("activityPresses", "PLAY"), 0);
}

void testWebViewEntityScreen::theWebViewsButtonsGoBeforeTheGlobalSetting() {
    // the action of a button: the web view's user override, else its option, else the global setting
    entity = createEntity(
        QVariantMap{{"url", "https://a.example/"},
                    {"buttons", QVariantMap{{"VOLUME_UP", QVariantMap{{"action", "function"}, {"function", "zoom_in"}}},
                                            {"CHANNEL_UP", QVariantMap{{"action", "key"}, {"key", "ChannelUp"}}}}},
                    {"user", QVariantMap{{"buttons", QVariantMap{{"CHANNEL_UP", QVariantMap{{"action", "pass"}}}}}}}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"VOLUME_UP": {"action": "pass"}, "VOLUME_DOWN": {"action": "pass"},
                   "CHANNEL_DOWN": {"action": "key", "key": "ChannelDown"}})");

    // the web view's option wins over the global pass
    zoomKey(Qt::Key_VolumeUp);
    QCOMPARE(webView()->zoom(), 1.1);
    QCOMPARE(presses("activityPresses", "VOLUME_UP"), 0);
    // the global setting where the web view sets nothing
    pressRemoteButton(Qt::Key_VolumeDown);
    QCOMPARE(presses("activityPresses", "VOLUME_DOWN"), 1);
    pressRemoteButton(Qt::Key_ChannelDown);
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0x10081193 411 0", "up 0x10081193 411 0"}));
    // the user's override wins over the option
    pressRemoteButton(Qt::Key_ChannelUp);
    QCOMPARE(presses("activityPresses", "CHANNEL_UP"), 1);
    QVERIFY(takeKeyEvents().isEmpty());

    // an options update applies at once: without the override, the option's key
    entity->updateOptions(
        QVariantMap{{"url", "https://a.example/"},
                    {"buttons", QVariantMap{{"CHANNEL_UP", QVariantMap{{"action", "key"}, {"key", "ChannelUp"}}}}}});
    pressRemoteButton(Qt::Key_ChannelUp);
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0x10081192 410 0", "up 0x10081192 410 0"}));
    QCOMPARE(presses("activityPresses", "CHANNEL_UP"), 1);
    pressRemoteButton(Qt::Key_VolumeUp);
    QCOMPARE(presses("activityPresses", "VOLUME_UP"), 1);
}

void testWebViewEntityScreen::functionsZoomScrollAndGoThroughTheHistory() {
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}, {"zoom", 80}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    // MUTE runs the function under test
    auto run = [this](const char *function) {
        setButtons(QStringLiteral(R"({"MUTE": {"action": "function", "function": "%1"}})")
                       .arg(QString::fromLatin1(function))
                       .toUtf8()
                       .constData());
        lib::calls.clear();
        takeKeyEvents();
        pressRemoteButton(Qt::Key_VolumeMute);
        if (!lib::renders.isEmpty()) {
            lib::finishRender();
        }
    };

    run("zoom_in");
    QCOMPARE(webView()->zoom(), 0.9);
    run("zoom_out");
    run("zoom_out");
    QCOMPARE(webView()->zoom(), 0.75);
    // back to the web view's zoom, with the label
    run("zoom_reset");
    QCOMPARE(webView()->zoom(), 0.8);
    QQuickItem *label = screen->findChild<QQuickItem *>("zoomLabel");
    QTRY_VERIFY(label->isVisible());
    QCOMPARE(entity->getZoom(), 80);

    run("scroll_up");
    QCOMPARE(lib::calls, QStringList{QStringLiteral("scroll(0,2)")});
    run("scroll_down");
    QCOMPARE(lib::calls, QStringList{QStringLiteral("scroll(0,-2)")});
    run("page_up");
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff55 112 0", "up 0xff55 112 0"}));
    run("page_down");
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff56 117 0", "up 0xff56 117 0"}));
    run("top");
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff50 110 0", "up 0xff50 110 0"}));
    run("bottom");
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff57 115 0", "up 0xff57 115 0"}));
    run("reload");
    QCOMPARE(lib::calls, QStringList{QStringLiteral("reload")});
    run("stop");
    QCOMPARE(lib::calls, QStringList{QStringLiteral("stop")});

    // back and forward only with history in that direction
    run("back");
    QVERIFY(lib::calls.isEmpty());
    run("forward");
    QVERIFY(lib::calls.isEmpty());
    lib::canGoBack = true;
    lib::canGoForward = true;
    lib::fireFrame();  // the item reads the history again
    QTRY_VERIFY(webView()->canGoBack() && webView()->canGoForward());
    run("back");
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_back")});
    run("forward");
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_forward")});
    // back_or_close goes back while there is history
    run("back_or_close");
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_back")});
    QCOMPARE(screen->property("state").toString(), QStringLiteral("open"));
    QCOMPARE(presses("activityPresses", "MUTE"), 0);
}

void testWebViewEntityScreen::functionsCloseToggleTheNavigationMoveTheFocusOrDoNothing() {
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"MUTE": {"action": "function", "function": "toggle_navigation"},
                   "PLAY": {"action": "function", "function": "focus_next"},
                   "NEXT": {"action": "function", "function": "focus_previous"},
                   "PREV": {"action": "function", "function": "none"},
                   "MENU": {"action": "function", "function": "back_or_close"}})");

    // the navigation bar for the visit, the entity's option stays
    QQuickItem *back = part(screen, "navigation_back");
    QVERIFY(back);
    QVERIFY(!back->isVisible());
    pressRemoteButton(Qt::Key_VolumeMute);
    QTRY_VERIFY(back->isVisible());
    QVERIFY(webView()->height() < screen->height());
    QVERIFY(!entity->getShowNavigation());
    pressRemoteButton(Qt::Key_VolumeMute);
    QTRY_VERIFY(!back->isVisible());
    QTRY_COMPARE(webView()->height(), screen->height());

    // Tab and Shift+Tab
    pressRemoteButton(Qt::Key_MediaTogglePlayPause);
    pressRemoteButton(Qt::Key_AudioForward);
    QCOMPARE(takeKeyEvents(),
             (QStringList{"down 0xff09 23 0", "up 0xff09 23 0", "down 0xff09 23 2", "up 0xff09 23 2"}));

    // none: nothing, also not the activity's mapping
    lib::calls.clear();
    pressRemoteButton(Qt::Key_AudioRewind);
    QVERIFY(takeKeyEvents().isEmpty());
    QVERIFY(lib::calls.isEmpty());
    QCOMPARE(presses("activityPresses", "PREV"), 0);

    // back_or_close on the first page: the web view closes
    pressRemoteButton(Qt::Key_F4);
    QTRY_COMPARE(screen->property("state").toString(), QStringLiteral("closed"));
    QVERIFY(lib::calls.isEmpty());
    QCOMPARE(presses("activityPresses", "MENU"), 0);

    // close
    QVERIFY(QMetaObject::invokeMethod(host, "closeScreen"));
    QTRY_VERIFY(screen.isNull());
    setButtons(R"({"MUTE": {"action": "function", "function": "close"}})");
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    pressRemoteButton(Qt::Key_VolumeMute);
    QTRY_COMPARE(screen->property("state").toString(), QStringLiteral("closed"));
}

void testWebViewEntityScreen::backWithAnotherActionKeepsTheWebViewOpen() {
    // BACK sends Escape: pressed, repeated while held, released; the web view stays open, also after a long press
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"BACK": {"action": "key", "key": "Escape"}})");
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Exit, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &press);
    QTest::qWait(900);
    QKeyEvent repeat(QEvent::KeyPress, Qt::Key_Exit, Qt::NoModifier, QString(), true);
    QCoreApplication::sendEvent(view, &repeat);
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Exit, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &release);
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff1b 9 0", "down 0xff1b 9 0", "up 0xff1b 9 0"}));
    QTest::qWait(400);
    QCOMPARE(screen->property("state").toString(), QStringLiteral("open"));

    // BACK with pass and no BACK on the main screen: it closes the web view
    setButtons(R"({"BACK": {"action": "pass"}})");
    pressRemoteButton(Qt::Key_Exit);
    QTRY_COMPARE(screen->property("state").toString(), QStringLiteral("closed"));
    QVERIFY(takeKeyEvents().isEmpty());

    // on top of an activity that maps BACK: the activity's mapping, the web view stays
    QVERIFY(QMetaObject::invokeMethod(host, "closeScreen"));
    QTRY_VERIFY(screen.isNull());
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    pressRemoteButton(Qt::Key_Exit);
    QCOMPARE(presses("activityPresses", "BACK"), 1);
    QTest::qWait(400);
    QCOMPARE(screen->property("state").toString(), QStringLiteral("open"));
}

void testWebViewEntityScreen::buttonsOfTheModel_data() {
    QTest::addColumn<int>("model");
    QTest::newRow("Remote 3") << static_cast<int>(HardwareModel::UCR3);
    QTest::newRow("Remote Two") << static_cast<int>(HardwareModel::UCR2);
}

void testWebViewEntityScreen::buttonsOfTheModel() {
    // the Remote Two has the colour buttons, the Remote 3 MENU: a button the model does not have in the mapping keeps
    // the activity's mapping, whatever the setting says
    QFETCH(int, model);
    cleanup();
    WebViewControllerStub::s_model = static_cast<HardwareModel::Enum>(model);
    init();
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"RED": {"action": "key", "key": "ColorF0Red"}, "MENU": {"action": "key", "key": "Exit"}})");

    pressRemoteButton(Qt::Key_Red);
    pressRemoteButton(Qt::Key_F4);
    if (model == HardwareModel::UCR2) {
        QCOMPARE(takeKeyEvents(), (QStringList{"down 0x1008ffa3 406 0", "up 0x1008ffa3 406 0"}));
        QCOMPARE(presses("activityPresses", "RED"), 0);
        QCOMPARE(presses("activityPresses", "MENU"), 1);
    } else {
        QCOMPARE(takeKeyEvents(), (QStringList{"down 0x1008ff56 182 0", "up 0x1008ff56 182 0"}));
        QCOMPARE(presses("activityPresses", "RED"), 1);
        QCOMPARE(presses("activityPresses", "MENU"), 0);
    }
}

void testWebViewEntityScreen::whileTypingTheDpadAndOkGoToTheTextField() {
    // the on-screen keyboard is up on a field of the page: the d-pad and OK are the text field's keys whatever their
    // action, forwarded once by the WebView on the focus chain; the screen's mapping does not send them
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());
    setButtons(R"({"DPAD_MIDDLE": {"action": "function", "function": "reload"}, "DPAD_UP": {"action": "pass"}})");
    QObject *keyboard = host->findChild<QObject *>("keyboard");
    QVERIFY(keyboard);
    keyboard->setProperty("active", true);

    lib::calls.clear();
    QTest::keyClick(view, Qt::Key_Return);
    QTest::keyClick(view, Qt::Key_Up);
    QTest::keyClick(view, Qt::Key_Left);
    // the focus chain's keycode (the event's scan code, 0 here), not the button mapping's
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff0d 0 0", "up 0xff0d 0 0", "down 0xff52 0 0", "up 0xff52 0 0",
                                           "down 0xff51 0 0", "up 0xff51 0 0"}));
    QVERIFY(lib::calls.filter(QStringLiteral("reload")).isEmpty());
    QCOMPARE(presses("activityPresses", "DPAD_UP"), 0);

    // a press made while typing is released the same way, also when the keyboard went in between
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &press);
    keyboard->setProperty("active", false);
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Down, Qt::NoModifier);
    QCoreApplication::sendEvent(view, &release);
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff54 0 0", "up 0xff54 0 0"}));

    // the keyboard is down: the mapping again
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(callCount("reload"), 1);
    QTest::keyClick(view, Qt::Key_Up);
    QCOMPARE(presses("activityPresses", "DPAD_UP"), 1);
    QVERIFY(takeKeyEvents().isEmpty());
}

void testWebViewEntityScreen::aDialogOfThePageOwnsTheButtons() {
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    setButtons(R"({"CHANNEL_UP": {"action": "key", "key": "ChannelUp"}})");
    // an alert() of the page fades in: the screen still has the input, but its keys and functions go nowhere
    lib::fireAlert(7);
    QTRY_VERIFY(requestShown());
    QCOMPARE(ui->input()->activeItem(), screen);
    takeKeyEvents();
    pressRemoteButton(Qt::Key_ChannelUp);
    pressRemoteButton(Qt::Key_ChannelDown);
    pressRemoteButton(Qt::Key_Down);
    QVERIFY(takeKeyEvents().isEmpty());
    QCOMPARE(presses("activityPresses", "CHANNEL_UP"), 0);

    // then it owns the input: nothing goes to the page or to the activity's mapping
    QTRY_VERIFY(ui->input()->activeItem() != screen);
    pressRemoteButton(Qt::Key_ChannelUp);
    pressRemoteButton(Qt::Key_Down);
    pressRemoteButton(Qt::Key_VolumeMute);
    QVERIFY(takeKeyEvents().isEmpty());
    QCOMPARE(presses("activityPresses", "CHANNEL_UP"), 0);
    QCOMPARE(presses("activityPresses", "MUTE"), 0);
    QCOMPARE(screen->property("state").toString(), QStringLiteral("open"));
}

// Regression: on Qt 5.15.2, the Qt of the unit tests in CI, a garbage collection during a for-of loop over a C++ list
// corrupts the JavaScript heap. The screen's button mapping looped like that over WebViewController.buttonIds (a
// QStringList): with a collection during the loop it mapped other values than the buttons, or the test crashed. Here
// the engine collects at every allocation (QV4_MM_AGGRESSIVE_GC, read when an engine is created).
void testWebViewEntityScreen::buttonsKeepTheirActionsWhileTheGarbageCollectorRuns() {
    // a run with QV4_MM_AGGRESSIVE_GC keeps it for the following tests
    const bool collecting = qEnvironmentVariableIsSet("QV4_MM_AGGRESSIVE_GC");
    cleanup();
    qputenv("QV4_MM_AGGRESSIVE_GC", "1");
    init();
    if (!collecting) {
        qunsetenv("QV4_MM_AGGRESSIVE_GC");
    }
    if (QTest::currentTestFailed()) {
        return;
    }

    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    QVERIFY(openScreen(true));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());
    // a function, a key to the page and a pass, after a change of the setting that maps the buttons again
    setButtons(R"({"MUTE": {"action": "function", "function": "zoom_in"}})");
    pressRemoteButton(Qt::Key_VolumeMute);
    QCOMPARE(webView()->zoom(), 1.1);
    takeKeyEvents();
    pressRemoteButton(Qt::Key_Up);
    QCOMPARE(takeKeyEvents(), (QStringList{"down 0xff52 111 0", "up 0xff52 111 0"}));
    pressRemoteButton(Qt::Key_MediaTogglePlayPause);
    QCOMPARE(presses("activityPresses", "PLAY"), 1);
    QCOMPARE(presses("activityPresses", "MUTE"), 0);
}

QQuickItem *testWebViewEntityScreen::part(QQuickItem *in, const char *name) const {
    if (!in) {
        return nullptr;
    }
    // the visual children too: the delegates of a Repeater are none of its QObject children
    for (QQuickItem *child : in->childItems()) {
        if (child->objectName() == QLatin1String(name)) {
            return child;
        }
        if (QQuickItem *found = part(child, name)) {
            return found;
        }
    }
    return in->findChild<QQuickItem *>(QString::fromLatin1(name));
}

void testWebViewEntityScreen::click(QQuickItem *item) {
    const QPointF centre = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
}

namespace {

// Holds while less than the load indicator's delay has passed on clock (started before the load), checking that
// nothing of the indicator shows: the check runs only on a time read before the event loop could end the delay.
void expectNothingDuringTheDelay(const QElapsedTimer &clock, QQuickItem *card, QQuickItem *bar) {
    while (clock.elapsed() < WebViewItem::kLoadIndicatorDelayMs - 50) {
        QVERIFY(!card->isVisible());
        QVERIFY(!bar->isVisible());
        QTest::qWait(20);
    }
}

QPointF centreInScene(QQuickItem *item) {
    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
}

// centred within a pixel (anchors.centerIn rounds)
bool centredOn(QQuickItem *item, QQuickItem *on) {
    return (centreInScene(item) - centreInScene(on)).manhattanLength() <= 1;
}

}  // namespace

void testWebViewEntityScreen::spinnerAfterTheDelayUntilTheFirstFrameThenTheBar() {
    // a slow page: a white screen, then the spinner with the percentage, until the page's first frame; then the thin
    // bar at the top edge until the page has loaded
    entity = createEntity(QVariantMap{{"url", "https://slow.example/"}});
    QElapsedTimer clock;
    clock.start();
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(lib::calls.contains(QStringLiteral("load_uri(https://slow.example/)")));
    QQuickItem *card = part(screen, "loadIndicatorCard");
    QQuickItem *spinner = part(screen, "loadSpinner");
    QQuickItem *progress = part(screen, "loadProgressText");
    QQuickItem *bar = part(screen, "loadProgressBar");
    QQuickItem *fill = part(screen, "loadProgressFill");
    QVERIFY(card && spinner && progress && bar && fill);

    // nothing for the delay: a quick page does not flicker
    expectNothingDuringTheDelay(clock, card, bar);
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://slow.example/");
    lib::fireProgress(0.1);
    QTRY_VERIFY(card->isVisible());
    QVERIFY(clock.elapsed() >= WebViewItem::kLoadIndicatorDelayMs);
    QVERIFY(spinner->isVisible());
    QVERIFY(spinner->width() >= 40);
    QVERIFY(progress->isVisible());
    QCOMPARE(progress->property("text").toString(), QStringLiteral("10 %"));
    QVERIFY(!bar->isVisible());
    // centred on the page in a black card with the light text: readable on white and on any background colour
    QVERIFY(centredOn(card, webView()));
    QCOMPARE(card->property("color").value<QColor>(), colors.property("bg").value<QColor>());
    QCOMPARE(progress->property("color").value<QColor>(), colors.property("textPrimary").value<QColor>());
    lib::fireProgress(0.45);
    QCOMPARE(progress->property("text").toString(), QStringLiteral("45 %"));

    // committed, not painted yet: the spinner stays
    lib::fireLoad(UC_WEBVIEW_LOAD_COMMITTED, "https://slow.example/");
    QVERIFY(card->isVisible());

    // the page's first frame: the spinner goes, the bar shows the progress at the top edge
    lib::fireFrame();
    QVERIFY(!card->isVisible());
    QVERIFY(bar->isVisible());
    QCOMPARE(bar->mapToScene(QPointF(0, 0)), webView()->mapToScene(QPointF(0, 0)));
    QCOMPARE(bar->width(), webView()->width());
    QVERIFY(bar->height() > 0 && bar->height() <= 8);
    QCOMPARE(fill->width(), bar->width() * 0.45);
    lib::fireProgress(0.9);
    QCOMPARE(fill->width(), bar->width() * 0.9);

    // loaded: nothing
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, "https://slow.example/");
    QVERIFY(!bar->isVisible());
    QVERIFY(!card->isVisible());
}

void testWebViewEntityScreen::reloadsShowTheBarOrNothingAndBackTheSpinner() {
    entity =
        createEntity(QVariantMap{{"url", "https://a.example/"}, {"show_navigation", true}, {"reload_interval", 60}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(lib::calls.contains(QStringLiteral("load_uri(https://a.example/)")));
    lib::loadQuickly("https://a.example/");
    QQuickItem *card = part(screen, "loadIndicatorCard");
    QQuickItem *bar = part(screen, "loadProgressBar");
    QVERIFY(card && bar);

    // the `reload` command: the old page stays until the new one commits, only the bar
    QElapsedTimer clock;
    clock.start();
    emit WebViewControllerStub::s_current->reloadRequested(kEntityId);
    QCOMPARE(callCount("reload"), 1);
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://a.example/");
    expectNothingDuringTheDelay(clock, card, bar);
    QTRY_VERIFY(bar->isVisible());
    QVERIFY(!card->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_COMMITTED, "https://a.example/");
    lib::fireFrame();
    QVERIFY(bar->isVisible());
    QVERIFY(!card->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, "https://a.example/");
    QVERIFY(!bar->isVisible());

    // Reload in the navigation bar: the same
    QQuickItem *reloadButton = part(screen, "navigation_reload");
    QVERIFY(reloadButton);
    click(reloadButton);
    QCOMPARE(callCount("reload"), 2);
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://a.example/");
    QTRY_VERIFY(bar->isVisible());
    QVERIFY(!card->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_COMMITTED, "https://a.example/");
    lib::fireFrame();
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, "https://a.example/");
    QVERIFY(!bar->isVisible());

    // the reload interval: nothing at all
    QObject *timer = screen->findChild<QObject *>(QStringLiteral("reloadTimer"));
    QVERIFY(timer);
    QVERIFY(QMetaObject::invokeMethod(timer, "triggered"));
    QCOMPARE(callCount("reload"), 3);
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://a.example/");
    QTest::qWait(WebViewItem::kLoadIndicatorDelayMs + 100);
    QVERIFY(!bar->isVisible());
    QVERIFY(!card->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_COMMITTED, "https://a.example/");
    lib::fireFrame();
    QTest::qWait(50);
    QVERIFY(!bar->isVisible());
    QVERIFY(!card->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, "https://a.example/");

    // Back in the navigation bar: another page, like a new load, the spinner until its first frame
    lib::canGoBack = true;
    lib::fireFrame();  // the item reads the history again
    QQuickItem *backButton = part(screen, "navigation_back");
    QVERIFY(backButton);
    QTRY_VERIFY(backButton->isEnabled());
    click(backButton);
    QCOMPARE(callCount("go_back"), 1);
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://a.example/before");
    QTRY_VERIFY(card->isVisible());
    QVERIFY(!bar->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_COMMITTED, "https://a.example/before");
    lib::fireFrame();
    QVERIFY(!card->isVisible());
    QVERIFY(bar->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, "https://a.example/before");
    QVERIFY(!bar->isVisible());
}

void testWebViewEntityScreen::failedLoadShowsTheMessageAndOkTriesAgain() {
    entity = createEntity(QVariantMap{{"url", "https://down.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());
    QQuickItem *card = part(screen, "loadIndicatorCard");
    QQuickItem *spinner = part(screen, "loadSpinner");
    QQuickItem *text = part(screen, "loadFailedText");
    QQuickItem *hint = part(screen, "loadFailedHint");
    QQuickItem *cover = part(screen, "loadFailedCover");
    QVERIFY(card && spinner && text && hint && cover);
    QVERIFY(!cover->isVisible());

    // the message at once, over the page's background: WebKit's error page and its raw error text stay hidden
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://down.example/");
    lib::fireLoad(UC_WEBVIEW_LOAD_FAILED, "https://down.example/");
    QVERIFY(card->isVisible());
    QVERIFY(!spinner->isVisible());
    QVERIFY(text->isVisible());
    QCOMPARE(text->property("text").toString(), QStringLiteral("The page could not be loaded"));
    QVERIFY(hint->isVisible());
    QCOMPARE(hint->property("text").toString(), QStringLiteral("Press OK to try again"));
    QVERIFY(cover->isVisible());
    QCOMPARE(cover->property("color").value<QColor>(), QColor(Qt::white));
    QCOMPARE(cover->mapRectToScene(QRectF(0, 0, cover->width(), cover->height())),
             webView()->mapRectToScene(QRectF(0, 0, webView()->width(), webView()->height())));
    QVERIFY(centredOn(card, webView()));
    QVERIFY(!text->property("truncated").toBool());
    QVERIFY(card->width() <= webView()->width() - 40);
    // in the firmware's font the message takes at most two lines and the hint one on the 480 px panel
    if (QFontInfo(fonts.prose()).family() == QLatin1String("Poppins")) {
        QVERIFY(text->property("lineCount").toInt() <= 2);
        QCOMPARE(hint->property("lineCount").toInt(), 1);
    }
    // the error page loads below it: the message stays
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, "https://down.example/");
    lib::loadQuickly("https://down.example/");
    QTest::qWait(WebViewItem::kLoadIndicatorDelayMs + 100);
    QVERIFY(text->isVisible());
    QVERIFY(cover->isVisible());

    // OK tries again and does not reach the error page; the message gives way, the spinner follows over the cover
    lib::keys.clear();
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(callCount("reload"), 1);
    QVERIFY(lib::keys.isEmpty());
    QVERIFY(!text->isVisible());
    QVERIFY(cover->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://down.example/");
    QTRY_VERIFY(spinner->isVisible());
    QVERIFY(cover->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_COMMITTED, "https://down.example/");
    lib::fireFrame();
    QVERIFY(!cover->isVisible());
    QVERIFY(!card->isVisible());
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, "https://down.example/");
    // OK goes to the page again
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(lib::keys, QList<uint32_t>{0xff0d});
    QCOMPARE(callCount("reload"), 1);

    // the page fails again: a tap on the message does nothing, Try again tries again; without the library's details
    // there is no Details
    lib::fail("https://down.example/");
    QVERIFY(text->isVisible());
    QQuickItem *retry = part(screen, "loadFailedRetry");
    QQuickItem *details = part(screen, "loadFailedDetails");
    QVERIFY(retry && details);
    QVERIFY(retry->isVisible());
    QVERIFY(!details->isVisible());
    click(text);
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier,
                      cover->mapToScene(QPointF(cover->width() / 2, 20)).toPoint());
    QCOMPARE(callCount("reload"), 1);
    QVERIFY(text->isVisible());
    click(retry);
    QCOMPARE(callCount("reload"), 2);
    QVERIFY(!text->isVisible());
    // a tap on the cover during the retry does nothing and does not reach the page
    lib::calls.clear();
    click(cover);
    QVERIFY(lib::calls.filter(QRegularExpression(QStringLiteral("^(reload|pointer_button)"))).isEmpty());

    // BACK closes the web view whatever the page shows
    pressRemoteButton(Qt::Key_Exit);
    QTRY_COMPARE(screen->property("state").toString(), QStringLiteral("closed"));
}

void testWebViewEntityScreen::indicatorLetsTouchesAndKeysThroughToThePage() {
    entity = createEntity(QVariantMap{{"url", "https://slow.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());
    QQuickItem *card = part(screen, "loadIndicatorCard");
    QQuickItem *bar = part(screen, "loadProgressBar");
    QVERIFY(card && bar);
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://slow.example/");
    QTRY_VERIFY(card->isVisible());

    // the spinner takes nothing: a tap on it and the d-pad reach the page
    lib::calls.clear();
    click(card);
    QCOMPARE(callCount("pointer_button"), 1);
    QTest::keyClick(view, Qt::Key_Down);
    QCOMPARE(lib::keys, QList<uint32_t>{0xff54});

    // nor does the bar over the page that is shown
    lib::fireLoad(UC_WEBVIEW_LOAD_COMMITTED, "https://slow.example/");
    lib::fireFrame();
    QVERIFY(bar->isVisible());
    const QPointF onTheBar = bar->mapToScene(QPointF(bar->width() / 2, bar->height() / 2));
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, onTheBar.toPoint());
    QCOMPARE(callCount("pointer_button"), 2);
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(lib::keys, (QList<uint32_t>{0xff54, 0xff0d}));
    QVERIFY(lib::calls.filter(QStringLiteral("reload")).isEmpty());
}

void testWebViewEntityScreen::failureSaysWhy_data() {
    QTest::addColumn<int>("reason");  // UC_WEBVIEW_FAIL_*, or -1 - UC_WEBVIEW_TERMINATED_* for two crashes
    QTest::addColumn<QString>("expected");

    QTest::newRow("host not found")
        << UC_WEBVIEW_FAIL_HOST_NOT_FOUND
        << "The server www.example.com wasn't found. Check the Remote's Wi-Fi connection and the address.";
    QTest::newRow("no network") << UC_WEBVIEW_FAIL_NO_NETWORK << "The Remote isn't connected to a network.";
    QTest::newRow("connection refused")
        << UC_WEBVIEW_FAIL_CONNECTION_REFUSED
        << "www.example.com refused the connection. Check that the server is running and the port is correct.";
    QTest::newRow("timeout") << UC_WEBVIEW_FAIL_TIMEOUT << "www.example.com didn't respond in time.";
    QTest::newRow("certificate")
        << UC_WEBVIEW_FAIL_CERTIFICATE
        << "The certificate of www.example.com isn't trusted (expired, wrong name, or self-signed).";
    QTest::newRow("scheme refused") << UC_WEBVIEW_FAIL_SCHEME_REFUSED
                                    << "Only http:// and https:// addresses can be opened.";
    QTest::newRow("other") << UC_WEBVIEW_FAIL_OTHER << "";
    QTest::newRow("crashed") << -1 - UC_WEBVIEW_TERMINATED_CRASHED << "The page stopped working.";
    QTest::newRow("out of memory") << -1 - UC_WEBVIEW_TERMINATED_EXCEEDED_MEMORY_LIMIT
                                   << "The page needs more memory than the Remote has.";
}

void testWebViewEntityScreen::failureSaysWhy() {
    QFETCH(int, reason);
    QFETCH(QString, expected);

    entity = createEntity(QVariantMap{{"url", "https://www.example.com/status?key=1"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QQuickItem *card = part(screen, "loadIndicatorCard");
    QQuickItem *text = part(screen, "loadFailedText");
    QQuickItem *line = part(screen, "loadFailedReason");
    QQuickItem *retry = part(screen, "loadFailedRetry");
    QQuickItem *details = part(screen, "loadFailedDetails");
    QQuickItem *hint = part(screen, "loadFailedHint");
    QVERIFY(card && text && line && retry && details && hint);

    const char *uri = "https://www.example.com/status?key=1";
    QTRY_VERIFY(lib::calls.contains(QStringLiteral("load_uri(%1)").arg(QString::fromLatin1(uri))));
    if (reason >= 0) {
        lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, uri);
        lib::fireFailDetail(uri, reason, "g-io-error-quark", 1, 0, "Some error");
        lib::fireLoad(UC_WEBVIEW_LOAD_FAILED, uri);
    } else {
        // the first crash reloads by itself, the second one shows the failure
        lib::fireTerminated(-1 - reason);
        QTRY_COMPARE(callCount("reload"), 1);
        lib::fireTerminated(-1 - reason);
    }
    QVERIFY(card->isVisible());
    // the generic message stays the title, the reason below it
    QCOMPARE(text->property("text").toString(), QStringLiteral("The page could not be loaded"));
    QCOMPARE(line->isVisible(), !expected.isEmpty());
    if (!expected.isEmpty()) {
        QCOMPARE(line->property("text").toString(), expected);
        QVERIFY(!line->property("truncated").toBool());
    }
    // Try again and Details side by side, the hint below them, all inside the card
    QVERIFY(retry->isVisible());
    QVERIFY(details->isVisible());
    QVERIFY(hint->isVisible());
    QCOMPARE(retry->y(), details->y());
    QVERIFY(retry->x() + retry->width() < details->x());
    const QRectF inCard = card->mapRectToScene(QRectF(0, 0, card->width(), card->height()));
    for (QQuickItem *item : {text, retry, details, hint}) {
        QVERIFY(inCard.contains(item->mapRectToScene(QRectF(0, 0, item->width(), item->height()))));
    }
    QVERIFY(card->width() <= webView()->width() - 40);
    QVERIFY(card->height() <= webView()->height() - 40);
}

void testWebViewEntityScreen::detailsShowTheTechnicalErrorAndClose() {
    entity = createEntity(QVariantMap{{"url", "https://self-signed.example:8443/ui?token=s3cret"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QTRY_VERIFY(webView()->hasActiveFocus());
    QQuickItem *details = part(screen, "loadFailedDetails");
    QQuickItem *text = part(screen, "loadFailedText");
    auto       *popup = screen->findChild<QObject *>(QStringLiteral("loadFailedDetailsPopup"));
    QVERIFY(details && text && popup);

    const char *uri = "https://user:pw@self-signed.example:8443/ui?token=s3cret";
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, uri);
    // G_TLS_CERTIFICATE_UNKNOWN_CA | BAD_IDENTITY | EXPIRED
    lib::fireFailDetail(uri, UC_WEBVIEW_FAIL_CERTIFICATE, "WebKitNetworkError", 1, 1u | 2u | 8u,
                        "Unacceptable TLS certificate");
    lib::fireLoad(UC_WEBVIEW_LOAD_FAILED, uri);
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, uri);
    QVERIFY(details->isVisible());

    // Details opens the sheet with the technical text: the address without path, query and user info
    click(details);
    QTRY_VERIFY(popup->property("opened").toBool());
    QQuickItem *detailsText = part(view->contentItem(), "loadFailedDetailsText");
    QQuickItem *close = part(view->contentItem(), "loadFailedDetailsClose");
    QVERIFY(detailsText && close);
    QCOMPARE(detailsText->property("text").toString(),
             QStringLiteral("https://self-signed.example:8443\nUnacceptable TLS certificate\nexpired, unknown issuer, "
                            "wrong host name\nWebKitNetworkError 1"));
    QVERIFY(close->isVisible());
    QCOMPARE(callCount("reload"), 0);

    // BACK closes the sheet, not the web view; the page gets the keys again
    pressRemoteButton(Qt::Key_Exit);
    QTRY_VERIFY(!popup->property("visible").toBool());
    QCOMPARE(screen->property("state").toString(), QStringLiteral("open"));
    QTRY_VERIFY(webView()->hasActiveFocus());
    QVERIFY(text->isVisible());

    // a tap above the sheet closes it
    click(details);
    QTRY_VERIFY(popup->property("opened").toBool());
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(240, 20));
    QTRY_VERIFY(!popup->property("visible").toBool());

    // and Close; OK on the sheet presses Close, it does not try again
    click(details);
    QTRY_VERIFY(popup->property("opened").toBool());
    QTest::keyClick(view, Qt::Key_Return);
    QTRY_VERIFY(!popup->property("visible").toBool());
    QCOMPARE(callCount("reload"), 0);
    click(details);
    QTRY_VERIFY(popup->property("opened").toBool());
    click(close);
    QTRY_VERIFY(!popup->property("visible").toBool());
    QCOMPARE(callCount("reload"), 0);
    QTRY_VERIFY(webView()->hasActiveFocus());

    // OK tries again after the sheet
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(callCount("reload"), 1);
    QVERIFY(!details->isVisible());

    // the sheet goes when the failure does (the reload command, the reload interval's success)
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, uri);
    lib::fireFailDetail(uri, UC_WEBVIEW_FAIL_CERTIFICATE, "WebKitNetworkError", 1, 1u, "Unacceptable TLS certificate");
    lib::fireLoad(UC_WEBVIEW_LOAD_FAILED, uri);
    click(details);
    QTRY_VERIFY(popup->property("opened").toBool());
    webView()->reload();
    QTRY_VERIFY(!popup->property("visible").toBool());
    QCOMPARE(callCount("reload"), 2);
}

void testWebViewEntityScreen::failureOfAnOlderLibraryHasNoReasonAndNoDetails() {
    // a library older than 0.10.0 does not say why: the generic message, Try again alone
    entity = createEntity(QVariantMap{{"url", "https://down.example/"}});
    QVERIFY(openScreen(false));
    QTRY_VERIFY(webView() != nullptr);
    QQuickItem *line = part(screen, "loadFailedReason");
    QQuickItem *retry = part(screen, "loadFailedRetry");
    QQuickItem *details = part(screen, "loadFailedDetails");
    QVERIFY(line && retry && details);
    lib::fail("https://down.example/");
    QVERIFY(retry->isVisible());
    QVERIFY(!line->isVisible());
    QVERIFY(!details->isVisible());
    QCOMPARE(retry->width(), retry->parentItem()->width());
}

void testWebViewEntityScreen::widgetShowsTheIndicatorAndTheFailure_data() {
    QTest::addColumn<bool>("interactive");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<bool>("percentage");
    QTest::addColumn<bool>("reasonShown");

    // 3 x 3 fields of the default 4 x 6 grid of an activity page, and of a denser grid
    QTest::newRow("interactive, 3 x 3 of 4 x 6") << true << QSize(360, 348) << true << true;
    QTest::newRow("not interactive, 3 x 3 of 4 x 6") << false << QSize(360, 348) << true << true;
    QTest::newRow("interactive, small") << true << QSize(170, 160) << false << false;
}

void testWebViewEntityScreen::widgetShowsTheIndicatorAndTheFailure() {
    QFETCH(bool, interactive);
    QFETCH(QSize, size);
    QFETCH(bool, percentage);
    QFETCH(bool, reasonShown);

    entity = createEntity(QVariantMap{{"url", "https://slow.example/"}});
    EntityControllerStub::s_entity = entity;
    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(host, "openWidget", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, kEntityId),
                                      Q_ARG(QVariant, interactive), Q_ARG(QVariant, size.width()),
                                      Q_ARG(QVariant, size.height())));
    auto *widget = qobject_cast<QQuickItem *>(result.value<QObject *>());
    QVERIFY(widget);
    QTRY_VERIFY(lib::calls.contains(QStringLiteral("load_uri(https://slow.example/)")));
    QQuickItem *indicator = part(widget, "loadIndicator");
    QQuickItem *card = part(widget, "loadIndicatorCard");
    QQuickItem *spinner = part(widget, "loadSpinner");
    QQuickItem *progress = part(widget, "loadProgressText");
    QQuickItem *text = part(widget, "loadFailedText");
    QQuickItem *hint = part(widget, "loadFailedHint");
    QVERIFY(indicator && card && spinner && progress && text && hint);
    QCOMPARE(indicator->size(), QSizeF(size));

    // the spinner after the delay, at the widget's size: inside it, the percentage where it fits
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "https://slow.example/");
    lib::fireProgress(0.3);
    QVERIFY(!card->isVisible());
    QTRY_VERIFY(card->isVisible());
    QVERIFY(spinner->isVisible());
    QCOMPARE(progress->isVisible(), percentage);
    QVERIFY(card->width() <= size.width() && card->height() <= size.height());
    QVERIFY(centredOn(card, indicator));

    // the failure without the OK hint and without buttons: the widget takes no keys; the reason where it fits
    lib::fireFailDetail("https://slow.example/", UC_WEBVIEW_FAIL_TIMEOUT, "g-io-error-quark", 24, 0,
                        "Socket I/O timed out");
    lib::fireLoad(UC_WEBVIEW_LOAD_FAILED, "https://slow.example/");
    QVERIFY(text->isVisible());
    QVERIFY(!hint->isVisible());
    QVERIFY(!part(widget, "loadFailedRetry")->isVisible());
    QVERIFY(!part(widget, "loadFailedDetails")->isVisible());
    QQuickItem *line = part(widget, "loadFailedReason");
    QVERIFY(line);
    QCOMPARE(line->isVisible(), reasonShown);
    if (reasonShown) {
        QCOMPARE(line->property("text").toString(), QStringLiteral("slow.example didn't respond in time."));
    }
    QVERIFY(card->width() <= size.width() && card->height() <= size.height());
    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED, "https://slow.example/");

    // a tap tries again when the widget is interactive, else it opens the fullscreen web view as always
    click(card);
    if (interactive) {
        QCOMPARE(callCount("reload"), 1);
        QVERIFY(!text->isVisible());
        QCOMPARE(host->property("fullscreenOpens").toInt(), 0);
    } else {
        QCOMPARE(callCount("reload"), 0);
        QVERIFY(text->isVisible());
        QCOMPARE(host->property("fullscreenOpens").toInt(), 1);
    }
}

void testWebViewEntityScreen::swipeOnAnInteractiveWidgetDoesNotSwitchTheActivityPage_data() {
    QTest::addColumn<bool>("interactive");
    QTest::newRow("interactive") << true;
    QTest::newRow("not interactive") << false;
}

void testWebViewEntityScreen::swipeOnAnInteractiveWidgetDoesNotSwitchTheActivityPage() {
    // core-api ui-widgets.md "Behaviour": swiping on an interactive widget scrolls the page and doesn't switch the user
    // interface page. Regression: the PathView of the activity's pages took the touch over from the WebView once the
    // finger had moved far enough sideways, cancelled it in WebKit and showed the next page. A widget that is not
    // interactive is only shown: the swipe is the PathView's.
    QFETCH(bool, interactive);
    entity = createEntity(QVariantMap{{"url", "https://a.example/"}});
    EntityControllerStub::s_entity = entity;
    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(host, "openPagedWidget", Q_RETURN_ARG(QVariant, result),
                                      Q_ARG(QVariant, kEntityId), Q_ARG(QVariant, interactive)));
    auto *pages = qobject_cast<QQuickItem *>(result.value<QObject *>());
    QVERIFY(pages);
    QTRY_VERIFY(lib::calls.contains(QStringLiteral("load_uri(https://a.example/)")));
    lib::loadQuickly("https://a.example/");
    QCOMPARE(pages->property("currentIndex").toInt(), 0);

    // a swipe to the left over the widget, finger by finger like the touch screen reports it
    static QTouchDevice *device = QTest::createTouchDevice();
    const QPoint         start(400, 250);
    QTest::touchEvent(view, device).press(0, start, view);
    for (int step = 1; step <= 16; ++step) {
        QTest::qWait(10);
        QTest::touchEvent(view, device).move(0, start - QPoint(20 * step, 0), view);
    }
    QTest::touchEvent(view, device).release(0, start - QPoint(320, 0), view);
    QTest::qWait(500);

    if (interactive) {
        // the page got the whole drag, and the activity page stays
        QCOMPARE(pages->property("currentIndex").toInt(), 0);
        QCOMPARE(pages->property("offset").toReal(), 0.0);
        QVERIFY(lib::touches.size() > 2);
        QCOMPARE(lib::touches.first(), static_cast<int>(UC_WEBVIEW_TOUCH_DOWN));
        QCOMPARE(lib::touches.last(), static_cast<int>(UC_WEBVIEW_TOUCH_UP));
        QVERIFY(!lib::touches.contains(UC_WEBVIEW_TOUCH_CANCEL));
    } else {
        QCOMPARE(pages->property("currentIndex").toInt(), 1);
        QVERIFY(lib::touches.isEmpty());
    }
}

int main(int argc, char *argv[]) {
    // offscreen unless the caller chose a platform (ctest sets offscreen too)
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    // a build host has no OpenGL context for the offscreen platform, and the page has no frames to show
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) {
        qputenv("QT_QUICK_BACKEND", QByteArray("software"));
    }
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    // the firmware's text fonts (Poppins, Space Mono), when the caller has them: they are not in the repository
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
    qmlRegisterSingletonType<EntityControllerStub>("Entity.Controller", 1, 0, "EntityController",
                                                   &EntityControllerStub::qmlInstance);
    qmlRegisterSingletonType<IntegrationControllerStub>("Integration.Controller", 1, 0, "IntegrationController",
                                                        &IntegrationControllerStub::qmlInstance);
    qmlRegisterSingletonType<WebViewControllerStub>("WebView.Controller", 1, 0, "WebViewController",
                                                    &WebViewControllerStub::qmlInstance);
    qmlRegisterSingletonType<PowerStub>("Power", 1, 0, "Power", &PowerStub::qmlInstance);

    testWebViewEntityScreen test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "test_webview_entity_screen.moc"
