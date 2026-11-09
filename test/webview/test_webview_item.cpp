// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QQuickWindow>
#include <QScreen>
#include <QtTest>
#include <qpa/qwindowsysteminterface.h>

#include "webview/webviewitem.h"
#include "webview/webviewlibrary.h"

using uc::webview::WebViewFunctions;
using uc::webview::WebViewItem;
using uc::webview::WebViewLibrary;

// A fake libucwebview: records every call and lets the test fire the callbacks the real library would deliver
// from the GLib main context on the GUI thread.
namespace stub {

struct State {
    int                  initCalls = 0;
    void                *eglDisplay = nullptr;
    uc_webview          *view = nullptr;  // the one live view (the test creates one item at a time)
    int                  width = 0;
    int                  height = 0;
    uc_webview_callbacks callbacks{};
    void                *user = nullptr;
    QStringList          calls;  // "name(args)" in order
    QStringList          panel;  // the item's QInputMethod show/hide requests
    bool                 canGoBack = false;
    bool                 canGoForward = false;
    QList<double>        refreshRates;           // uc_webview_global_set_refresh_rate() arguments
    int                  refreshRateResult = 0;  // what the stub returns (-1: refused)
};

State s;

uc_webview *const kView = reinterpret_cast<uc_webview *>(0x1234);

void record(const QString &call) {
    s.calls.append(call);
}

int apiVersion() {
    return UC_WEBVIEW_API_VERSION;
}

int globalInit(void *egl, const char *, const char *) {
    ++s.initCalls;
    s.eglDisplay = egl;
    return 0;
}

void globalShutdown() {}

int globalSetRefreshRate(double hz) {
    s.refreshRates.append(hz);
    record(QStringLiteral("global_set_refresh_rate(%1)").arg(hz));
    return s.refreshRateResult;
}

uc_webview *create(int w, int h, const uc_webview_callbacks *cb, void *user) {
    s.view = kView;
    s.width = w;
    s.height = h;
    s.callbacks = *cb;
    s.user = user;
    record(QStringLiteral("new(%1x%2)").arg(w).arg(h));
    return kView;
}

void destroy(uc_webview *v) {
    QCOMPARE(v, kView);
    s.view = nullptr;
    record(QStringLiteral("free"));
}

void resize(uc_webview *, int w, int h) {
    s.width = w;
    s.height = h;
    record(QStringLiteral("resize(%1x%2)").arg(w).arg(h));
}

void setMapped(uc_webview *, int mapped) {
    record(QStringLiteral("set_mapped(%1)").arg(mapped));
}

void setFocused(uc_webview *, int focused) {
    record(QStringLiteral("set_focused(%1)").arg(focused));
}

void setScale(uc_webview *, double scale) {
    record(QStringLiteral("set_scale(%1)").arg(scale));
}

void loadUri(uc_webview *, const char *uri) {
    record(QStringLiteral("load_uri(%1)").arg(QString::fromUtf8(uri)));
}

void loadHtml(uc_webview *, const char *html, const char *base) {
    record(QStringLiteral("load_html(%1,%2)").arg(QString::fromUtf8(html), base ? QString::fromUtf8(base) : "null"));
}

void reload(uc_webview *) {
    record(QStringLiteral("reload"));
}

void stop(uc_webview *) {
    record(QStringLiteral("stop"));
}

void goBack(uc_webview *) {
    record(QStringLiteral("go_back"));
}

void goForward(uc_webview *) {
    record(QStringLiteral("go_forward"));
}

int canGoBack(uc_webview *) {
    return s.canGoBack ? 1 : 0;
}

int canGoForward(uc_webview *) {
    return s.canGoForward ? 1 : 0;
}

const char *getUri(uc_webview *) {
    return nullptr;
}

const char *getTitle(uc_webview *) {
    return nullptr;
}

void runJavascript(uc_webview *, const char *script) {
    record(QStringLiteral("run_javascript(%1)").arg(QString::fromUtf8(script)));
}

void setZoom(uc_webview *, double) {}

void setBackground(uc_webview *, float r, float g, float b, float a) {
    record(QStringLiteral("set_background_rgba(%1,%2,%3,%4)").arg(r).arg(g).arg(b).arg(a));
}

void frameRendered(uc_webview *, void *handle) {
    record(QStringLiteral("frame_rendered(%1)").arg(reinterpret_cast<quintptr>(handle)));
}

void frameRelease(uc_webview *, void *handle) {
    record(QStringLiteral("frame_release(%1)").arg(reinterpret_cast<quintptr>(handle)));
}

void touch(uc_webview *, int id, int phase, double x, double y, uint32_t) {
    record(QStringLiteral("touch(%1,%2,%3,%4)").arg(id).arg(phase).arg(x).arg(y));
}

void pointerButton(uc_webview *, int button, int pressed, double x, double y, unsigned, uint32_t) {
    record(QStringLiteral("pointer_button(%1,%2,%3,%4)").arg(button).arg(pressed).arg(x).arg(y));
}

void pointerMove(uc_webview *, double x, double y, unsigned, uint32_t) {
    record(QStringLiteral("pointer_move(%1,%2)").arg(x).arg(y));
}

void scroll(uc_webview *, double dx, double dy, double, double, int precise, uint32_t) {
    record(QStringLiteral("scroll(%1,%2,%3)").arg(dx).arg(dy).arg(precise));
}

void key(uc_webview *, uint32_t keysym, uint32_t, int pressed, unsigned mods, uint32_t) {
    record(QStringLiteral("key(0x%1,%2,%3)").arg(keysym, 0, 16).arg(pressed).arg(mods));
}

void imeCommit(uc_webview *, const char *text) {
    record(QStringLiteral("ime_commit(%1)").arg(QString::fromUtf8(text)));
}

void imePreedit(uc_webview *, const char *text, int cursor) {
    record(QStringLiteral("ime_preedit(%1,%2)").arg(QString::fromUtf8(text)).arg(cursor));
}

void imeDeleteSurrounding(uc_webview *, int offset, int n) {
    record(QStringLiteral("ime_delete_surrounding(%1,%2)").arg(offset).arg(n));
}

WebViewFunctions functions() {
    WebViewFunctions f;
    f.api_version = &apiVersion;
    f.global_init = &globalInit;
    f.global_shutdown = &globalShutdown;
    f.global_set_refresh_rate = &globalSetRefreshRate;
    f.create = &create;
    f.destroy = &destroy;
    f.resize = &resize;
    f.set_mapped = &setMapped;
    f.set_focused = &setFocused;
    f.set_scale = &setScale;
    f.load_uri = &loadUri;
    f.load_html = &loadHtml;
    f.reload = &reload;
    f.stop = &stop;
    f.go_back = &goBack;
    f.go_forward = &goForward;
    f.can_go_back = &canGoBack;
    f.can_go_forward = &canGoForward;
    f.get_uri = &getUri;
    f.get_title = &getTitle;
    f.run_javascript = &runJavascript;
    f.set_zoom = &setZoom;
    f.set_background_rgba = &setBackground;
    f.frame_rendered = &frameRendered;
    f.frame_release = &frameRelease;
    f.touch = &touch;
    f.pointer_button = &pointerButton;
    f.pointer_move = &pointerMove;
    f.scroll = &scroll;
    f.key = &key;
    f.ime_commit = &imeCommit;
    f.ime_preedit = &imePreedit;
    f.ime_delete_surrounding = &imeDeleteSurrounding;
    return f;
}

// the library's callbacks, as the GLib main context would deliver them
void fireLoadChanged(int event, const char *uri) {
    s.callbacks.load_changed(s.view, event, uri, s.user);
}

void fireTitle(const char *title) {
    s.callbacks.title_changed(s.view, title, s.user);
}

void fireProgress(double progress) {
    s.callbacks.progress_changed(s.view, progress, s.user);
}

void fireTerminated(int reason) {
    s.callbacks.web_process_terminated(s.view, reason, s.user);
}

void fireImeFocus(int focused, unsigned hints, unsigned purpose, const char *surrounding, int cursor) {
    s.callbacks.ime_focus(s.view, focused, hints, purpose, surrounding, cursor, s.user);
}

void fireImeCursorArea(int x, int y, int w, int h) {
    s.callbacks.ime_cursor_area(s.view, x, y, w, h, s.user);
}

void fireImeSurrounding(const char *text, int cursor, int anchor) {
    s.callbacks.ime_surrounding(s.view, text, cursor, anchor, s.user);
}

void fireFrame(int number) {
    uc_webview_frame frame{};
    frame.width = s.width;
    frame.height = s.height;
    frame.handle = reinterpret_cast<void *>(static_cast<intptr_t>(number));
    s.callbacks.frame_ready(s.view, &frame, s.user);
}

}  // namespace stub

// the protected grab-loss hooks Qt calls when another item takes a grab
class ExposedItem : public WebViewItem {
 public:
    explicit ExposedItem(QQuickItem *parent) : WebViewItem(parent) {}
    using WebViewItem::mouseUngrabEvent;
    using WebViewItem::touchUngrabEvent;
};

// A WebView item inside a (never shown) window, like the QML page creates it. The window is never exposed, so the
// scene graph does not render and updatePaintNode() is not called: the test covers the GUI thread side.
struct Fixture {
    QQuickWindow window;
    ExposedItem *item;

    Fixture() {
        stub::s = stub::State();
        WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
        WebViewItem::setInputPanelHookForTest([](bool show) { stub::s.panel.append(show ? "show" : "hide"); });
        item = new ExposedItem(window.contentItem());
        item->setSize(QSizeF(300, 200));
    }

    ~Fixture() {
        delete item;
        WebViewItem::setInputPanelHookForTest(nullptr);
    }

    // Items get the active focus only inside a focused window (qquickwindow.cpp setFocusInScope checks the content
    // item's focus, which QQuickWindow::focusInEvent sets): fake the window focus without showing it.
    void focusWindow() {
        QFocusEvent focusIn(QEvent::FocusIn, Qt::ActiveWindowFocusReason);
        QCoreApplication::sendEvent(&window, &focusIn);
    }

    void sendTouch(QEvent::Type type, const QList<QTouchEvent::TouchPoint> &points, Qt::TouchPointStates states) {
        static QTouchDevice *device = QTest::createTouchDevice();
        QTouchEvent          event(type, device, Qt::NoModifier, states, points);
        event.setTimestamp(4242);
        QCoreApplication::sendEvent(item, &event);
        lastAccepted = event.isAccepted();
    }

    static QTouchEvent::TouchPoint touchPoint(int id, Qt::TouchPointState state, const QPointF &pos) {
        QTouchEvent::TouchPoint point(id);
        point.setState(state);
        point.setPos(pos);
        point.setScenePos(pos);
        point.setScreenPos(pos);
        return point;
    }

    // a one-finger touch sequence: press at from, one move to `to`, release there
    void touchTap(const QPointF &from, const QPointF &to, int id = 1) {
        sendTouch(QEvent::TouchBegin, {touchPoint(id, Qt::TouchPointPressed, from)}, Qt::TouchPointPressed);
        if (to != from) {
            sendTouch(QEvent::TouchUpdate, {touchPoint(id, Qt::TouchPointMoved, to)}, Qt::TouchPointMoved);
        }
        sendTouch(QEvent::TouchEnd, {touchPoint(id, Qt::TouchPointReleased, to)}, Qt::TouchPointReleased);
    }

    void sendKey(QEvent::Type type, int key, const QString &text = QString(),
                 Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QKeyEvent event(type, key, modifiers, text);
        event.ignore();
        QCoreApplication::sendEvent(item, &event);
        lastAccepted = event.isAccepted();
    }

    bool lastAccepted = false;
};

class testWebViewItem : public QObject {
    Q_OBJECT

 private slots:
    void realLibraryIsAbsentOnTheHost();
    void availableWithTheFunctionTable();
    void viewIsCreatedInDevicePixelsOnceItHasWindowAndSize();
    void pendingUrlIsLoadedWhenTheViewExists();
    void loadEventsDriveTheProperties();
    void historyFollowsSameDocumentNavigations();
    void loadFailureIsSignalled();
    void crashReloadsOnceUntilALoadFinishes();
    void imeFocusEnablesTheInputMethodWithTheMappedHints();
    void keyboardFollowsTheFocusOwnerNotTheImeCallbacks();
    void tapIntoTheFocusedFieldAsksForTheKeyboardAgain();
    void tapThatMovesThePageFocusDoesNotFlashTheKeyboard();
    void urlsOutsideTheSchemeAllowlistAreRefused();
    void inputMethodEventsReachTheLibrary();
    void touchPointsAreForwardedAndCancelledWhenTheSequenceIsLost();
    void mouseButtonsAreReleasedWhenTheGrabIsLost();
    void pageKeysAreForwardedRemoteButtonsAreNot();
    void framesArriveAndAreReleasedWhenTheItemGoes();
    void screenRefreshRateGoesToTheLibraryBeforeTheView();
    void refreshRateFollowsTheScreen();
    void refreshRateSetterIsOptional();
    void destructorFreesTheView();
};

void testWebViewItem::realLibraryIsAbsentOnTheHost() {
    // runs first, before a Fixture installs the stub table: the real probe on a build host without
    // libucwebview.so.1 (or without Qt's GLib dispatcher, as with the offscreen platform) reports why
    QVERIFY(!WebViewLibrary::instance()->isAvailable());
    QVERIFY(!WebViewLibrary::instance()->unavailableReason().isEmpty());
    QVERIFY(!WebViewLibrary::instance()->ensureInitialized());

    // an item created without the library is a placeholder: no view, no library calls, events are ignored
    QQuickWindow window;
    WebViewItem  item(window.contentItem());
    item.setSize(QSizeF(100, 100));
    QVERIFY(!item.isAvailable());
    QCOMPARE(item.inputMethodQuery(Qt::ImEnabled).toBool(), false);
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
    key.ignore();
    QCoreApplication::sendEvent(&item, &key);
    QVERIFY(!key.isAccepted());
    item.setUrl(QUrl(QStringLiteral("http://localhost/")));
    item.reload();
    QCOMPARE(item.url(), QUrl(QStringLiteral("http://localhost/")));
}

void testWebViewItem::availableWithTheFunctionTable() {
    Fixture f;
    QVERIFY(f.item->isAvailable());
    QVERIFY(WebViewLibrary::instance()->isAvailable());
}

void testWebViewItem::viewIsCreatedInDevicePixelsOnceItHasWindowAndSize() {
    Fixture f;
    QCOMPARE(stub::s.initCalls, 1);
    QVERIFY(stub::s.view != nullptr);
    const qreal ratio = f.window.effectiveDevicePixelRatio();
    QCOMPARE(stub::s.width, qRound(300 * ratio));
    QCOMPARE(stub::s.height, qRound(200 * ratio));
    QVERIFY(stub::s.calls.contains(QStringLiteral("set_scale(%1)").arg(ratio)));
    // the window is not shown: the page is told it is hidden
    QVERIFY(stub::s.calls.contains(QStringLiteral("set_mapped(0)")));

    f.item->setSize(QSizeF(100, 50));
    QCOMPARE(stub::s.calls.last(), QStringLiteral("resize(%1x%2)").arg(qRound(100 * ratio)).arg(qRound(50 * ratio)));
}

void testWebViewItem::pendingUrlIsLoadedWhenTheViewExists() {
    stub::s = stub::State();
    WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
    QQuickWindow window;

    // url set before the item is in a scene: remembered and loaded with the view
    auto *item = new WebViewItem();
    item->setUrl(QUrl(QStringLiteral("http://localhost/")));
    QVERIFY(stub::s.view == nullptr);
    item->setSize(QSizeF(300, 200));
    item->setParentItem(window.contentItem());
    QVERIFY(stub::s.view != nullptr);
    QVERIFY(stub::s.calls.contains(QStringLiteral("load_uri(http://localhost/)")));

    QSignalSpy urlSpy(item, &WebViewItem::urlChanged);
    item->setUrl(QUrl(QStringLiteral("http://example.com/a")));
    QCOMPARE(urlSpy.count(), 1);
    QCOMPARE(stub::s.calls.last(), QStringLiteral("load_uri(http://example.com/a)"));

    item->loadHtml(QStringLiteral("<p>hi</p>"), QUrl(QStringLiteral("http://base/")));
    QCOMPARE(stub::s.calls.last(), QStringLiteral("load_html(<p>hi</p>,http://base/)"));
    item->runJavaScript(QStringLiteral("1+1"));
    QCOMPARE(stub::s.calls.last(), QStringLiteral("run_javascript(1+1)"));
    item->reload();
    QCOMPARE(stub::s.calls.last(), QStringLiteral("reload"));
    delete item;
}

void testWebViewItem::loadEventsDriveTheProperties() {
    Fixture    f;
    QSignalSpy loading(f.item, &WebViewItem::loadingChanged);
    QSignalSpy history(f.item, &WebViewItem::navigationHistoryChanged);

    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/");
    QVERIFY(f.item->isLoading());
    QCOMPARE(f.item->url(), QUrl(QStringLiteral("http://localhost/")));
    QCOMPARE(loading.count(), 1);

    stub::fireProgress(0.5);
    QCOMPARE(f.item->loadProgress(), 50);
    stub::fireTitle("Welcome");
    QCOMPARE(f.item->title(), QStringLiteral("Welcome"));

    stub::fireLoadChanged(UC_WEBVIEW_LOAD_REDIRECTED, "http://localhost/index.html");
    QCOMPARE(f.item->url(), QUrl(QStringLiteral("http://localhost/index.html")));

    stub::s.canGoBack = true;
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_COMMITTED, "http://localhost/index.html");
    QVERIFY(f.item->canGoBack());
    QVERIFY(!f.item->canGoForward());
    QCOMPARE(history.count(), 1);

    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://localhost/index.html");
    QVERIFY(!f.item->isLoading());
    QCOMPARE(loading.count(), 2);
}

void testWebViewItem::historyFollowsSameDocumentNavigations() {
    // a single-page application changes the back/forward list with history.pushState and shows the new route:
    // WebKit reports no load event for it, only the new frame and maybe a new title
    Fixture    f;
    QSignalSpy history(f.item, &WebViewItem::navigationHistoryChanged);
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://localhost/");
    QVERIFY(!f.item->canGoBack());
    QCOMPARE(history.count(), 0);

    stub::s.canGoBack = true;
    stub::fireFrame(1);
    QVERIFY(f.item->canGoBack());
    QVERIFY(!f.item->canGoForward());
    QCOMPARE(history.count(), 1);

    // frames without a history change do not signal
    stub::fireFrame(2);
    QCOMPARE(history.count(), 1);

    // going back between such entries: the page's new title arrives before its frame
    stub::s.canGoBack = false;
    stub::s.canGoForward = true;
    stub::fireTitle("Route 1");
    QVERIFY(!f.item->canGoBack());
    QVERIFY(f.item->canGoForward());
    QCOMPARE(history.count(), 2);
}

void testWebViewItem::loadFailureIsSignalled() {
    Fixture    f;
    QSignalSpy failed(f.item, &WebViewItem::loadFailed);

    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://nowhere/");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FAILED, "http://nowhere/");
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().first().toUrl(), QUrl(QStringLiteral("http://nowhere/")));
    // loading stays true until the FINISHED event that follows a failure
    QVERIFY(f.item->isLoading());
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://nowhere/");
    QVERIFY(!f.item->isLoading());
}

void testWebViewItem::crashReloadsOnceUntilALoadFinishes() {
    Fixture    f;
    QSignalSpy crashed(f.item, &WebViewItem::crashed);

    stub::fireTerminated(UC_WEBVIEW_TERMINATED_CRASHED);
    QCOMPARE(crashed.count(), 1);
    QCOMPARE(crashed.first().first().toInt(), UC_WEBVIEW_TERMINATED_CRASHED);
    // the reload is deferred out of the callback
    QCOMPARE(stub::s.calls.count(QStringLiteral("reload")), 0);
    QTRY_COMPARE(stub::s.calls.count(QStringLiteral("reload")), 1);

    // a second crash without a completed load in between is not reloaded again
    stub::fireTerminated(UC_WEBVIEW_TERMINATED_EXCEEDED_MEMORY_LIMIT);
    QCOMPARE(crashed.count(), 2);
    QTest::qWait(10);
    QCOMPARE(stub::s.calls.count(QStringLiteral("reload")), 1);

    // a completed load re-arms the automatic reload
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://localhost/");
    stub::fireTerminated(UC_WEBVIEW_TERMINATED_CRASHED);
    QTRY_COMPARE(stub::s.calls.count(QStringLiteral("reload")), 2);

    // a failed load does not
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FAILED, "http://localhost/");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://localhost/");
    stub::fireTerminated(UC_WEBVIEW_TERMINATED_CRASHED);
    QTest::qWait(10);
    QCOMPARE(stub::s.calls.count(QStringLiteral("reload")), 2);
}

void testWebViewItem::imeFocusEnablesTheInputMethodWithTheMappedHints() {
    Fixture f;
    QCOMPARE(f.item->inputMethodQuery(Qt::ImEnabled).toBool(), false);

    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_DIGITS, "12", 2);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImEnabled).toBool(), true);
    QCOMPARE(Qt::InputMethodHints(f.item->inputMethodQuery(Qt::ImHints).toInt()),
             Qt::InputMethodHints(Qt::ImhDigitsOnly));
    QCOMPARE(f.item->inputMethodQuery(Qt::ImSurroundingText).toString(), QStringLiteral("12"));
    QCOMPARE(f.item->inputMethodQuery(Qt::ImCursorPosition).toInt(), 2);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImAnchorPosition).toInt(), 2);
    // the item never takes the focus from an IME callback, and asks for the keyboard only as the focus owner
    QVERIFY(!f.item->hasFocus());
    QVERIFY(stub::s.panel.isEmpty());

    // the caret rectangle comes in view pixels and is answered in item coordinates
    const qreal ratio = f.window.effectiveDevicePixelRatio();
    stub::fireImeCursorArea(qRound(20 * ratio), qRound(40 * ratio), qRound(2 * ratio), qRound(16 * ratio));
    QCOMPARE(f.item->inputMethodQuery(Qt::ImCursorRectangle).toRectF(), QRectF(20, 40, 2, 16));

    stub::fireImeSurrounding("123", 3, 1);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImSurroundingText).toString(), QStringLiteral("123"));
    QCOMPARE(f.item->inputMethodQuery(Qt::ImCursorPosition).toInt(), 3);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImAnchorPosition).toInt(), 1);

    // hints change while focused: a password field now
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PASSWORD, nullptr, -1);
    QVERIFY(Qt::InputMethodHints(f.item->inputMethodQuery(Qt::ImHints).toInt()) & Qt::ImhHiddenText);

    stub::fireImeFocus(0, 0, 0, nullptr, -1);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImEnabled).toBool(), false);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImSurroundingText).toString(), QString());
}

void testWebViewItem::keyboardFollowsTheFocusOwnerNotTheImeCallbacks() {
    Fixture f;
    f.focusWindow();
    f.item->forceActiveFocus();
    QVERIFY(f.item->hasActiveFocus());
    QCOMPARE(stub::s.calls.last(), QStringLiteral("set_focused(1)"));

    // the page focuses an editable element while the item owns the focus: keyboard requested once
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
    QCOMPARE(stub::s.panel, QStringList{QStringLiteral("show")});
    QVERIFY(f.item->inputMethodQuery(Qt::ImEnabled).toBool());

    // a TextField next to the view takes the focus (the address field of the developer page)
    QQuickItem other(f.window.contentItem());
    other.setFlag(QQuickItem::ItemAcceptsInputMethod);
    other.forceActiveFocus();
    QVERIFY(other.hasActiveFocus());
    QVERIFY(!f.item->hasActiveFocus());
    QCOMPARE(stub::s.calls.last(), QStringLiteral("set_focused(0)"));
    // the item forgets the page's editable element without closing the other item's keyboard
    QVERIFY(!f.item->inputMethodQuery(Qt::ImEnabled).toBool());
    QCOMPARE(stub::s.panel, QStringList{QStringLiteral("show")});

    // WebKit reports the focus-out that set_focused(0) caused: still no hide, no refocus
    stub::fireImeFocus(0, 0, 0, nullptr, -1);
    QCOMPARE(stub::s.panel, QStringList{QStringLiteral("show")});
    QVERIFY(other.hasActiveFocus());

    // a focus-in arriving while another item owns the focus is recorded but moves neither focus nor keyboard
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
    QCOMPARE(stub::s.panel, QStringList{QStringLiteral("show")});
    QVERIFY(other.hasActiveFocus());
    QVERIFY(!f.item->hasActiveFocus());

    // the user comes back to the view (a press): the page's element is reported again and the keyboard follows
    f.item->forceActiveFocus();
    QCOMPARE(stub::s.calls.last(), QStringLiteral("set_focused(1)"));
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
    QCOMPARE(stub::s.panel, (QStringList{QStringLiteral("show"), QStringLiteral("show")}));

    // the page blurs the element while the item owns the focus: this hide is the item's to request
    stub::fireImeFocus(0, 0, 0, nullptr, -1);
    QCOMPARE(stub::s.panel, (QStringList{QStringLiteral("show"), QStringLiteral("show"), QStringLiteral("hide")}));
    QVERIFY(!f.item->inputMethodQuery(Qt::ImEnabled).toBool());
}

void testWebViewItem::tapIntoTheFocusedFieldAsksForTheKeyboardAgain() {
    Fixture           f;
    const QStringList once{QStringLiteral("show")};
    const QStringList twice{QStringLiteral("show"), QStringLiteral("show")};
    f.focusWindow();

    // the user taps the page's PIN field: the press gives the item the focus, WebKit reports the element
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QVERIFY(f.item->hasActiveFocus());
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
    QCOMPARE(stub::s.panel, once);

    // The keyboard's hide key closes it; the field keeps the DOM focus, so the next tap on it brings no ime_focus
    // (WebKit only reports focus changes). The tap itself asks again, after WebKit had the time to report a focus
    // change of the page (not at once), and a small finger movement is still a tap.
    f.touchTap(QPointF(100, 50), QPointF(103, 52));
    QCOMPARE(stub::s.panel, once);
    QTRY_COMPARE(stub::s.panel, twice);

    // a drag farther than WebKit's 8 px is a scroll, not a tap: no request
    f.touchTap(QPointF(100, 50), QPointF(100, 70));
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
    QCOMPARE(stub::s.panel, twice);

    // a release outside the item does not count
    f.touchTap(QPointF(296, 50), QPointF(302, 50));
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
    QCOMPARE(stub::s.panel, twice);

    // a second finger is not a tap of its own (WebKit's gesture controller follows the first one only)
    f.sendTouch(QEvent::TouchBegin, {Fixture::touchPoint(5, Qt::TouchPointPressed, QPointF(10, 10))},
                Qt::TouchPointPressed);
    f.sendTouch(QEvent::TouchUpdate,
                {Fixture::touchPoint(5, Qt::TouchPointMoved, QPointF(10, 60)),
                 Fixture::touchPoint(6, Qt::TouchPointPressed, QPointF(50, 50))},
                Qt::TouchPointPressed | Qt::TouchPointMoved);
    f.sendTouch(QEvent::TouchUpdate,
                {Fixture::touchPoint(5, Qt::TouchPointStationary, QPointF(10, 60)),
                 Fixture::touchPoint(6, Qt::TouchPointReleased, QPointF(50, 50))},
                Qt::TouchPointReleased | Qt::TouchPointStationary);
    f.sendTouch(QEvent::TouchEnd, {Fixture::touchPoint(5, Qt::TouchPointReleased, QPointF(10, 60))},
                Qt::TouchPointReleased);
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
    QCOMPARE(stub::s.panel, twice);

    // a click of the primary mouse button (desktop) is a tap too
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(40, 40), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(f.item, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(41, 40), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(f.item, &release);
    QTRY_COMPARE(stub::s.panel.size(), 3);

    // the page asked for no on-screen keyboard (inputmode="none", a field focused by script): taps leave it alone
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_INHIBIT_OSK, UC_WEBVIEW_IME_PURPOSE_FREE_FORM, "", 0);
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
    QCOMPARE(stub::s.panel.size(), 3);

    // without a focused editable element a tap is just a click for the page
    stub::fireImeFocus(0, 0, 0, nullptr, -1);
    QCOMPARE(stub::s.panel.last(), QStringLiteral("hide"));
    const int afterBlur = stub::s.panel.size();
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
    QCOMPARE(stub::s.panel.size(), afterBlur);
}

void testWebViewItem::tapThatMovesThePageFocusDoesNotFlashTheKeyboard() {
    Fixture f;
    f.focusWindow();
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
    QCOMPARE(stub::s.panel, QStringList{QStringLiteral("show")});

    // keyboard closed by its hide key, then a tap on the page's Submit button: WebKit moves the focus to the
    // button and reports the field's focus-out before the tap's request is due, which cancels it
    f.touchTap(QPointF(100, 150), QPointF(100, 150));
    stub::fireImeFocus(0, 0, 0, nullptr, -1);
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
    QCOMPARE(stub::s.panel, (QStringList{QStringLiteral("show"), QStringLiteral("hide")}));

    // a tap whose request is pending when another item takes the focus (the address field next to the view)
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
    const int before = stub::s.panel.size();
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QQuickItem other(f.window.contentItem());
    other.forceActiveFocus();
    QVERIFY(!f.item->hasActiveFocus());
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
    QCOMPARE(stub::s.panel.size(), before);

    // a new press while a request is pending (the user scrolls right after the tap) drops it
    f.item->forceActiveFocus();
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
    const int scrolled = stub::s.panel.size();
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    f.touchTap(QPointF(100, 50), QPointF(100, 90));
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
    QCOMPARE(stub::s.panel.size(), scrolled);
}

void testWebViewItem::urlsOutsideTheSchemeAllowlistAreRefused() {
    qunsetenv("UC_WEBVIEW_ALLOW_FILE_URLS");
    Fixture    f;
    QSignalSpy failed(f.item, &WebViewItem::loadFailed);
    QSignalSpy urlChanged(f.item, &WebViewItem::urlChanged);
    auto       loads = [] { return stub::s.calls.filter(QRegularExpression("^load_(uri|html)\\(")); };

    // file: is blocked by default (production), like the library does: loadFailed at once, nothing loaded, the
    // item's url stays
    const QUrl file(QStringLiteral("file:///app/data/emoji.html"));
    f.item->setUrl(file);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.last().first().toUrl(), file);
    QVERIFY(loads().isEmpty());
    QCOMPARE(urlChanged.count(), 0);
    QVERIFY(f.item->url().isEmpty());

    for (const char *refused : {"javascript:alert(1)", "ftp://host/file", "localhost:8080/", "/app/data/x.html"}) {
        f.item->setUrl(QUrl(QString::fromLatin1(refused)));
    }
    QCOMPARE(failed.count(), 5);
    QVERIFY(loads().isEmpty());

    // a base URL outside the allowlist refuses the HTML too
    f.item->loadHtml(QStringLiteral("<p>x</p>"), QUrl(QStringLiteral("file:///")));
    QCOMPARE(failed.count(), 6);
    QCOMPARE(failed.last().first().toUrl(), QUrl(QStringLiteral("file:///")));
    QVERIFY(loads().isEmpty());

    // the allowed schemes go to the library
    f.item->setUrl(QUrl(QStringLiteral("http://localhost/")));
    f.item->setUrl(QUrl(QStringLiteral("about:blank")));
    f.item->setUrl(QUrl(QStringLiteral("data:text/html,hi")));
    f.item->loadHtml(QStringLiteral("<p>y</p>"), QUrl());
    QCOMPARE(loads(), (QStringList{QStringLiteral("load_uri(http://localhost/)"), QStringLiteral("load_uri(about:blank)"),
                                   QStringLiteral("load_uri(data:text/html,hi)"), QStringLiteral("load_html(<p>y</p>,null)")}));
    QCOMPARE(failed.count(), 6);

    // UC_WEBVIEW_ALLOW_FILE_URLS=1 (the library reads the same variable) lets file: through
    qputenv("UC_WEBVIEW_ALLOW_FILE_URLS", "1");
    f.item->setUrl(file);
    f.item->loadHtml(QStringLiteral("<p>z</p>"), QUrl(QStringLiteral("file:///")));
    qunsetenv("UC_WEBVIEW_ALLOW_FILE_URLS");
    QCOMPARE(loads().mid(4), (QStringList{QStringLiteral("load_uri(file:///app/data/emoji.html)"),
                                          QStringLiteral("load_html(<p>z</p>,file:///)")}));
    QCOMPARE(failed.count(), 6);

    // the library refuses on its own as well (another host, a different environment): FAILED without STARTED or
    // FINISHED is a load failure that leaves `loading` alone
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FAILED, "file:///etc/passwd");
    QCOMPARE(failed.count(), 7);
    QCOMPARE(failed.last().first().toUrl(), QUrl(QStringLiteral("file:///etc/passwd")));
    QVERIFY(!f.item->isLoading());
}

void testWebViewItem::inputMethodEventsReachTheLibrary() {
    Fixture f;

    // without a focused editable element the keyboard's events are not forwarded
    {
        QInputMethodEvent commit;
        commit.setCommitString(QStringLiteral("x"));
        QCoreApplication::sendEvent(f.item, &commit);
        QVERIFY(!stub::s.calls.contains(QStringLiteral("ime_commit(x)")));
    }

    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_FREE_FORM, "", 0);
    const int before = stub::s.calls.size();

    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 1, 1, QVariant()));
    QInputMethodEvent preedit(QStringLiteral("ab"), attributes);
    QCoreApplication::sendEvent(f.item, &preedit);
    QCOMPARE(stub::s.calls.mid(before), QStringList{QStringLiteral("ime_preedit(ab,1)")});

    QInputMethodEvent commit;
    commit.setCommitString(QStringLiteral("abc"));
    QCoreApplication::sendEvent(f.item, &commit);
    QCOMPARE(stub::s.calls.mid(before + 1), QStringList{QStringLiteral("ime_commit(abc)")});

    // an empty event after a pre-edit ends it; after a commit there is nothing to end
    QInputMethodEvent empty;
    QCoreApplication::sendEvent(f.item, &empty);
    QCOMPARE(stub::s.calls.size(), before + 2);

    QInputMethodEvent preeditAgain(QStringLiteral("z"), QList<QInputMethodEvent::Attribute>());
    QCoreApplication::sendEvent(f.item, &preeditAgain);
    QCoreApplication::sendEvent(f.item, &empty);
    QCOMPARE(stub::s.calls.mid(before + 2),
             (QStringList{QStringLiteral("ime_preedit(z,1)"), QStringLiteral("ime_preedit(,0)")}));

    // word replacement: delete around the caret, then commit
    QInputMethodEvent replace;
    replace.setCommitString(QStringLiteral("word"), -3, 3);
    QCoreApplication::sendEvent(f.item, &replace);
    QCOMPARE(stub::s.calls.mid(before + 4),
             (QStringList{QStringLiteral("ime_delete_surrounding(-3,3)"), QStringLiteral("ime_commit(word)")}));
}

void testWebViewItem::pageKeysAreForwardedRemoteButtonsAreNot() {
    Fixture   f;
    const int before = stub::s.calls.size();

    f.sendKey(QEvent::KeyPress, Qt::Key_Return, QStringLiteral("\r"));
    QVERIFY(f.lastAccepted);
    f.sendKey(QEvent::KeyRelease, Qt::Key_Return, QStringLiteral("\r"));
    QVERIFY(f.lastAccepted);
    f.sendKey(QEvent::KeyPress, Qt::Key_A, QStringLiteral("A"), Qt::ShiftModifier);
    QVERIFY(f.lastAccepted);
    QCOMPARE(stub::s.calls.mid(before),
             (QStringList{QStringLiteral("key(0xff0d,1,0)"), QStringLiteral("key(0xff0d,0,0)"),
                          QStringLiteral("key(0x41,1,%1)").arg(UC_WEBVIEW_MOD_SHIFT)}));

    // BACK and HOME of the remote are ignored, so the QML page gets them
    f.sendKey(QEvent::KeyPress, Qt::Key_Exit);
    QVERIFY(!f.lastAccepted);
    f.sendKey(QEvent::KeyPress, Qt::Key_Home);
    QVERIFY(!f.lastAccepted);
    f.sendKey(QEvent::KeyPress, Qt::Key_VolumeUp);
    QVERIFY(!f.lastAccepted);
    QCOMPARE(stub::s.calls.size(), before + 3);
}

void testWebViewItem::touchPointsAreForwardedAndCancelledWhenTheSequenceIsLost() {
    Fixture     f;
    const qreal r = f.window.effectiveDevicePixelRatio();
    auto        touch = [r](int slot, int phase, qreal x, qreal y) {
        return QStringLiteral("touch(%1,%2,%3,%4)").arg(slot).arg(phase).arg(x * r).arg(y * r);
    };
    // only the touch calls: the first press also records the focus change (set_focused)
    auto touches = [](int from) { return stub::s.calls.mid(from).filter(QStringLiteral("touch(")).join(" | "); };
    const int before = stub::s.calls.size();

    // a tap with Qt id 7: down, a small move, up; stationary points are not forwarded; the library gets slot 0
    // like WPE's DRM platform would send it (the device's touch ids are the kernel's incrementing tracking ids)
    f.sendTouch(QEvent::TouchBegin, {Fixture::touchPoint(7, Qt::TouchPointPressed, QPointF(10, 20))},
                Qt::TouchPointPressed);
    QVERIFY(f.lastAccepted);
    f.sendTouch(QEvent::TouchUpdate, {Fixture::touchPoint(7, Qt::TouchPointMoved, QPointF(11, 21))},
                Qt::TouchPointMoved);
    f.sendTouch(QEvent::TouchUpdate, {Fixture::touchPoint(7, Qt::TouchPointStationary, QPointF(11, 21))},
                Qt::TouchPointStationary);
    f.sendTouch(QEvent::TouchEnd, {Fixture::touchPoint(7, Qt::TouchPointReleased, QPointF(11, 21))},
                Qt::TouchPointReleased);
    QCOMPARE(touches(before),
             (QStringList{touch(0, UC_WEBVIEW_TOUCH_DOWN, 10, 20), touch(0, UC_WEBVIEW_TOUCH_MOVE, 11, 21),
                          touch(0, UC_WEBVIEW_TOUCH_UP, 11, 21)}).join(" | "));
    // the first press took the focus
    QVERIFY(f.item->hasFocus());

    // the next finger gets slot 0 again; another item steals the grab mid-sequence: Qt calls touchUngrabEvent()
    // without a cancel event
    const int lost = stub::s.calls.size();
    f.sendTouch(QEvent::TouchBegin, {Fixture::touchPoint(8, Qt::TouchPointPressed, QPointF(50, 60))},
                Qt::TouchPointPressed);
    f.sendTouch(QEvent::TouchUpdate, {Fixture::touchPoint(8, Qt::TouchPointMoved, QPointF(70, 60))},
                Qt::TouchPointMoved);
    f.item->touchUngrabEvent();
    QCOMPARE(touches(lost),
             (QStringList{touch(0, UC_WEBVIEW_TOUCH_DOWN, 50, 60), touch(0, UC_WEBVIEW_TOUCH_MOVE, 70, 60),
                          touch(0, UC_WEBVIEW_TOUCH_CANCEL, 70, 60)}).join(" | "));
    // nothing left to cancel, and a late release of the cancelled point is dropped (no press for it)
    f.item->touchUngrabEvent();
    f.sendTouch(QEvent::TouchEnd, {Fixture::touchPoint(8, Qt::TouchPointReleased, QPointF(70, 60))},
                Qt::TouchPointReleased);
    QCOMPARE(touches(lost).count(QStringLiteral("touch(")), 3);

    // two fingers: slots 0 and 1; a TouchCancel comes without touch points (qquickwindow.cpp
    // deliverTouchCancelEvent): both end
    const int two = stub::s.calls.size();
    f.sendTouch(QEvent::TouchBegin, {Fixture::touchPoint(9, Qt::TouchPointPressed, QPointF(1, 2))},
                Qt::TouchPointPressed);
    f.sendTouch(QEvent::TouchUpdate,
                {Fixture::touchPoint(9, Qt::TouchPointStationary, QPointF(1, 2)),
                 Fixture::touchPoint(10, Qt::TouchPointPressed, QPointF(3, 4))},
                Qt::TouchPointPressed | Qt::TouchPointStationary);
    QCOMPARE(touches(two),
             (QStringList{touch(0, UC_WEBVIEW_TOUCH_DOWN, 1, 2), touch(1, UC_WEBVIEW_TOUCH_DOWN, 3, 4)}).join(" | "));
    f.sendTouch(QEvent::TouchCancel, {}, Qt::TouchPointStates());
    QVERIFY(f.lastAccepted);
    QStringList cancelled = stub::s.calls.mid(two).filter(QStringLiteral("touch(")).mid(2);
    cancelled.sort();
    QCOMPARE(cancelled.join(" | "), (QStringList{touch(0, UC_WEBVIEW_TOUCH_CANCEL, 1, 2),
                                                 touch(1, UC_WEBVIEW_TOUCH_CANCEL, 3, 4)}).join(" | "));

    // a press for a Qt id that is still down (its release was lost) ends the old sequence first
    const int reuse = stub::s.calls.size();
    f.sendTouch(QEvent::TouchBegin, {Fixture::touchPoint(11, Qt::TouchPointPressed, QPointF(5, 5))},
                Qt::TouchPointPressed);
    f.sendTouch(QEvent::TouchBegin, {Fixture::touchPoint(11, Qt::TouchPointPressed, QPointF(6, 6))},
                Qt::TouchPointPressed);
    QCOMPARE(touches(reuse),
             (QStringList{touch(0, UC_WEBVIEW_TOUCH_DOWN, 5, 5), touch(0, UC_WEBVIEW_TOUCH_CANCEL, 5, 5),
                          touch(0, UC_WEBVIEW_TOUCH_DOWN, 6, 6)}).join(" | "));
}

void testWebViewItem::mouseButtonsAreReleasedWhenTheGrabIsLost() {
    Fixture     f;
    const qreal r = f.window.effectiveDevicePixelRatio();
    const int   before = stub::s.calls.size();

    QMouseEvent press(QEvent::MouseButtonPress, QPointF(10, 10), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(f.item, &press);
    QVERIFY(press.isAccepted());
    f.item->mouseUngrabEvent();
    auto button = [r](int pressed) {
        return QStringLiteral("pointer_button(%1,%2,%3,%4)")
            .arg(UC_WEBVIEW_BUTTON_PRIMARY)
            .arg(pressed)
            .arg(10 * r)
            .arg(10 * r);
    };
    QCOMPARE(stub::s.calls.mid(before).filter(QStringLiteral("pointer_button(")).join(" | "),
             (QStringList{button(1), button(0)}).join(" | "));
    // nothing pressed any more
    f.item->mouseUngrabEvent();
    QCOMPARE(stub::s.calls.mid(before).filter(QStringLiteral("pointer_button(")).size(), 2);
}

void testWebViewItem::framesArriveAndAreReleasedWhenTheItemGoes() {
    stub::s = stub::State();
    WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
    QQuickWindow window;
    auto        *item = new WebViewItem(window.contentItem());
    item->setSize(QSizeF(300, 200));
    QSignalSpy stats(item, &WebViewItem::frameStatsChanged);

    stub::fireFrame(1);
    QCOMPARE(item->framesReceived(), 1);
    QCOMPARE(item->framesReleased(), 0);
    QCOMPARE(stats.count(), 1);

    // the window never renders: the second frame replaces the first, which goes straight back
    stub::fireFrame(2);
    QCOMPARE(item->framesReceived(), 2);
    QCOMPARE(item->framesReleased(), 1);
    QVERIFY(stub::s.calls.contains(QStringLiteral("frame_release(1)")));

    // the item is destroyed with a frame in hand: released before the view is freed
    delete item;
    const int release = stub::s.calls.indexOf(QStringLiteral("frame_release(2)"));
    const int free = stub::s.calls.indexOf(QStringLiteral("free"));
    QVERIFY(release >= 0);
    QVERIFY(free > release);
}

void testWebViewItem::screenRefreshRateGoesToTheLibraryBeforeTheView() {
    // WebKit reads the rate once, when the first load sets up the page's display link: it has to reach the library
    // after uc_webview_global_init() and before uc_webview_new()
    Fixture     f;
    const qreal rate = f.window.screen()->refreshRate();
    QCOMPARE(stub::s.initCalls, 1);
    QCOMPARE(stub::s.refreshRates.count(), 1);
    QCOMPARE(stub::s.refreshRates.first(), static_cast<double>(rate));
    const int setIndex = stub::s.calls.indexOf(QStringLiteral("global_set_refresh_rate(%1)").arg(rate));
    const int newIndex = stub::s.calls.indexOf(QRegularExpression(QStringLiteral("new\\(.*")));  // exact match
    QVERIFY2(setIndex >= 0 && newIndex > setIndex, qPrintable(stub::s.calls.join(QLatin1Char(' '))));

    // resizing or remapping the view does not pass the same rate again
    f.item->setSize(QSizeF(120, 80));
    QCOMPARE(stub::s.refreshRates.count(), 1);
}

void testWebViewItem::refreshRateFollowsTheScreen() {
    stub::s = stub::State();
    WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
    QQuickWindow window;
    QScreen     *screen = window.screen();
    QVERIFY(screen);
    const qreal original = screen->refreshRate();
    const qreal panel = qFuzzyCompare(original, 50.0) ? 60.0 : 50.0;

    // like the QML engine creates it: parented after construction, so the item watches its window's screen
    auto *item = new WebViewItem();
    item->setSize(QSizeF(300, 200));
    item->setParentItem(window.contentItem());
    QVERIFY(stub::s.view != nullptr);
    QCOMPARE(stub::s.refreshRates.count(), 1);
    QCOMPARE(stub::s.refreshRates.last(), static_cast<double>(original));

    // eglfs_kms reports a mode change through the QPA layer: the new rate goes to the library
    QWindowSystemInterface::handleScreenRefreshRateChange(screen, panel);
    QWindowSystemInterface::flushWindowSystemEvents();
    QCOMPARE(screen->refreshRate(), panel);
    QCOMPARE(stub::s.refreshRates.count(), 2);
    QCOMPARE(stub::s.refreshRates.last(), static_cast<double>(panel));
    QCOMPARE(stub::s.calls.last(), QStringLiteral("global_set_refresh_rate(%1)").arg(panel));

    QWindowSystemInterface::handleScreenRefreshRateChange(screen, original);
    QWindowSystemInterface::flushWindowSystemEvents();
    QCOMPARE(stub::s.refreshRates.count(), 3);
    QCOMPARE(stub::s.refreshRates.last(), static_cast<double>(original));

    // once the item is gone, screen changes no longer reach the library
    delete item;
    QWindowSystemInterface::handleScreenRefreshRateChange(screen, panel);
    QWindowSystemInterface::flushWindowSystemEvents();
    QCOMPARE(stub::s.refreshRates.count(), 3);
    QWindowSystemInterface::handleScreenRefreshRateChange(screen, original);
    QWindowSystemInterface::flushWindowSystemEvents();
    QCOMPARE(screen->refreshRate(), original);
}

void testWebViewItem::refreshRateSetterIsOptional() {
    // a library of the same API version from before 0.2.0 has no uc_webview_global_set_refresh_rate
    {
        stub::s = stub::State();
        WebViewFunctions functions = stub::functions();
        functions.global_set_refresh_rate = nullptr;
        WebViewLibrary::instance()->setFunctionsForTest(functions);
        QQuickWindow window;
        WebViewItem  item(window.contentItem());
        item.setSize(QSizeF(300, 200));
        QVERIFY(stub::s.view != nullptr);
        QVERIFY(stub::s.refreshRates.isEmpty());
    }
    // a rate the library refuses is logged once, not passed again, and the view works
    {
        stub::s = stub::State();
        stub::s.refreshRateResult = -1;
        WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("refused the display refresh rate")));
        QQuickWindow window;
        WebViewItem  item(window.contentItem());
        item.setSize(QSizeF(300, 200));
        QVERIFY(stub::s.view != nullptr);
        QCOMPARE(stub::s.refreshRates.count(), 1);
        WebViewLibrary::instance()->setDisplayRefreshRate(window.screen()->refreshRate());
        QCOMPARE(stub::s.refreshRates.count(), 1);
    }
}

void testWebViewItem::destructorFreesTheView() {
    stub::s = stub::State();
    WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
    {
        QQuickWindow window;
        auto        *item = new WebViewItem(window.contentItem());
        item->setSize(QSizeF(10, 10));
        QVERIFY(stub::s.view != nullptr);
        delete item;
    }
    QVERIFY(stub::s.view == nullptr);
    QCOMPARE(stub::s.calls.count(QStringLiteral("free")), 1);
}

QTEST_MAIN(testWebViewItem)
#include "test_webview_item.moc"
