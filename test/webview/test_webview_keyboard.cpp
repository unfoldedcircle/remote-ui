// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The on-screen keyboard on the WebView settings page: the page as the app loads it (qrc:/settings/settings/
// WebView.qml) under a copy of main.qml's keyboard controller (qml/KeyboardHost.qml), with the real Qt Virtual
// Keyboard input context (QT_IM_MODULE=qtvirtualkeyboard) and the real InputController behind ui.inputController.
//
// Regression: tapping the address field made the keyboard show and hide forever, with no web page loaded. The
// field used InputField's default moveInput (the full-screen keyboardInputField overlay), whose takeControl() made
// the page's ButtonNavigation (manageFocus) park the focus, which hid the virtual keyboard (its panel is visible
// only while the focus object accepts input), whose closed() dropped the field's focus and hid the overlay, whose
// releaseControl() made the ButtonNavigation hand the focus back to the field, which opened the overlay again.
//
// Regression 2: the keyboard's hide key did not close the keyboard while the address field had the focus. InputField
// dropped the field's focus on keyboard.closed(), the page's ButtonNavigation handed it back, and TextInput's focus
// gain re-showed the input method (qquicktextinput.cpp:2681), which the keyboard controller mirrors into show().
// InputField now moves the focus to itself, around the text field, and passes the d-pad on.
//
// Regression 3: after the hide key, a tap on the web page's input that still had the DOM focus did not bring the
// keyboard back: WebKit reports the element's focus only when it changes. The WebView item asks for the keyboard
// again on such a tap; this checks that the request reaches main.qml's controller through the real input context.
//
// Regression 4: the address field could not be reached with the d-pad. It sits left of Load, and Load had only an
// `up` link to it: left did nothing. The page now walks its two rows as they look (field <-> Load, Back <-> Forward
// <-> Stop/Reload, up to the field, down to the web view), with the history buttons skipped while disabled.
//
// PREV and NEXT of the remote go back and forward in the page's history, VOLUME +/-
// zoom the page and CHANNEL +/- scroll it a page, through the page's ButtonNavigation,
// wherever the d-pad focus is; the WebView item must neither forward these keys to the page nor accept them. The
// InputController is installed on the window like main.qml does, so a key travels both paths.

#include <QElapsedTimer>
#include <QJSValue>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

#include <algorithm>
#include <cstring>
#include <utility>

#include "hardware/hardwareModel.h"
#include "ui/colors.h"
#include "ui/fonts.h"
#include "ui/inputController.h"
#include "webview/webviewitem.h"
#include "webview/webviewlibrary.h"

using uc::ui::Colors;
using uc::ui::Fonts;
using uc::ui::InputController;
using uc::webview::WebViewFunctions;
using uc::webview::WebViewItem;
using uc::webview::WebViewLibrary;

namespace {

// The `ui` context property as far as the components of the page use it (uiController.h).
class UiStub : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *inputController READ inputController CONSTANT)
    Q_PROPERTY(int cornerRadiusSmall READ cornerRadiusSmall CONSTANT)
    Q_PROPERTY(int cornerRadiusLarge READ cornerRadiusLarge CONSTANT)
    Q_PROPERTY(bool keyNavigationActive READ keyNavigationActive CONSTANT)
    Q_PROPERTY(double globalBrightness READ globalBrightness CONSTANT)
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(bool webViewAvailable READ webViewAvailable CONSTANT)

 public:
    explicit UiStub(QObject *parent = nullptr) : QObject(parent), m_input(uc::hw::HardwareModel::DEV) {}

    QObject *inputController() { return &m_input; }
    int      cornerRadiusSmall() const { return 10; }
    int      cornerRadiusLarge() const { return 20; }
    bool     keyNavigationActive() const { return true; }
    double   globalBrightness() const { return 1.0; }
    int      width() const { return 480; }
    bool     webViewAvailable() const { return true; }

    // uiController.cpp setTimeOut: a single shot timer calling back into QML
    Q_INVOKABLE void setTimeOut(int msec, QJSValue callback) {
        QTimer::singleShot(msec, this, [callback]() mutable {
            if (callback.isCallable()) {
                callback.call();
            }
        });
    }

    InputController *input() { return &m_input; }

 private:
    InputController m_input;
};

// the Haptic singleton of the hardware backends (haptic.h): the components call Haptic.play(Haptic.<effect>)
class HapticStub : public QObject {
    Q_OBJECT
 public:
    enum Effects { Click = 0, Buzz = 1, Error = 2, Bump = 3 };
    Q_ENUM(Effects)

    Q_INVOKABLE void play(Effects) {}

    static QObject *qmlInstance(QQmlEngine *, QJSEngine *) { return new HapticStub; }
};

// `resource.getIcon()` of Components.Icon (TopNavigation's back arrow)
class ResourceStub : public QObject {
    Q_OBJECT
 public:
    Q_INVOKABLE QString getIcon(const QString &, const QString &) { return QString(); }
};

// libucwebview with nothing behind it: the page creates its WebView item like on the device, no page is loaded. The
// callbacks are kept so that a test can report what WebKit would (a focused input, a running load).
namespace lib {

uc_webview *const kView = reinterpret_cast<uc_webview *>(0x1234);

uc_webview_callbacks callbacks{};
void                *user = nullptr;
QStringList          calls;  // "reload", "stop", "go_back", "go_forward", "load_uri(<uri>)", "set_zoom(<zoom>)"
bool                 canBack = false;
bool                 canForward = false;
QList<uint32_t>      keys;   // keysyms forwarded to the page (uc_webview_key presses)
// uc_webview_when_rendered() requests (library 0.5.0) the page waits for, oldest first: the test ends them
QList<QPair<uc_webview_rendered_cb, void *>> renders;

int         apiVersion() { return UC_WEBVIEW_API_VERSION; }
int         globalInit(void *, const char *, const char *) { return 0; }
void        globalShutdown() {}
uc_webview *create(int, int, const uc_webview_callbacks *cb, void *u) {
    callbacks = *cb;
    user = u;
    return kView;
}
// library 0.4.0 and 0.6.0: the struct of the host's size, and the pinch zoom's magnification
uc_webview *createSized(int, int, const uc_webview_callbacks *cb, size_t size, void *u) {
    callbacks = uc_webview_callbacks{};
    memcpy(&callbacks, cb, std::min(size, sizeof callbacks));
    user = u;
    return kView;
}
int setMagnification(uc_webview *, double magnification, double, double) {
    calls.append(QStringLiteral("set_magnification(%1)").arg(magnification));
    return 0;
}
double getMagnification(uc_webview *) {
    return 1.0;
}
void        destroy(uc_webview *) {
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
// the page rendered: the oldest request ends (the item may issue the next one from the callback)
void finishRender(int status = UC_WEBVIEW_RENDERED_DONE) {
    QVERIFY(!renders.isEmpty());
    const auto render = renders.takeFirst();
    render.first(kView, status, render.second);
}
void        resize(uc_webview *, int, int) {}
void        setInt(uc_webview *, int) {}
void        setDouble(uc_webview *, double) {}
void        setString(uc_webview *, const char *) {}
void        loadHtml(uc_webview *, const char *, const char *) {}
void        reload(uc_webview *) { calls.append(QStringLiteral("reload")); }
void        stop(uc_webview *) { calls.append(QStringLiteral("stop")); }
void        goBack(uc_webview *) { calls.append(QStringLiteral("go_back")); }
void        goForward(uc_webview *) { calls.append(QStringLiteral("go_forward")); }
void        loadUri(uc_webview *, const char *uri) { calls.append(QStringLiteral("load_uri(%1)").arg(uri)); }
void        setZoom(uc_webview *, double zoom) { calls.append(QStringLiteral("set_zoom(%1)").arg(zoom)); }
int         canGoBack(uc_webview *) { return canBack ? 1 : 0; }
int         canGoForward(uc_webview *) { return canForward ? 1 : 0; }
const char *none(uc_webview *) { return nullptr; }
void        setBackground(uc_webview *, float, float, float, float) {}
void        frame(uc_webview *, void *) {}
void        touch(uc_webview *, int, int, double, double, uint32_t) {}
void        pointerButton(uc_webview *, int, int, double, double, unsigned, uint32_t) {}
void        pointerMove(uc_webview *, double, double, unsigned, uint32_t) {}
void        scroll(uc_webview *, double, double, double, double, int, uint32_t) {}
void        key(uc_webview *, uint32_t keysym, uint32_t, int pressed, unsigned, uint32_t) {
    if (pressed) {
        keys.append(keysym);
    }
}
void        imePreedit(uc_webview *, const char *, int) {}
void        imeDeleteSurrounding(uc_webview *, int, int) {}

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
    f.can_go_back = &canGoBack;
    f.can_go_forward = &canGoForward;
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
    return f;
}

// a two-finger pinch as the library reports it
void firePinch(int phase, double scale) {
    QVERIFY(callbacks.pinch != nullptr);
    callbacks.pinch(kView, phase, scale, 200, 300, user);
}

void fireImeFocus(int focused, unsigned purpose) {
    callbacks.ime_focus(kView, focused, UC_WEBVIEW_IME_HINT_NONE, purpose, "", 0, user);
}

void fireLoad(int event, const char *uri = "http://localhost/") {
    callbacks.load_changed(kView, event, uri, user);
}

// WebKit's back/forward list after a navigation; the item reads it on every load event
void setHistory(bool back, bool forward, const char *uri) {
    canBack = back;
    canForward = forward;
    fireLoad(UC_WEBVIEW_LOAD_COMMITTED, uri);
}

}  // namespace lib

// the page's TextField (the one of its InputField; the overlay's field is outside the page)
QQuickItem *textFieldOf(QQuickItem *page) {
    for (QQuickItem *item : page->findChildren<QQuickItem *>()) {
        if (item->inherits("QQuickTextField")) {
            return item;
        }
    }
    return nullptr;
}

// a Components.Button by its text
QQuickItem *buttonOf(QQuickItem *page, const QString &text) {
    for (QQuickItem *item : page->findChildren<QQuickItem *>()) {
        if (item->metaObject()->indexOfProperty("trigger") >= 0 && item->property("text").toString() == text) {
            return item;
        }
    }
    return nullptr;
}

}  // namespace

class testWebViewKeyboard : public QObject {
    Q_OBJECT

 private slots:
    void init();
    void cleanup();

    void tapOnTheAddressFieldShowsTheKeyboardOnceAndItStays();
    void dpadToTheLoadButtonHidesTheKeyboardAndBackShowsItAgain();
    void hideKeyClosesTheKeyboardAndItStaysClosed();
    void dpadReachesTheAddressFieldFromLoad();
    void dpadWalksBothRowsAndIntoTheWebView();
    void backAndForwardFollowTheHistory();
    void disabledHistoryButtonHandsTheFocusOn();
    void addressFieldShowsThePageAddress();
    void stopReloadButtonFollowsTheLoad();
    void addressesAreNormalized();
    void tapOnTheFocusedPageInputReopensTheKeyboardAfterTheHideKey();
    void prevAndNextStepThroughTheHistoryFromEveryControl();
    void prevAndNextWithoutHistoryDoNothing();
    void prevAndNextPassTheFocusedWebView();
    void heldPrevStepsOnce();
    void prevAndNextAreIgnoredWhileTheKeyboardIsUp();
    void volumeZoomsThePageFromEveryControl();
    void heldVolumeZoomsOneLevel();
    void zoomLabelShowsTheLevelAndFades();
    void zoomLabelStaysWithASpinnerWhileThePageRenders();
    void quickZoomStepsShowNoSpinner();
    void quickPressesDuringARenderCostTwoRelayouts();
    void channelScrollsThePageFromEveryControl();
    void heldChannelScrollsOnEveryRepeat();
    void volumeAndChannelAreIgnoredWhileTheKeyboardIsUp();
    void pinchShowsTheMagnificationNextToTheZoom();

 private:
    bool pressRemoteButton(Qt::Key key, int repeats = 0);
    void tapTheAddressField();
    void click(QQuickItem *item);
    void tapTheWebView(const QPointF &moveBy = QPointF());
    void pressTheHideKey();

    QQuickView  *view = nullptr;
    UiStub      *ui = nullptr;
    ResourceStub resource;
    Colors       colors;
    Fonts        fonts;
    QQuickItem  *page = nullptr;
    QQuickItem  *field = nullptr;
    QQuickItem  *fieldFrame = nullptr;  // the InputField around the text field
    QQuickItem  *loadButton = nullptr;
    QQuickItem  *backButton = nullptr;
    QQuickItem  *forwardButton = nullptr;
    QQuickItem  *stopReloadButton = nullptr;
    QQuickItem  *stopReloadIcon = nullptr;
    WebViewItem *webView = nullptr;
    QObject     *keyboard = nullptr;
    QObject     *overlay = nullptr;
    QSignalSpy  *opened = nullptr;
    QSignalSpy  *closed = nullptr;
};

void testWebViewKeyboard::init() {
    ui = new UiStub(this);

    view = new QQuickView;
    view->rootContext()->setContextProperty("ui", ui);
    view->rootContext()->setContextProperty("colors", &colors);
    view->rootContext()->setContextProperty("fonts", &fonts);
    view->rootContext()->setContextProperty("resource", &resource);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->resize(480, 850);
    view->setSource(QUrl("qrc:/test/KeyboardHost.qml"));
    QStringList errors;
    for (const QQmlError &error : view->errors()) {
        errors.append(error.toString());
    }
    QVERIFY2(view->status() == QQuickView::Ready, qPrintable(errors.join("\n")));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));

    QQuickItem *root = view->rootObject();
    page = root->findChild<QQuickItem *>("page");
    keyboard = root->findChild<QObject *>("keyboard");
    overlay = root->findChild<QObject *>("keyboardInputField");
    QVERIFY(page && keyboard && overlay);
    field = textFieldOf(page);
    fieldFrame = field ? field->parentItem() : nullptr;
    loadButton = buttonOf(page, "Load");
    backButton = page->findChild<QQuickItem *>("backButton");
    forwardButton = page->findChild<QQuickItem *>("forwardButton");
    stopReloadButton = page->findChild<QQuickItem *>("stopReloadButton");
    stopReloadIcon = page->findChild<QQuickItem *>("stopReloadIcon");
    QVERIFY(field && fieldFrame && loadButton && backButton && forwardButton && stopReloadButton && stopReloadIcon);
    QVERIFY(!page->findChildren<WebViewItem *>().isEmpty());
    webView = page->findChildren<WebViewItem *>().first();
    lib::calls.clear();
    lib::keys.clear();
    lib::renders.clear();
    lib::canBack = lib::canForward = false;

    // both paths of a key press (docs/key-navigation.md section 1): the input controller filters the window's
    // events like main.qml's ui.inputController.setSource(applicationWindow)
    ui->input()->setSource(view);

    opened = new QSignalSpy(keyboard, SIGNAL(opened()));
    closed = new QSignalSpy(keyboard, SIGNAL(closed()));
    QVERIFY(opened->isValid() && closed->isValid());

    // the settings container hands the input to the page; its ButtonNavigation then claims the focus
    ui->input()->takeControl(page);
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QVERIFY(!keyboard->property("active").toBool());
}

void testWebViewKeyboard::cleanup() {
    delete opened;
    delete closed;
    opened = closed = nullptr;
    delete view;
    view = nullptr;
    delete ui;
    ui = nullptr;
}

void testWebViewKeyboard::tapTheAddressField() {
    QQuickItem *container = field->parentItem();  // the InputField, its MouseArea covers the TextField
    const QPointF center = container->mapToScene(QPointF(container->width() / 2, container->height() / 2));
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, center.toPoint());
}

// a tap (mouse click) in the middle of an item
void testWebViewKeyboard::click(QQuickItem *item) {
    const QPointF center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, center.toPoint());
}

void testWebViewKeyboard::tapTheWebView(const QPointF &moveBy) {
    static QTouchDevice *device = QTest::createTouchDevice();
    // near the top of the view: the keyboard, when up, covers the bottom of the window
    const QPoint at = webView->mapToScene(QPointF(webView->width() / 2, 30)).toPoint();
    QTest::touchEvent(view, device).press(0, at, view);
    if (!moveBy.isNull()) {
        QTest::touchEvent(view, device).move(0, at + moveBy.toPoint(), view);
    }
    QTest::touchEvent(view, device).release(0, at + moveBy.toPoint(), view);
}

void testWebViewKeyboard::pressTheHideKey() {
    // HideKeyboardKey of the app's layouts (src/qml/keyboard/layouts/*/main.qml): QML runs the key's own handler,
    // InputContext.priv.hideInputPanel() (= PlatformInputContext::hideInputPanel, which QInputMethod::hide()
    // reaches), and then the layout's `onClicked: root.keyboard.hide()` (main.qml's keyboard.hide())
    QGuiApplication::inputMethod()->hide();
    QVERIFY(QMetaObject::invokeMethod(keyboard, "hide"));
}

// A button of the remote as the device delivers it to the window: the evdev keymap gives it no text
// (qevdevkeyboardhandler.cpp:249), a held button repeats presses flagged autoRepeat and releases once. The input
// controller (event filter, path 1) and the focused item (path 2) see it. Returns whether an item accepted the press.
bool testWebViewKeyboard::pressRemoteButton(Qt::Key key, int repeats) {
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

void testWebViewKeyboard::tapOnTheAddressFieldShowsTheKeyboardOnceAndItStays() {
    tapTheAddressField();

    QTRY_VERIFY(keyboard->property("active").toBool());
    QVERIFY(field->hasActiveFocus());

    // the loop had a period of about 600 ms (two 300 ms overlay transitions): wait for several of them
    QTest::qWait(2500);

    QCOMPARE(closed->count(), 0);
    QVERIFY(keyboard->property("active").toBool());
    QVERIFY(QGuiApplication::inputMethod()->isVisible());
    QVERIFY(field->hasActiveFocus());
    QCOMPARE(overlay->property("state").toString(), QStringLiteral("hidden"));
    QCOMPARE(ui->input()->activeItem(), page);
}

void testWebViewKeyboard::dpadToTheLoadButtonHidesTheKeyboardAndBackShowsItAgain() {
    tapTheAddressField();
    QTRY_VERIFY(keyboard->property("active").toBool());

    // the caret is at the end of the text: right leaves the field for Load
    QTest::keyClick(view, Qt::Key_Right);
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QTRY_COMPARE(closed->count(), 1);
    QTest::qWait(800);
    QCOMPARE(closed->count(), 1);
    QVERIFY(!keyboard->property("active").toBool());
    QVERIFY(loadButton->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Left);
    QTRY_VERIFY(field->hasActiveFocus());
    QTRY_VERIFY(keyboard->property("active").toBool());
    QTest::qWait(800);
    QCOMPARE(closed->count(), 1);
    QVERIFY(keyboard->property("active").toBool());
    QVERIFY(field->hasActiveFocus());
}

void testWebViewKeyboard::hideKeyClosesTheKeyboardAndItStaysClosed() {
    tapTheAddressField();
    QTRY_VERIFY(keyboard->property("active").toBool());
    QVERIFY(field->hasActiveFocus());

    // The keyboard's hide key (bottom right) is the HideKeyboardKey of the app's own layouts
    // (src/qml/keyboard/layouts/*/main.qml) with `onClicked: root.keyboard.hide()`: main.qml's keyboard.hide(),
    // not Qt.inputMethod.hide(). The InputField around the text keeps the focus (InputField.qml onClosed):
    // dropping it made the ButtonNavigation hand it back to the text field, whose focus gain reopened the keyboard
    // through Qt.inputMethod.visible.
    QVERIFY(QMetaObject::invokeMethod(keyboard, "hide"));
    QTRY_COMPARE(closed->count(), 1);
    QTest::qWait(800);
    QCOMPARE(closed->count(), 1);
    QVERIFY(!keyboard->property("active").toBool());
    QVERIFY(fieldFrame->hasActiveFocus());
    QVERIFY(!field->hasActiveFocus());
    QCOMPARE(ui->input()->activeItem(), page);

    // the next tap on the field opens it again
    tapTheAddressField();
    QTRY_VERIFY(keyboard->property("active").toBool());
    QTest::qWait(300);
    QCOMPARE(closed->count(), 1);
    QVERIFY(keyboard->property("active").toBool());

    // hide again, then walk the d-pad away and back: the focus arrival shows it like before
    QVERIFY(QMetaObject::invokeMethod(keyboard, "hide"));
    QTRY_COMPARE(closed->count(), 2);
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(stopReloadButton->hasActiveFocus());
    QVERIFY(!keyboard->property("active").toBool());
    QTest::keyClick(view, Qt::Key_Up);
    QTRY_VERIFY(field->hasActiveFocus());
    QTRY_VERIFY(keyboard->property("active").toBool());
}

void testWebViewKeyboard::dpadReachesTheAddressFieldFromLoad() {
    // init() left the focus on Load, where the page opens. The field is left of it: left reaches it, and the
    // keyboard comes up like on the Wi-Fi forms (keyboardFollowsFocus)
    QTest::keyClick(view, Qt::Key_Left);
    QTRY_VERIFY(field->hasActiveFocus());
    QTRY_VERIFY(keyboard->property("active").toBool());

    // right at the end of the text goes back to Load, which hides the keyboard
    QTest::keyClick(view, Qt::Key_Right);
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QTRY_VERIFY(!keyboard->property("active").toBool());

    // up from Load reaches the field as well (nothing else is above Load)
    QTest::keyClick(view, Qt::Key_Up);
    QTRY_VERIFY(field->hasActiveFocus());
    QTRY_VERIFY(keyboard->property("active").toBool());

    // left inside the field moves the caret and stays, up has nowhere to go
    QTest::keyClick(view, Qt::Key_Left);
    QTest::keyClick(view, Qt::Key_Up);
    QTest::qWait(50);
    QVERIFY(field->hasActiveFocus());
    QCOMPARE(field->property("cursorPosition").toInt(), field->property("text").toString().size() - 1);

    // DPAD_MIDDLE is the field's Return: it loads the address and puts the keyboard away, the InputField keeps the
    // focus
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("load_uri(http://localhost/)")});
    QTRY_VERIFY(!keyboard->property("active").toBool());
    QVERIFY(fieldFrame->hasActiveFocus());
    QCOMPARE(ui->input()->activeItem(), page);

    // from there the d-pad goes on to Load without the keyboard
    QTest::keyClick(view, Qt::Key_Right);
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QTest::qWait(300);
    QVERIFY(!keyboard->property("active").toBool());
}

void testWebViewKeyboard::dpadWalksBothRowsAndIntoTheWebView() {
    // the field has the room the buttons left
    QVERIFY2(field->parentItem()->width() > 300, qPrintable(QString::number(field->parentItem()->width())));

    // a fresh view has no history: down from Load reaches Stop/Reload, Back and Forward are skipped
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(stopReloadButton->hasActiveFocus());
    QVERIFY(!keyboard->property("active").toBool());
    QTest::keyClick(view, Qt::Key_Left);
    QTest::qWait(50);
    QVERIFY(stopReloadButton->hasActiveFocus());

    // down again: the web view, where the arrows belong to the page (forwarded and accepted): the focus stays
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(webView->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Up);
    QTest::qWait(50);
    QVERIFY(webView->hasActiveFocus());

    // with history in both directions the second row walks Back <-> Forward <-> Stop/Reload
    lib::setHistory(true, true, "http://localhost/2");
    stopReloadButton->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Left);
    QTRY_VERIFY(forwardButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Left);
    QTRY_VERIFY(backButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Left);
    QTest::qWait(50);
    QVERIFY(backButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Right);
    QTRY_VERIFY(forwardButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Right);
    QTRY_VERIFY(stopReloadButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Right);
    QTest::qWait(50);
    QVERIFY(stopReloadButton->hasActiveFocus());

    // up from every button of the row: the address field above them
    for (QQuickItem *button : {backButton, forwardButton, stopReloadButton}) {
        button->forceActiveFocus();
        QTest::keyClick(view, Qt::Key_Up);
        QTRY_VERIFY2(field->hasActiveFocus(), qPrintable(button->objectName()));
    }
    QTRY_VERIFY(keyboard->property("active").toBool());

    // down from the field: the first button of the row (the keyboard goes), down again: the web view
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(backButton->hasActiveFocus());
    QTRY_VERIFY(!keyboard->property("active").toBool());
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(webView->hasActiveFocus());
    forwardButton->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(webView->hasActiveFocus());
}

void testWebViewKeyboard::backAndForwardFollowTheHistory() {
    // square, as large as Stop/Reload, dimmed and disabled without history: a tap does nothing
    for (QQuickItem *button : {backButton, forwardButton}) {
        QCOMPARE(button->width(), button->height());
        QCOMPARE(button->height(), stopReloadButton->height());
        QVERIFY(!button->isEnabled());
        QVERIFY(button->opacity() < 0.5);
        click(button);
    }
    QVERIFY(lib::calls.isEmpty());

    // a link was followed: Back works, by touch and by DPAD_MIDDLE (Return on the focused button)
    lib::setHistory(true, false, "http://localhost/2");
    QVERIFY(backButton->isEnabled());
    QCOMPARE(backButton->opacity(), 1.0);
    QVERIFY(!forwardButton->isEnabled());
    click(backButton);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_back")});
    stopReloadButton->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Left);  // over the disabled Forward
    QTRY_VERIFY(backButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("go_back"), QStringLiteral("go_back")}));

    // WebKit went back to the first page: Forward works
    lib::setHistory(false, true, "http://localhost/");
    QVERIFY(!backButton->isEnabled());
    QVERIFY(forwardButton->isEnabled());
    lib::calls.clear();
    click(forwardButton);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_forward")});

    // and the field's down goes to the first enabled button
    field->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(forwardButton->hasActiveFocus());
}

void testWebViewKeyboard::disabledHistoryButtonHandsTheFocusOn() {
    // DPAD_MIDDLE on Back until the first page: Back turns disabled under the d-pad and the focus moves to Forward
    lib::setHistory(true, false, "http://localhost/2");
    backButton->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Return);
    lib::setHistory(false, true, "http://localhost/");
    QTRY_VERIFY(forwardButton->hasActiveFocus());

    // and back the other way
    QTest::keyClick(view, Qt::Key_Return);
    lib::setHistory(true, false, "http://localhost/2");
    QTRY_VERIFY(backButton->hasActiveFocus());

    // no history left at all (a new view): Stop/Reload
    lib::setHistory(false, false, "http://localhost/3");
    QTRY_VERIFY(stopReloadButton->hasActiveFocus());
    QTest::qWait(50);
    QVERIFY(stopReloadButton->hasActiveFocus());
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("go_back"), QStringLiteral("go_forward")}));
}

void testWebViewKeyboard::addressFieldShowsThePageAddress() {
    // a redirect, Back or Forward: the field shows the address of the page on screen
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED, "http://localhost/a");
    QCOMPARE(field->property("text").toString(), QStringLiteral("http://localhost/a"));
    lib::fireLoad(UC_WEBVIEW_LOAD_REDIRECTED, "https://localhost/b");
    QCOMPARE(field->property("text").toString(), QStringLiteral("https://localhost/b"));

    // but not while the user types into it
    tapTheAddressField();
    QTRY_VERIFY(keyboard->property("active").toBool());
    field->setProperty("text", QStringLiteral("example.com"));
    lib::fireLoad(UC_WEBVIEW_LOAD_COMMITTED, "https://localhost/c");
    QCOMPARE(field->property("text").toString(), QStringLiteral("example.com"));

    // Load (a tap, the InputField keeps the focus): the keyboard goes and the field shows the completed address
    click(loadButton);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("load_uri(http://example.com)")});
    QTRY_VERIFY(!keyboard->property("active").toBool());
    QCOMPARE(field->property("text").toString(), QStringLiteral("http://example.com"));
    QVERIFY(fieldFrame->hasActiveFocus());

    // a refused address stays in the field
    field->setProperty("text", QStringLiteral("ftp://host/f"));
    click(loadButton);
    QCOMPARE(lib::calls.size(), 1);
    QCOMPARE(field->property("text").toString(), QStringLiteral("ftp://host/f"));
}

void testWebViewKeyboard::stopReloadButtonFollowsTheLoad() {
    QCOMPARE(stopReloadIcon->property("icon").toString(), QStringLiteral("uc:arrow-rotate-right"));
    QCOMPARE(stopReloadButton->height(), 60.0);
    QCOMPARE(stopReloadButton->width(), stopReloadButton->height());

    // idle: the button reloads (DPAD_MIDDLE is Key_Return on the focused button)
    stopReloadButton->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("reload")});

    // a load runs: the same button stops it
    lib::fireLoad(UC_WEBVIEW_LOAD_STARTED);
    QVERIFY(webView->isLoading());
    QCOMPARE(stopReloadIcon->property("icon").toString(), QStringLiteral("uc:xmark"));
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("reload"), QStringLiteral("stop")}));

    lib::fireLoad(UC_WEBVIEW_LOAD_FINISHED);
    QCOMPARE(stopReloadIcon->property("icon").toString(), QStringLiteral("uc:arrow-rotate-right"));
    QTest::keyClick(view, Qt::Key_Return);
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("reload"), QStringLiteral("stop"), QStringLiteral("reload")}));
}

void testWebViewKeyboard::addressesAreNormalized() {
    auto normalized = [this](const QString &address) {
        QVariant result;
        if (!QMetaObject::invokeMethod(page, "normalizedAddress", Q_RETURN_ARG(QVariant, result),
                                       Q_ARG(QVariant, address))) {
            return QStringLiteral("(invoke failed)");
        }
        return result.toString();
    };
    QCOMPARE(normalized("/app/data/emoji.html"), QStringLiteral("file:///app/data/emoji.html"));
    QCOMPARE(normalized("file:///app/data/emoji.html"), QStringLiteral("file:///app/data/emoji.html"));
    QCOMPARE(normalized("localhost:8080/x"), QStringLiteral("http://localhost:8080/x"));
    QCOMPARE(normalized("example.com"), QStringLiteral("http://example.com"));
    QCOMPARE(normalized("HTTPS://example.com/"), QStringLiteral("HTTPS://example.com/"));
    QCOMPARE(normalized("about:blank"), QStringLiteral("about:blank"));
    QCOMPARE(normalized("data:text/html,<p>hi</p>"), QStringLiteral("data:text/html,<p>hi</p>"));
    QCOMPARE(normalized("ftp://host/f"), QStringLiteral("ftp://host/f"));  // the item refuses it with loadFailed
}

void testWebViewKeyboard::tapOnTheFocusedPageInputReopensTheKeyboardAfterTheHideKey() {
    // the user taps the PIN field of a web page: the press gives the web view the focus, WebKit reports the input
    tapTheWebView();
    QTRY_VERIFY(webView->hasActiveFocus());
    lib::fireImeFocus(1, UC_WEBVIEW_IME_PURPOSE_PIN);
    QTRY_VERIFY(keyboard->property("active").toBool());
    QTRY_VERIFY(QGuiApplication::inputMethod()->isVisible());

    // the hide key closes it; the page's input keeps the DOM focus and the web view the item focus
    pressTheHideKey();
    QTRY_COMPARE(closed->count(), 1);
    QVERIFY(!keyboard->property("active").toBool());
    QVERIFY(!QGuiApplication::inputMethod()->isVisible());
    QVERIFY(webView->hasActiveFocus());

    // a scroll is not a tap: the keyboard stays closed
    tapTheWebView(QPointF(0, 40));
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 200);
    QVERIFY(!keyboard->property("active").toBool());

    // a tap on the input again: WebKit reports nothing (the focus did not change), the item asks by itself and
    // main.qml's controller opens the keyboard through Qt.inputMethod.visible
    const int openedBefore = opened->count();
    tapTheWebView();
    QTRY_VERIFY(keyboard->property("active").toBool());
    QVERIFY(opened->count() > openedBefore);
    QVERIFY(webView->hasActiveFocus());

    // and the next tap while it is up changes nothing (no close/open, which would reset the keyboard's layout)
    const int closedBefore = closed->count();
    tapTheWebView();
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 200);
    QCOMPARE(closed->count(), closedBefore);
    QVERIFY(keyboard->property("active").toBool());
}

void testWebViewKeyboard::prevAndNextStepThroughTheHistoryFromEveryControl() {
    // history in both directions: PREV goes back, NEXT forward, whichever control the d-pad is on (the field is left
    // out: the d-pad brings the keyboard up there, see prevAndNextAreIgnoredWhileTheKeyboardIsUp)
    lib::setHistory(true, true, "http://localhost/2");
    QStringList expected;
    QQuickItem *const webViewItem = webView;
    for (QQuickItem *item : {loadButton, backButton, forwardButton, stopReloadButton, webViewItem}) {
        item->forceActiveFocus();
        QTRY_VERIFY(item->hasActiveFocus());
        QVERIFY(!keyboard->property("active").toBool());

        QVERIFY2(!pressRemoteButton(Qt::Key_AudioRewind), qPrintable(item->objectName()));
        expected << QStringLiteral("go_back");
        QCOMPARE(lib::calls, expected);
        QVERIFY2(!pressRemoteButton(Qt::Key_AudioForward), qPrintable(item->objectName()));
        expected << QStringLiteral("go_forward");
        QCOMPARE(lib::calls, expected);

        // the keys change nothing else: the focus stays where it was, the page gets no key
        QVERIFY(item->hasActiveFocus());
        QVERIFY(lib::keys.isEmpty());
    }
    QCOMPARE(ui->input()->activeItem(), page);
}

void testWebViewKeyboard::prevAndNextWithoutHistoryDoNothing() {
    // a fresh view: no history at all, neither key calls the library
    webView->forceActiveFocus();
    QVERIFY(!pressRemoteButton(Qt::Key_AudioRewind));
    QVERIFY(!pressRemoteButton(Qt::Key_AudioForward));
    QVERIFY(lib::calls.isEmpty());

    // only back: NEXT does nothing, PREV goes back
    lib::setHistory(true, false, "http://localhost/2");
    pressRemoteButton(Qt::Key_AudioForward);
    QVERIFY(lib::calls.isEmpty());
    pressRemoteButton(Qt::Key_AudioRewind);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_back")});

    // WebKit is on the first entry again: PREV does nothing, NEXT goes forward
    lib::setHistory(false, true, "http://localhost/");
    lib::calls.clear();
    pressRemoteButton(Qt::Key_AudioRewind);
    QVERIFY(lib::calls.isEmpty());
    pressRemoteButton(Qt::Key_AudioForward);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_forward")});
    QVERIFY(lib::keys.isEmpty());
}

void testWebViewKeyboard::prevAndNextPassTheFocusedWebView() {
    // the d-pad is in the web view, where the arrows belong to the page
    lib::setHistory(true, true, "http://localhost/2");
    tapTheWebView();
    QTRY_VERIFY(webView->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Down);
    QCOMPARE(lib::keys, QList<uint32_t>{0xff54});  // WPE_KEY_Down
    lib::keys.clear();

    // PREV/NEXT as the device sends them: not accepted by the item, not forwarded, handled by the page
    QVERIFY(!pressRemoteButton(Qt::Key_AudioRewind));
    QVERIFY(!pressRemoteButton(Qt::Key_AudioForward));
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("go_back"), QStringLiteral("go_forward")}));
    QVERIFY(lib::keys.isEmpty());
    QVERIFY(webView->hasActiveFocus());

    // the button simulator sends the key's name as text ("Media Rewind"): still not a character for the page
    ui->input()->pressSimulatorKey(Qt::Key_AudioRewind);
    ui->input()->releaseSimulatorKey(Qt::Key_AudioRewind);
    ui->input()->pressSimulatorKey(Qt::Key_AudioForward);
    ui->input()->releaseSimulatorKey(Qt::Key_AudioForward);
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("go_back"), QStringLiteral("go_forward"),
                                      QStringLiteral("go_back"), QStringLiteral("go_forward")}));
    QVERIFY(lib::keys.isEmpty());

    // BACK leaves the page from inside the web view (Settings.Page: profileRoot.goBack()), also not forwarded.
    // Last: the BACK handler restores the page's original button configuration.
    QObject *profileRoot = view->rootObject()->findChild<QObject *>("profileRoot");
    QVERIFY(profileRoot);
    QVERIFY(!pressRemoteButton(Qt::Key_Exit));
    QCOMPARE(profileRoot->property("backCalls").toInt(), 1);
    QVERIFY(lib::keys.isEmpty());
}

void testWebViewKeyboard::heldPrevStepsOnce() {
    // a held button repeats every 150 ms on the device: still one step, every step is a navigation
    lib::setHistory(true, true, "http://localhost/2");
    webView->forceActiveFocus();
    pressRemoteButton(Qt::Key_AudioRewind, 5);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_back")});
    pressRemoteButton(Qt::Key_AudioForward, 5);
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("go_back"), QStringLiteral("go_forward")}));

    // and the next press steps again
    pressRemoteButton(Qt::Key_AudioRewind);
    QCOMPARE(lib::calls,
             (QStringList{QStringLiteral("go_back"), QStringLiteral("go_forward"), QStringLiteral("go_back")}));
    QVERIFY(lib::keys.isEmpty());
}

void testWebViewKeyboard::prevAndNextAreIgnoredWhileTheKeyboardIsUp() {
    lib::setHistory(true, true, "http://localhost/2");

    // typing into the address field
    tapTheAddressField();
    QTRY_VERIFY(keyboard->property("active").toBool());
    QVERIFY(!pressRemoteButton(Qt::Key_AudioRewind));
    QVERIFY(!pressRemoteButton(Qt::Key_AudioForward));
    QVERIFY(lib::calls.isEmpty());
    QVERIFY(field->hasActiveFocus());
    QVERIFY(keyboard->property("active").toBool());

    // typing into an input of the page
    tapTheWebView();
    QTRY_VERIFY(webView->hasActiveFocus());
    lib::fireImeFocus(1, UC_WEBVIEW_IME_PURPOSE_FREE_FORM);
    QTRY_VERIFY(keyboard->property("active").toBool());
    pressRemoteButton(Qt::Key_AudioRewind);
    pressRemoteButton(Qt::Key_AudioForward);
    QVERIFY(lib::calls.isEmpty());
    QVERIFY(lib::keys.isEmpty());

    // the keyboard is put away: the keys work again
    pressTheHideKey();
    QTRY_VERIFY(!keyboard->property("active").toBool());
    pressRemoteButton(Qt::Key_AudioRewind);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("go_back")});
}

void testWebViewKeyboard::volumeZoomsThePageFromEveryControl() {
    // history in both directions, so that Back and Forward are enabled and the d-pad can be on them
    lib::setHistory(true, true, "http://localhost/2");
    QStringList expected;
    QQuickItem *const webViewItem = webView;
    for (QQuickItem *item : {loadButton, backButton, forwardButton, stopReloadButton, webViewItem}) {
        item->forceActiveFocus();
        QTRY_VERIFY(item->hasActiveFocus());
        QVERIFY(!keyboard->property("active").toBool());

        // VOLUME+ one level in, VOLUME- back out: the item's zoom, set in the library once the page rendered the step
        QVERIFY2(!pressRemoteButton(Qt::Key_VolumeUp), qPrintable(item->objectName()));
        expected << QStringLiteral("set_zoom(1.1)");
        QCOMPARE(lib::calls, expected);
        QCOMPARE(webView->zoom(), 1.1);
        lib::finishRender();
        QVERIFY2(!pressRemoteButton(Qt::Key_VolumeDown), qPrintable(item->objectName()));
        expected << QStringLiteral("set_zoom(1)");
        QCOMPARE(lib::calls, expected);
        QCOMPARE(webView->zoom(), 1.0);
        lib::finishRender();
        QVERIFY(!webView->isZoomRendering());

        // nothing else changes: the focus stays, the page gets no key
        QVERIFY(item->hasActiveFocus());
        QVERIFY(lib::keys.isEmpty());
    }
    QCOMPARE(ui->input()->activeItem(), page);
}

void testWebViewKeyboard::heldVolumeZoomsOneLevel() {
    // a held button repeats every 150 ms on the device: one level per press, every level is a relayout
    webView->forceActiveFocus();
    pressRemoteButton(Qt::Key_VolumeUp, 5);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("set_zoom(1.1)")});
    lib::finishRender();
    pressRemoteButton(Qt::Key_VolumeDown, 5);
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("set_zoom(1.1)"), QStringLiteral("set_zoom(1)")}));
    QCOMPARE(webView->zoom(), 1.0);
}

void testWebViewKeyboard::zoomLabelShowsTheLevelAndFades() {
    QQuickItem *label = page->findChild<QQuickItem *>("zoomLabel");
    QObject    *text = page->findChild<QObject *>("zoomText");
    QVERIFY(label && text);
    QVERIFY(!label->isVisible());

    // the new level at once; the page renders it quickly: the label fades about a second after the press
    pressRemoteButton(Qt::Key_VolumeUp);
    QTRY_VERIFY(label->isVisible());
    QCOMPARE(text->property("text").toString(), QStringLiteral("110 %"));
    lib::finishRender();
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 3000);

    // down to 67 % (the zoom is 0.67), one rendered step per press
    for (int i = 0; i < 5; ++i) {
        pressRemoteButton(Qt::Key_VolumeDown);
        lib::finishRender();
    }
    QCOMPARE(webView->zoom(), 0.67);
    QCOMPARE(text->property("text").toString(), QStringLiteral("67 %"));
    QVERIFY(label->isVisible());

    // at the first level a press calls nothing, the label shows that the zoom ends there
    pressRemoteButton(Qt::Key_VolumeDown);
    lib::finishRender();
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 3000);
    const int calls = lib::calls.size();
    pressRemoteButton(Qt::Key_VolumeDown);
    QCOMPARE(lib::calls.size(), calls);
    QVERIFY(lib::renders.isEmpty());
    QCOMPARE(text->property("text").toString(), QStringLiteral("50 %"));
    QTRY_VERIFY(label->isVisible());
}

void testWebViewKeyboard::zoomLabelStaysWithASpinnerWhileThePageRenders() {
    // a large page takes seconds to render a zoom step: the level stays up with the app's
    // spinner next to it until the page has rendered, then the label fades as after a quick step
    QQuickItem *label = page->findChild<QQuickItem *>("zoomLabel");
    QQuickItem *spinner = page->findChild<QQuickItem *>("zoomSpinner");
    QVERIFY(label && spinner);
    QVERIFY(!spinner->isVisible());

    QElapsedTimer clock;
    clock.start();
    pressRemoteButton(Qt::Key_VolumeUp);
    QVERIFY(webView->isZoomRendering());
    QTRY_VERIFY(label->isVisible());
    // the spinner comes after the page's zoomSpinnerDelay (150 ms), not before
    QTRY_VERIFY_WITH_TIMEOUT(spinner->isVisible(), 1000);
    QVERIFY(clock.elapsed() >= page->property("zoomSpinnerDelay").toInt());
    // longer than the label's second: it stays while the page renders
    QTest::qWait(1500);
    QVERIFY(label->isVisible());
    QCOMPARE(label->opacity(), 1.0);
    QVERIFY(spinner->isVisible());

    // rendered: the spinner goes at once, the label fades
    lib::finishRender();
    QVERIFY(!webView->isZoomRendering());
    QVERIFY(!spinner->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 1000);

    // a timeout of the library ends the wait as well
    pressRemoteButton(Qt::Key_VolumeUp);
    QTRY_VERIFY_WITH_TIMEOUT(spinner->isVisible(), 1000);
    lib::finishRender(UC_WEBVIEW_RENDERED_TIMEOUT);
    QVERIFY(!spinner->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 3000);
}

void testWebViewKeyboard::quickZoomStepsShowNoSpinner() {
    // rendered within the spinner's delay: no spinner flickers up, the label shows the level for its second
    QQuickItem *label = page->findChild<QQuickItem *>("zoomLabel");
    QQuickItem *spinner = page->findChild<QQuickItem *>("zoomSpinner");
    QVERIFY(label && spinner);
    bool spinnerSeen = false;
    QObject::connect(spinner, &QQuickItem::visibleChanged, spinner, [&] { spinnerSeen |= spinner->isVisible(); });

    for (int i = 0; i < 3; ++i) {
        pressRemoteButton(Qt::Key_VolumeUp);
        QTest::qWait(50);
        lib::finishRender();
    }
    QTest::qWait(300);
    QVERIFY(!spinnerSeen);
    QVERIFY(label->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 3000);
    QVERIFY(!spinnerSeen);
}

void testWebViewKeyboard::quickPressesDuringARenderCostTwoRelayouts() {
    // three quick VOLUME+ on a large page: the level follows every press, WebKit gets the first and the last level
    QObject *text = page->findChild<QObject *>("zoomText");
    QVERIFY(text);
    pressRemoteButton(Qt::Key_VolumeUp);
    pressRemoteButton(Qt::Key_VolumeUp);
    QCOMPARE(text->property("text").toString(), QStringLiteral("125 %"));
    pressRemoteButton(Qt::Key_VolumeUp);
    QCOMPARE(text->property("text").toString(), QStringLiteral("150 %"));
    QCOMPARE(lib::calls, QStringList{QStringLiteral("set_zoom(1.1)")});

    lib::finishRender();
    QCOMPARE(lib::calls, (QStringList{QStringLiteral("set_zoom(1.1)"), QStringLiteral("set_zoom(1.5)")}));
    QVERIFY(webView->isZoomRendering());
    lib::finishRender();
    QVERIFY(!webView->isZoomRendering());
    QVERIFY(lib::renders.isEmpty());
    QCOMPARE(lib::calls.size(), 2);
}

void testWebViewKeyboard::channelScrollsThePageFromEveryControl() {
    lib::setHistory(true, true, "http://localhost/2");
    QList<uint32_t>   expected;
    QQuickItem *const webViewItem = webView;
    for (QQuickItem *item : {loadButton, backButton, forwardButton, stopReloadButton, webViewItem}) {
        item->forceActiveFocus();
        QTRY_VERIFY(item->hasActiveFocus());

        // CHANNEL- a page down, CHANNEL+ a page up: Page_Down and Page_Up for the page
        QVERIFY2(!pressRemoteButton(Qt::Key_ChannelDown), qPrintable(item->objectName()));
        expected << 0xff56;  // WPE_KEY_Page_Down
        QCOMPARE(lib::keys, expected);
        QVERIFY2(!pressRemoteButton(Qt::Key_ChannelUp), qPrintable(item->objectName()));
        expected << 0xff55;  // WPE_KEY_Page_Up
        QCOMPARE(lib::keys, expected);

        // the focus stays, no navigation, no zoom
        QVERIFY(item->hasActiveFocus());
        QVERIFY(lib::calls.isEmpty());
    }
}

void testWebViewKeyboard::heldChannelScrollsOnEveryRepeat() {
    // holding CHANNEL scrolls on: the press and every repeat are a page
    webView->forceActiveFocus();
    pressRemoteButton(Qt::Key_ChannelDown, 3);
    QCOMPARE(lib::keys, (QList<uint32_t>{0xff56, 0xff56, 0xff56, 0xff56}));
    lib::keys.clear();
    pressRemoteButton(Qt::Key_ChannelUp, 2);
    QCOMPARE(lib::keys, (QList<uint32_t>{0xff55, 0xff55, 0xff55}));
}

void testWebViewKeyboard::volumeAndChannelAreIgnoredWhileTheKeyboardIsUp() {
    QQuickItem *label = page->findChild<QQuickItem *>("zoomLabel");
    QVERIFY(label);
    const Qt::Key keys[] = {Qt::Key_VolumeUp, Qt::Key_VolumeDown, Qt::Key_ChannelUp, Qt::Key_ChannelDown};

    // typing into the address field
    tapTheAddressField();
    QTRY_VERIFY(keyboard->property("active").toBool());
    for (Qt::Key key : keys) {
        pressRemoteButton(key);
    }
    QVERIFY(lib::calls.isEmpty());
    QVERIFY(lib::keys.isEmpty());
    QVERIFY(field->hasActiveFocus());
    QVERIFY(keyboard->property("active").toBool());

    // typing into an input of the page
    tapTheWebView();
    QTRY_VERIFY(webView->hasActiveFocus());
    lib::fireImeFocus(1, UC_WEBVIEW_IME_PURPOSE_FREE_FORM);
    QTRY_VERIFY(keyboard->property("active").toBool());
    for (Qt::Key key : keys) {
        pressRemoteButton(key);
    }
    QVERIFY(lib::calls.isEmpty());
    QVERIFY(lib::keys.isEmpty());
    QCOMPARE(webView->zoom(), 1.0);
    QVERIFY(!label->isVisible());

    // the keyboard is put away: the keys work again
    pressTheHideKey();
    QTRY_VERIFY(!keyboard->property("active").toBool());
    pressRemoteButton(Qt::Key_VolumeUp);
    QCOMPARE(lib::calls, QStringList{QStringLiteral("set_zoom(1.1)")});
    pressRemoteButton(Qt::Key_ChannelDown);
    QCOMPARE(lib::keys, QList<uint32_t>{0xff56});
}

void testWebViewKeyboard::pinchShowsTheMagnificationNextToTheZoom() {
    // libucwebview 0.6.0: the label shows the magnification while the fingers move, stays while the page renders it
    // (with the spinner), and shows both factors once the page zoom differs from 100 % as well
    QQuickItem *label = page->findChild<QQuickItem *>("zoomLabel");
    QObject    *text = page->findChild<QObject *>("zoomText");
    QQuickItem *spinner = page->findChild<QQuickItem *>("zoomSpinner");
    QVERIFY(label && text && spinner);
    QVERIFY(webView->isPinchZoomAvailable());
    QVERIFY(!label->isVisible());

    lib::firePinch(UC_WEBVIEW_PINCH_BEGIN, 1.0);
    lib::firePinch(UC_WEBVIEW_PINCH_UPDATE, 1.5);
    QTRY_VERIFY(label->isVisible());
    QCOMPARE(text->property("text").toString(), QStringLiteral("1.5\u00d7"));
    lib::firePinch(UC_WEBVIEW_PINCH_UPDATE, 2.0);
    QCOMPARE(text->property("text").toString(), QStringLiteral("2.0\u00d7"));
    // a long pinch: the label stays while the fingers are down
    QTest::qWait(1200);
    QVERIFY(label->isVisible());

    lib::firePinch(UC_WEBVIEW_PINCH_END, 2.0);
    QCOMPARE(lib::calls.filter(QStringLiteral("set_magnification(")),
             QStringList{QStringLiteral("set_magnification(2)")});
    QVERIFY(webView->isZoomRendering());
    QTRY_VERIFY_WITH_TIMEOUT(spinner->isVisible(), 1000);
    lib::finishRender();
    QVERIFY(!spinner->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 3000);

    // VOLUME zooms the page and keeps the magnification: both in the label
    pressRemoteButton(Qt::Key_VolumeUp);
    QTRY_VERIFY(label->isVisible());
    QCOMPARE(text->property("text").toString(), QStringLiteral("110 % \u00b7 2.0\u00d7"));
    QCOMPARE(webView->magnification(), 2.0);
    lib::finishRender();
}

int main(int argc, char *argv[]) {
    // the app's input method (main.cpp); offscreen unless the caller chose a platform (ctest sets offscreen too)
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    // the window has to be shown for the focus and the input method; a build host has no OpenGL context for the
    // offscreen platform, and the page has no frames to show, so the software scene graph does
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) {
        qputenv("QT_QUICK_BACKEND", QByteArray("software"));
    }
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    WebViewLibrary::instance()->setFunctionsForTest(lib::functions());
    WebViewItem::registerQmlTypes();
    qmlRegisterSingletonType<HapticStub>("Haptic", 1, 0, "Haptic", &HapticStub::qmlInstance);

    testWebViewKeyboard test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "test_webview_keyboard.moc"
