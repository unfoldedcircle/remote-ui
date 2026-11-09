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
//
// Regression 3: after the hide key, a tap on the web page's input that still had the DOM focus did not bring the
// keyboard back: WebKit reports the element's focus only when it changes. The WebView item asks for the keyboard
// again on such a tap; this checks that the request reaches main.qml's controller through the real input context.

#include <QJSValue>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

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
QStringList          calls;  // navigation requests: "reload", "stop"

int         apiVersion() { return UC_WEBVIEW_API_VERSION; }
int         globalInit(void *, const char *, const char *) { return 0; }
void        globalShutdown() {}
uc_webview *create(int, int, const uc_webview_callbacks *cb, void *u) {
    callbacks = *cb;
    user = u;
    return kView;
}
void        destroy(uc_webview *) {}
void        resize(uc_webview *, int, int) {}
void        setInt(uc_webview *, int) {}
void        setDouble(uc_webview *, double) {}
void        setString(uc_webview *, const char *) {}
void        loadHtml(uc_webview *, const char *, const char *) {}
void        noArgs(uc_webview *) {}
void        reload(uc_webview *) { calls.append(QStringLiteral("reload")); }
void        stop(uc_webview *) { calls.append(QStringLiteral("stop")); }
int         no(uc_webview *) { return 0; }
const char *none(uc_webview *) { return nullptr; }
void        setBackground(uc_webview *, float, float, float, float) {}
void        frame(uc_webview *, void *) {}
void        touch(uc_webview *, int, int, double, double, uint32_t) {}
void        pointerButton(uc_webview *, int, int, double, double, unsigned, uint32_t) {}
void        pointerMove(uc_webview *, double, double, unsigned, uint32_t) {}
void        scroll(uc_webview *, double, double, double, double, int, uint32_t) {}
void        key(uc_webview *, uint32_t, uint32_t, int, unsigned, uint32_t) {}
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
    f.load_uri = &setString;
    f.load_html = &loadHtml;
    f.reload = &reload;
    f.stop = &stop;
    f.go_back = &noArgs;
    f.go_forward = &noArgs;
    f.can_go_back = &no;
    f.can_go_forward = &no;
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

void fireImeFocus(int focused, unsigned purpose) {
    callbacks.ime_focus(kView, focused, UC_WEBVIEW_IME_HINT_NONE, purpose, "", 0, user);
}

void fireLoad(int event) {
    callbacks.load_changed(kView, event, "http://localhost/", user);
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
    void dpadToTheLoadButtonHidesTheKeyboardAndBackUpShowsItAgain();
    void hideKeyClosesTheKeyboardAndItStaysClosed();
    void dpadWalksFieldLoadStopReloadWebView();
    void stopReloadButtonFollowsTheLoad();
    void addressesAreNormalized();
    void tapOnTheFocusedPageInputReopensTheKeyboardAfterTheHideKey();

 private:
    void tapTheAddressField();
    void tapTheWebView(const QPointF &moveBy = QPointF());
    void pressTheHideKey();

    QQuickView  *view = nullptr;
    UiStub      *ui = nullptr;
    ResourceStub resource;
    Colors       colors;
    Fonts        fonts;
    QQuickItem  *page = nullptr;
    QQuickItem  *field = nullptr;
    QQuickItem  *loadButton = nullptr;
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
    loadButton = buttonOf(page, "Load");
    stopReloadButton = page->findChild<QQuickItem *>("stopReloadButton");
    stopReloadIcon = page->findChild<QQuickItem *>("stopReloadIcon");
    QVERIFY(field && loadButton && stopReloadButton && stopReloadIcon);
    QVERIFY(!page->findChildren<WebViewItem *>().isEmpty());
    webView = page->findChildren<WebViewItem *>().first();
    lib::calls.clear();

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

void testWebViewKeyboard::dpadToTheLoadButtonHidesTheKeyboardAndBackUpShowsItAgain() {
    tapTheAddressField();
    QTRY_VERIFY(keyboard->property("active").toBool());

    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QTRY_COMPARE(closed->count(), 1);
    QTest::qWait(800);
    QCOMPARE(closed->count(), 1);
    QVERIFY(!keyboard->property("active").toBool());
    QVERIFY(loadButton->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Up);
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
    // not Qt.inputMethod.hide(). The field keeps the focus (InputField.qml onClosed); dropping it made the
    // ButtonNavigation hand it back and TextInput's focus gain reopened the keyboard through Qt.inputMethod.visible.
    QVERIFY(QMetaObject::invokeMethod(keyboard, "hide"));
    QTRY_COMPARE(closed->count(), 1);
    QTest::qWait(800);
    QCOMPARE(closed->count(), 1);
    QVERIFY(!keyboard->property("active").toBool());
    QVERIFY(field->hasActiveFocus());
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
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QVERIFY(!keyboard->property("active").toBool());
    QTest::keyClick(view, Qt::Key_Up);
    QTRY_VERIFY(field->hasActiveFocus());
    QTRY_VERIFY(keyboard->property("active").toBool());
}

void testWebViewKeyboard::dpadWalksFieldLoadStopReloadWebView() {
    // init() left the focus on Load; down: Stop/Reload, then the web view
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(stopReloadButton->hasActiveFocus());
    QVERIFY(!keyboard->property("active").toBool());
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(webView->hasActiveFocus());

    // inside the web view the arrows belong to the page (they are forwarded and accepted): the focus stays
    QTest::keyClick(view, Qt::Key_Up);
    QTest::qWait(50);
    QVERIFY(webView->hasActiveFocus());

    // back up from the button row: Stop/Reload -> Load -> address field (which brings the keyboard)
    stopReloadButton->forceActiveFocus();
    QTest::keyClick(view, Qt::Key_Up);
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Right);
    QTRY_VERIFY(stopReloadButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Left);
    QTRY_VERIFY(loadButton->hasActiveFocus());
    QTest::keyClick(view, Qt::Key_Up);
    QTRY_VERIFY(field->hasActiveFocus());
    QTRY_VERIFY(keyboard->property("active").toBool());
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_VERIFY(loadButton->hasActiveFocus());
}

void testWebViewKeyboard::stopReloadButtonFollowsTheLoad() {
    QCOMPARE(stopReloadIcon->property("icon").toString(), QStringLiteral("uc:arrow-rotate-right"));
    QCOMPARE(stopReloadButton->height(), loadButton->height());
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
