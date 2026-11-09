// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QElapsedTimer>
#include <QQuickWindow>
#include <QScreen>
#include <QtTest>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>

#include <algorithm>
#include <cstring>
#include <utility>

#include "webview/webviewinput.h"
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
    int                  dragThreshold = 16;     // uc_webview_global_get_drag_threshold() (library 0.3.0 default)
    // WebKit reports the page's focused editable element synchronously when the view's focus changes
    // (WPEWebViewPlatform.cpp:100-110): set_focused() fires ime_focus with these hints while replayFocus is set
    bool                 replayFocus = false;
    unsigned             replayHints = 0;
    unsigned             replayPurpose = 0;
    QList<uint32_t>      keycodes;  // the keycode of every uc_webview_key() call
    QList<uint32_t>      keyTimes;  // and its event time
    // uc_webview_when_rendered() requests (library 0.5.0) not ended yet, oldest first; renderResult is what the
    // stub returns (-1: refused)
    QList<QPair<uc_webview_rendered_cb, void *>> renders;
    int                  renderResult = 0;
    // pinch zoom (library 0.6.0): the view came from uc_webview_new_sized() with this cb_size, the scales of
    // uc_webview_set_magnification() and what it returns (-1: refused)
    bool                 createdSized = false;
    size_t               callbacksSize = 0;
    QList<double>        magnifications;
    int                  magnificationResult = 0;
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

int globalGetDragThreshold() {
    return s.dragThreshold;
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

// uc_webview_new_sized(): like the library, min(cb_size, its own size) bytes of the host's struct
uc_webview *createSized(int w, int h, const uc_webview_callbacks *cb, size_t size, void *user) {
    s.view = kView;
    s.width = w;
    s.height = h;
    s.callbacks = uc_webview_callbacks{};
    memcpy(&s.callbacks, cb, std::min(size, sizeof s.callbacks));
    s.user = user;
    s.createdSized = true;
    s.callbacksSize = size;
    record(QStringLiteral("new(%1x%2)").arg(w).arg(h));
    return kView;
}

int setMagnification(uc_webview *, double scale, double x, double y) {
    record(QStringLiteral("set_magnification(%1,%2,%3)").arg(scale).arg(x).arg(y));
    if (s.magnificationResult == 0) {
        s.magnifications.append(scale);
    }
    return s.magnificationResult;
}

double getMagnification(uc_webview *) {
    return s.magnifications.isEmpty() ? 1.0 : s.magnifications.last();
}

void destroy(uc_webview *v) {
    QCOMPARE(v, kView);
    record(QStringLiteral("free"));
    // like uc_webview_free(): pending render requests end as cancelled inside the call
    const auto pending = std::exchange(s.renders, {});
    for (const auto &render : pending) {
        render.first(v, UC_WEBVIEW_RENDERED_CANCELLED, render.second);
    }
    s.view = nullptr;
}

int whenRendered(uc_webview *, uc_webview_rendered_cb cb, void *user) {
    record(QStringLiteral("when_rendered"));
    if (s.renderResult == 0) {
        s.renders.append(qMakePair(cb, user));
    }
    return s.renderResult;
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
    if (s.replayFocus && s.callbacks.ime_focus) {
        s.callbacks.ime_focus(s.view, focused, s.replayHints, s.replayPurpose, focused ? "" : nullptr, focused ? 0 : -1,
                              s.user);
    }
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

void setZoom(uc_webview *, double zoom) {
    record(QStringLiteral("set_zoom(%1)").arg(zoom));
}

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

void key(uc_webview *, uint32_t keysym, uint32_t keycode, int pressed, unsigned mods, uint32_t ms) {
    s.keycodes.append(keycode);
    s.keyTimes.append(ms);
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
    f.global_get_drag_threshold = &globalGetDragThreshold;
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
    f.when_rendered = &whenRendered;
    f.create_sized = &createSized;
    f.set_magnification = &setMagnification;
    f.get_magnification = &getMagnification;
    return f;
}

// a library 0.5.x: no pinch zoom
WebViewFunctions functionsWithoutPinch() {
    WebViewFunctions f = functions();
    f.set_magnification = nullptr;
    f.get_magnification = nullptr;
    return f;
}

// a library older than 0.5.0: no uc_webview_when_rendered() (nor pinch zoom)
WebViewFunctions functionsWithoutWhenRendered() {
    WebViewFunctions f = functionsWithoutPinch();
    f.when_rendered = nullptr;
    return f;
}

// a library older than 0.4.0: uc_webview_new() only
WebViewFunctions functionsWithoutNewSized() {
    WebViewFunctions f = functionsWithoutWhenRendered();
    f.create_sized = nullptr;
    return f;
}

// the page rendered: the oldest request ends with status (the item may issue the next one from the callback)
void finishRender(int status = UC_WEBVIEW_RENDERED_DONE) {
    QVERIFY(!s.renders.isEmpty());
    const auto render = s.renders.takeFirst();
    render.first(s.view, status, render.second);
}

// every request ends as done, also the ones the item issues meanwhile
void finishRenders() {
    while (!s.renders.isEmpty()) {
        finishRender();
    }
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

void firePinch(int phase, double scale, double x, double y) {
    QVERIFY(s.callbacks.pinch != nullptr);
    s.callbacks.pinch(s.view, phase, scale, x, y, s.user);
}

// a pinch from begin to end around (x, y), the scale reached in two updates
void pinch(double scale, double x, double y) {
    firePinch(UC_WEBVIEW_PINCH_BEGIN, 1.0, x, y);
    firePinch(UC_WEBVIEW_PINCH_UPDATE, (1.0 + scale) / 2, x, y);
    firePinch(UC_WEBVIEW_PINCH_UPDATE, scale, x, y);
    firePinch(UC_WEBVIEW_PINCH_END, scale, x, y);
}

void fireMagnificationChanged(double magnification) {
    s.magnifications.append(magnification);
    s.callbacks.magnification_changed(s.view, magnification, s.user);
}

// the calls of the pinch zoom
QStringList magnificationCalls() {
    return s.calls.filter(QRegularExpression(QStringLiteral("^(set_magnification|when_rendered)")));
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

    // an empty size leaves the item without a view until the test sizes it
    explicit Fixture(const WebViewFunctions &functions = stub::functions(), const QSizeF &size = QSizeF(300, 200)) {
        stub::s = stub::State();
        WebViewLibrary::instance()->setFunctionsForTest(functions);
        WebViewItem::setInputPanelHookForTest([](bool show) { stub::s.panel.append(show ? "show" : "hide"); });
        item = new ExposedItem(window.contentItem());
        item->setSize(size);
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
                 Qt::KeyboardModifiers modifiers = Qt::NoModifier, quint32 scanCode = 0) {
        QKeyEvent event(type, key, modifiers, scanCode, 0, 0, text);
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
    void tapShowsTheKeyboardOnAnElementThePageFocusedItself();
    void inputmodeNoneHoldsForTapsAfterTheUsersFocus();
    void tapDistanceIsTheLibrarysDragThreshold();
    void traceCarriesNoTypedTextOrUrlSecrets();
    void urlsOutsideTheSchemeAllowlistAreRefused();
    void inputMethodEventsReachTheLibrary();
    void touchPointsAreForwardedAndCancelledWhenTheSequenceIsLost();
    void mouseButtonsAreReleasedWhenTheGrabIsLost();
    void pageKeysAreForwardedRemoteButtonsAreNot();
    void zoomStepsThroughTheBrowserLevelsAndStopsAtTheEnds();
    void zoomOutsideTheLibrarysRangeIsRefused();
    void zoomSetBeforeTheViewIsAppliedWhenItIsCreated();
    void zoomWaitsForThePageToRenderAndPassesOnlyTheLatestLevel();
    void zoomBackToTheRenderedLevelNeedsNoRelayout();
    void zoomWaitEndsOnTimeoutAndCancellation();
    void zoomWaitEndsQuietlyWhenTheItemGoes();
    void zoomWithoutWhenRenderedWaitsForTheFirstFrame();
    void zoomWithoutWhenRenderedEndsAfterTheFallbackTimeout();
    void refusedWhenRenderedFallsBackToTheFirstFrame();
    void zoomRenderTraceLines();
    void pageUpAndDownPressAndReleaseThePageKeys();
    void pinchCallbacksNeedTheSizedViewAndTheMagnificationFunctions();
    void olderLibrariesHaveNoPinchZoom();
    void pinchScalesTheFrameAndCommitsOnceAtTheEnd();
    void scaledFrameIsHeldUntilThePageRenderedTheMagnification();
    void magnificationTimeoutAndCancelShowTheNextFrame();
    void pinchesDuringAMagnificationRenderAreCommittedTogether();
    void smallOrCancelledPinchCommitsNothing();
    void refusedMagnificationSnapsBack();
    void navigationAndHistoryResetTheMagnification();
    void pageZoomAndMagnificationAreIndependent();
    void twoFingersAreNoTapWithPinchZoom();
    void magnificationWaitEndsQuietlyWhenTheItemGoes();
    void pinchTraceLines();
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
    // no view, nothing to call: the zoom is kept like the URL, the page keys go nowhere
    item.zoomIn();
    QCOMPARE(item.zoom(), 1.1);
    item.pageDown();
    item.pageUp();
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

    // the user taps an element with inputmode="none": WebKit reports it with INHIBIT_OSK right after the tap, and
    // taps leave it alone (inputmodeNoneHoldsForTapsAfterTheUsersFocus, tapShowsTheKeyboardOnAnElementThePage...)
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
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

void testWebViewItem::tapShowsTheKeyboardOnAnElementThePageFocusedItself() {
    // Seen in a device trace: the login page focused its PIN field by itself while the view had no focus.
    // When the d-pad moved into the view, WebKit reported the field with INHIBIT_OSK (focus without a user gesture,
    // WebPageGLib.cpp:216-217) from inside set_focused(1), and taps on the field brought no new report.
    const unsigned    autofocused = UC_WEBVIEW_IME_HINT_SPELLCHECK | UC_WEBVIEW_IME_HINT_INHIBIT_OSK;  // 0x81
    const QStringList shown{QStringLiteral("show")};
    Fixture           f;
    f.focusWindow();
    stub::s.replayFocus = true;
    stub::s.replayHints = autofocused;
    stub::s.replayPurpose = UC_WEBVIEW_IME_PURPOSE_PASSWORD;

    // the d-pad moves into the view: the replayed report enables the input method but opens no keyboard
    f.item->forceActiveFocus();
    QVERIFY(f.item->inputMethodQuery(Qt::ImEnabled).toBool());
    QVERIFY(Qt::InputMethodHints(f.item->inputMethodQuery(Qt::ImHints).toInt()) & Qt::ImhHiddenText);
    QVERIFY(stub::s.panel.isEmpty());

    // a tap on the field: no report from WebKit, the keyboard comes after the tap delay
    f.touchTap(QPointF(100, 50), QPointF(102, 51));
    QVERIFY(stub::s.panel.isEmpty());
    QTRY_COMPARE(stub::s.panel, shown);

    // the same when the first touch into the view lands on the field: the press gives the item the focus (the
    // report is replayed inside set_focused(1)), the release is the tap
    QQuickItem other(f.window.contentItem());
    other.forceActiveFocus();
    QVERIFY(!f.item->hasActiveFocus());
    stub::s.panel.clear();
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QVERIFY(f.item->hasActiveFocus());
    QTRY_COMPARE(stub::s.panel, shown);

    // the page focuses an element by itself while the view has the focus (a report long after the user's last tap):
    // no keyboard for the focus, but for a tap on it
    stub::fireImeFocus(0, 0, 0, nullptr, -1);
    stub::s.panel.clear();
    QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 50);
    stub::fireImeFocus(1, autofocused, UC_WEBVIEW_IME_PURPOSE_PASSWORD, "", 0);
    QVERIFY(stub::s.panel.isEmpty());
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QTRY_COMPARE(stub::s.panel, shown);
}

void testWebViewItem::inputmodeNoneHoldsForTapsAfterTheUsersFocus() {
    // inputmode="none" (a page with a keypad of its own): WebKit reports the element with INHIBIT_OSK right after
    // the tap that focused it. Taps on the element, or on the page's keypad, which keeps the focus in the element,
    // open no keyboard.
    const unsigned none = UC_WEBVIEW_IME_HINT_INHIBIT_OSK;
    const int      settle = WebViewItem::kTapInputPanelDelayMs + 50;
    Fixture        f;
    f.focusWindow();
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    stub::fireImeFocus(1, none, UC_WEBVIEW_IME_PURPOSE_FREE_FORM, "", 0);
    QVERIFY(f.item->inputMethodQuery(Qt::ImEnabled).toBool());
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QTest::qWait(settle);
    f.touchTap(QPointF(100, 150), QPointF(100, 150));
    QTest::qWait(settle);
    QVERIFY(stub::s.panel.isEmpty());

    // from a field with the keyboard to the inputmode="none" element: the tap's pending request is dropped once the
    // element's report arrives
    stub::fireImeFocus(0, 0, 0, nullptr, -1);
    f.touchTap(QPointF(100, 20), QPointF(100, 20));
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_FREE_FORM, "", 0);
    QCOMPARE(stub::s.panel, (QStringList{QStringLiteral("hide"), QStringLiteral("show")}));
    stub::s.panel.clear();
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    stub::fireImeFocus(1, none, UC_WEBVIEW_IME_PURPOSE_FREE_FORM, "", 0);
    QTest::qWait(settle);
    QVERIFY(stub::s.panel.isEmpty());

    // leaving the view and coming back keeps it: the replayed report cannot tell, the item remembers the element
    stub::s.replayFocus = true;
    stub::s.replayHints = none;
    stub::s.replayPurpose = UC_WEBVIEW_IME_PURPOSE_FREE_FORM;
    QQuickItem other(f.window.contentItem());
    other.forceActiveFocus();
    QVERIFY(!f.item->hasActiveFocus());
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QVERIFY(f.item->hasActiveFocus());
    QTest::qWait(settle);
    QVERIFY(stub::s.panel.isEmpty());

    // a new document forgets it: the element the next page focuses by itself gets the keyboard on a tap
    other.forceActiveFocus();
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/next");
    stub::s.replayHints = UC_WEBVIEW_IME_HINT_SPELLCHECK | UC_WEBVIEW_IME_HINT_INHIBIT_OSK;
    f.item->forceActiveFocus();
    QVERIFY(stub::s.panel.isEmpty());
    f.touchTap(QPointF(100, 50), QPointF(100, 50));
    QTRY_COMPARE(stub::s.panel, QStringList{QStringLiteral("show")});
}

void testWebViewItem::tapDistanceIsTheLibrarysDragThreshold() {
    const int settle = WebViewItem::kTapInputPanelDelayMs + 50;
    qunsetenv("UC_WEBVIEW_DRAG_THRESHOLD");
    // library 0.3.0 reports its display's threshold (16 px): a press that moves 12 px is still a click for WebKit
    // and a tap for the keyboard request, 20 px is a drag
    {
        Fixture f;
        f.focusWindow();
        QCOMPARE(WebViewLibrary::instance()->dragThreshold(), 16.0);
        f.touchTap(QPointF(100, 50), QPointF(100, 50));
        stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
        stub::s.panel.clear();
        f.touchTap(QPointF(100, 50), QPointF(100, 62));
        QTRY_COMPARE(stub::s.panel, QStringList{QStringLiteral("show")});
        f.touchTap(QPointF(100, 50), QPointF(100, 70));
        QTest::qWait(settle);
        QCOMPARE(stub::s.panel.size(), 1);
    }
    // a library before 0.3.0 reports nothing and leaves WPE's 8 px, or UC_WEBVIEW_DRAG_THRESHOLD
    WebViewFunctions older = stub::functions();
    older.global_get_drag_threshold = nullptr;
    {
        Fixture f(older);
        f.focusWindow();
        QCOMPARE(WebViewLibrary::instance()->dragThreshold(), 8.0);
        f.touchTap(QPointF(100, 50), QPointF(100, 50));
        stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
        stub::s.panel.clear();
        f.touchTap(QPointF(100, 50), QPointF(100, 62));
        QTest::qWait(settle);
        QVERIFY(stub::s.panel.isEmpty());
        f.touchTap(QPointF(100, 50), QPointF(100, 56));
        QTRY_COMPARE(stub::s.panel, QStringList{QStringLiteral("show")});
    }
    qputenv("UC_WEBVIEW_DRAG_THRESHOLD", "30");
    {
        Fixture f(older);
        QCOMPARE(WebViewLibrary::instance()->dragThreshold(), 30.0);
    }
    // a library that reports its value wins: it read the variable itself (the stub says 16)
    {
        Fixture f;
        QCOMPARE(WebViewLibrary::instance()->dragThreshold(), 16.0);
    }
    qunsetenv("UC_WEBVIEW_DRAG_THRESHOLD");
}

namespace {

QStringList *capturedMessages = nullptr;

void captureMessage(QtMsgType, const QMessageLogContext &context, const QString &message) {
    capturedMessages->append(QString::fromLatin1(context.category ? context.category : "default") +
                             QStringLiteral(": ") + message);
}

// the item's trace lines after "uc.ui.webview: trace 0x...: ", nothing else (docs/webview.md)
const char *const kAllowedTraceLines[] = {
    R"(^touch slot \d+ \(qt id \d+\) (down|move|up|cancel) -?[\d.e+-]+,-?[\d.e+-]+ ms \d+ moved [\d.e+-]+ px, \d+ down$)",
    R"(^key printable (down|up) ms \d+$)",
    R"(^key special [A-Za-z0-9_]+ (down|up) \(keysym 0x[0-9a-f]+, scancode \d+\) ms \d+$)",
    R"(^ime_focus (true|false) hints 0x[0-9a-f]+ purpose \d+( replayed with the view's focus)?)"
    R"(( \(keyboard inhibited(, also on taps| until a tap)\))? \((focus owner|another item has the focus)\)$)",
    R"(^ime_cursor_area -?\d+,-?\d+ \d+x\d+ \(view px\)$)",
    R"(^ime_surrounding changed$)",
    R"(^input method event:( commit)?( preedit| preedit end)?( replacement)?( empty)?)"
    R"(( \(ignored, no editable element\))?$)",
    R"(^input panel (show|hide)$)",
    R"(^tap with a focused editable element: keyboard request in \d+ ms unless the page moves its focus$)",
    R"(^active focus (true|false)$)",
    R"(^load (started|redirected|committed|finished|failed) ".*"$)",
    R"(^load_uri ".*"$)",
    R"(^load_html \d+ chars, base ".*"$)",
    R"(^pointer button \d (down|up) -?[\d.]+,-?[\d.]+ ms \d+$)",
    R"(^pointer move -?[\d.]+,-?[\d.]+ ms \d+$)",
    R"(^cancelling \d+ touch point\(s\): .+$)",
    R"(^mouse grab taken by another item, releasing buttons$)",
    R"(^touch id \d+ (down|move|up|cancel) without a press, dropped$)",
    R"(^zoom [\d.]+ \(\d+ %\)$)",
    R"(^zoom [\d.]+ \(\d+ %\) waits until the page rendered \d+ %$)",
    R"(^zoom \d+ % rendered after \d+ ms \((done|timeout|cancelled|status -?\d+|first frame after the zoom|)"
    R"(no frame within \d+ ms)\)$)",
    R"(^page (up|down): Page_(Up|Down) press and release \(keysym 0x[0-9a-f]+, scancode \d+\)$)",
    R"(^pinch begin at -?[\d.]+,-?[\d.]+, magnification [\d.]+$)",
    R"(^pinch end, scale [\d.e+-]+, magnification [\d.]+( \(after the running render\)| \(no change\))?$)",
    R"(^pinch cancelled$)",
    R"(^magnification [\d.]+ -> [\d.]+ at -?[\d.]+,-?[\d.]+$)",
    R"(^magnification [\d.]+ rendered after \d+ ms \((done|timeout|cancelled|status -?\d+)\)$)",
    R"(^magnification changed to [\d.]+ by a navigation or history step$)",
};

bool traceLineAllowed(const QString &line) {
    for (const char *pattern : kAllowedTraceLines) {
        const QRegularExpressionMatch match = QRegularExpression(QString::fromLatin1(pattern)).match(line);
        if (!match.hasMatch()) {
            continue;
        }
        if (line.startsWith(QLatin1String("key special "))) {
            // the keysym of a special line is one of the named keys, never a character's
            const int at = line.indexOf(QLatin1String("keysym 0x")) + 9;
            const uint32_t keysym = line.mid(at, line.indexOf(QLatin1Char(','), at) - at).toUInt(nullptr, 16);
            return uc::webview::specialKeyName(keysym) != nullptr;
        }
        return true;
    }
    return false;
}

}  // namespace

void testWebViewItem::traceCarriesNoTypedTextOrUrlSecrets() {
    // A password session through the item with the trace on (UC_WEBVIEW_TRACE=1 on the device) and every message of
    // the process captured. No message may carry a typed character, its keysym or scan code, the field's text, its
    // length or caret offsets, or the credentials, query and fragment of a URL (docs/webview.md, "What the trace
    // logs"). The typed characters never occur in the trace's own words.
    const QStringList typed{QStringLiteral("Q"), QStringLiteral("J"), QStringLiteral("Z"),
                            QStringLiteral("§"), QStringLiteral("€"), QStringLiteral("ж"),
                            QStringLiteral("Ω"), QStringLiteral("¶")};
    const QStringList secrets{QStringLiteral("hunter2"), QStringLiteral("alice"), QStringLiteral("pin="),
                              QStringLiteral("token"), QStringLiteral("●")};
    const QList<int>  keys{Qt::Key_Q,       Qt::Key_J,       Qt::Key_Z,       Qt::Key_section,
                          Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_paragraph};
    QStringList       messages;
    bool              committed = false;
    capturedMessages = &messages;
    WebViewItem::setTraceEnabledForTest(true);
    const QtMessageHandler previous = qInstallMessageHandler(captureMessage);
    {
        Fixture f;
        f.focusWindow();
        const QUrl login(QStringLiteral("https://alice:hunter2@remote.local/login?pin=QJZ#token=x"));
        f.item->setUrl(login);
        stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, login.toEncoded().constData());
        stub::fireLoadChanged(UC_WEBVIEW_LOAD_FAILED, login.toEncoded().constData());
        stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, login.toEncoded().constData());

        // the user taps the password field, which holds a saved password; WebKit reports the masked text
        f.touchTap(QPointF(100, 50), QPointF(101, 50));
        stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_PRIVATE, UC_WEBVIEW_IME_PURPOSE_PASSWORD, "QJZ", 3);
        stub::fireImeCursorArea(60, 40, 1, 20);

        // a hardware keyboard: characters with their scan codes, the field reports its text after each
        QString text;
        QString bullets;
        for (int i = 0; i < typed.size(); ++i) {
            const quint32 scanCode = 101 + i;
            f.sendKey(QEvent::KeyPress, keys.at(i), typed.at(i), Qt::NoModifier, scanCode);
            f.sendKey(QEvent::KeyRelease, keys.at(i), typed.at(i), Qt::NoModifier, scanCode);
            text += typed.at(i);
            bullets += QChar(0x25cf);
            stub::fireImeSurrounding(text.toUtf8().constData(), i + 1, i + 1);
            stub::fireImeSurrounding(bullets.toUtf8().constData(), i + 1, i + 1);
        }
        f.sendKey(QEvent::KeyPress, Qt::Key_Backspace, QStringLiteral("\b"), Qt::NoModifier, 22);

        // the on-screen keyboard: commits, a composition, a correction, the end
        QInputMethodEvent commit;
        commit.setCommitString(typed.at(0) + typed.at(1));
        QCoreApplication::sendEvent(f.item, &commit);
        QList<QInputMethodEvent::Attribute> attributes{
            QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 2, 1, QVariant())};
        QInputMethodEvent preedit(typed.at(3) + typed.at(4), attributes);
        QCoreApplication::sendEvent(f.item, &preedit);
        QInputMethodEvent replace;
        replace.setCommitString(typed.at(5) + typed.at(6), -1, 1);
        QCoreApplication::sendEvent(f.item, &replace);
        QInputMethodEvent empty;
        QCoreApplication::sendEvent(f.item, &empty);
        stub::fireImeSurrounding(text.toUtf8().constData(), 6, 2);
        f.sendKey(QEvent::KeyPress, Qt::Key_Return, QStringLiteral("\r"), Qt::NoModifier, 36);
        stub::fireImeFocus(0, 0, 0, nullptr, -1);
        committed = stub::s.calls.contains(QStringLiteral("ime_commit(%1)").arg(typed.at(0) + typed.at(1)));
    }
    qInstallMessageHandler(previous);
    WebViewItem::setTraceEnabledForTest(false);
    capturedMessages = nullptr;

    QVERIFY(committed);  // the library got the text: only the log does not
    const QRegularExpression traceLine(QStringLiteral("^uc\\.ui\\.webview: trace 0x[0-9a-f]+: (.*)$"));
    int                      traced = 0;
    for (const QString &message : messages) {
        for (const QString &secret : secrets) {
            QVERIFY2(!message.contains(secret), qPrintable(message));
        }
        for (int i = 0; i < typed.size(); ++i) {
            QVERIFY2(!message.contains(QStringLiteral("scancode %1").arg(101 + i)), qPrintable(message));
            QVERIFY2(!message.contains(QStringLiteral("keysym 0x%1").arg(uc::webview::xkbKeysymForKey(keys.at(i),
                                                                                                     typed.at(i)),
                                                                         0, 16)),
                     qPrintable(message));
            if (typed.at(i).at(0).unicode() > 0x7f) {
                QVERIFY2(!message.contains(typed.at(i)), qPrintable(message));  // in no message at all
            }
        }
        const QRegularExpressionMatch match = traceLine.match(message);
        if (!match.hasMatch()) {
            continue;
        }
        ++traced;
        QVERIFY2(traceLineAllowed(match.captured(1)), qPrintable(message));
        for (const QString &character : typed) {
            QVERIFY2(!match.captured(1).contains(character), qPrintable(message));
        }
    }
    // the trace still tells what happened
    const QString all = messages.join(QLatin1Char('\n'));
    QVERIFY(traced > 30);
    QCOMPARE(all.count(QStringLiteral(": key printable down")), typed.size());
    QVERIFY(all.contains(QStringLiteral(": key special BackSpace down (keysym 0xff08, scancode 22)")));
    QVERIFY(all.contains(QStringLiteral(": ime_focus true hints 0x800 purpose 8 (focus owner)")));
    QCOMPARE(all.count(QStringLiteral(": ime_surrounding changed")), 2 * typed.size() + 1);
    QVERIFY(all.contains(QStringLiteral(": input method event: commit\n")));
    QVERIFY(all.contains(QStringLiteral(": input method event: preedit\n")));
    QVERIFY(all.contains(QStringLiteral(": input method event: commit preedit end replacement\n")));
    QVERIFY(all.contains(QStringLiteral(": load started \"https://remote.local/login?...#...\"")));
    QVERIFY(all.contains(QStringLiteral("Load failed: \"https://remote.local/login?...#...\"")));
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
    // VOLUME and CHANNEL as well: the developer page zooms and scrolls with them through zoomIn() and pageDown()
    f.sendKey(QEvent::KeyPress, Qt::Key_VolumeDown);
    QVERIFY(!f.lastAccepted);
    f.sendKey(QEvent::KeyPress, Qt::Key_ChannelUp);
    QVERIFY(!f.lastAccepted);
    f.sendKey(QEvent::KeyPress, Qt::Key_ChannelDown);
    QVERIFY(!f.lastAccepted);
    QCOMPARE(stub::s.calls.size(), before + 3);
}

void testWebViewItem::zoomStepsThroughTheBrowserLevelsAndStopsAtTheEnds() {
    Fixture    f;
    QSignalSpy changed(f.item, &WebViewItem::zoomChanged);
    auto       zoomCalls = [] { return stub::s.calls.filter(QStringLiteral("set_zoom(")); };

    // a new view is at WebKit's 100 %, the item does not set it
    QCOMPARE(f.item->zoom(), 1.0);
    QVERIFY(zoomCalls().isEmpty());

    // in: one level per call up to 300 %, then nothing (no library call, no signal)
    const QList<qreal> in{1.1, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0};
    for (int i = 0; i < in.size(); ++i) {
        f.item->zoomIn();
        QCOMPARE(f.item->zoom(), in.at(i));
        QCOMPARE(zoomCalls().last(), QStringLiteral("set_zoom(%1)").arg(in.at(i)));
        QCOMPARE(changed.count(), i + 1);
        stub::finishRenders();  // the page rendered the step (zoomWaitsForThePageToRender... covers the wait)
    }
    f.item->zoomIn();
    QCOMPARE(f.item->zoom(), 3.0);
    QCOMPARE(zoomCalls().size(), in.size());
    QCOMPARE(changed.count(), in.size());

    // out: all the way down to 50 %, then nothing
    const QList<qreal> out{2.5, 2.0, 1.75, 1.5, 1.25, 1.1, 1.0, 0.9, 0.8, 0.75, 0.67, 0.5};
    for (const qreal level : out) {
        f.item->zoomOut();
        QCOMPARE(f.item->zoom(), level);
        QCOMPARE(zoomCalls().last(), QStringLiteral("set_zoom(%1)").arg(level));
        stub::finishRenders();
    }
    f.item->zoomOut();
    QCOMPARE(f.item->zoom(), 0.5);
    QCOMPARE(zoomCalls().size(), in.size() + out.size());

    // a zoom between two levels goes to the next one in either direction
    f.item->setZoom(1.2);
    QCOMPARE(zoomCalls().last(), QStringLiteral("set_zoom(1.2)"));
    stub::finishRenders();
    f.item->zoomIn();
    QCOMPARE(f.item->zoom(), 1.25);
    stub::finishRenders();
    f.item->setZoom(1.2);
    stub::finishRenders();
    f.item->zoomOut();
    QCOMPARE(f.item->zoom(), 1.1);
    stub::finishRenders();

    // beyond the last level the steps lead back into the table, beyond the first one as well
    f.item->setZoom(4.0);
    stub::finishRenders();
    f.item->zoomIn();
    QCOMPARE(f.item->zoom(), 4.0);
    f.item->zoomOut();
    QCOMPARE(f.item->zoom(), 3.0);
    stub::finishRenders();
    f.item->setZoom(0.3);
    stub::finishRenders();
    f.item->zoomOut();
    QCOMPARE(f.item->zoom(), 0.3);
    f.item->zoomIn();
    QCOMPARE(f.item->zoom(), 0.5);
    stub::finishRenders();

    // the same zoom again calls nothing
    const int calls = zoomCalls().size();
    const int emitted = changed.count();
    f.item->setZoom(0.5);
    QCOMPARE(zoomCalls().size(), calls);
    QCOMPARE(changed.count(), emitted);

    // WebKit keeps the zoom across navigations and a web process crash: no call after a load or a crash reload
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/2");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_COMMITTED, "http://localhost/2");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://localhost/2");
    stub::fireTerminated(UC_WEBVIEW_TERMINATED_CRASHED);
    QTRY_VERIFY(stub::s.calls.contains(QStringLiteral("reload")));
    QCOMPARE(zoomCalls().size(), calls);
    QCOMPARE(f.item->zoom(), 0.5);
}

void testWebViewItem::zoomOutsideTheLibrarysRangeIsRefused() {
    Fixture    f;
    QSignalSpy changed(f.item, &WebViewItem::zoomChanged);

    // uc_webview_set_zoom() ignores these with a warning: the item does not pass them on and keeps its zoom, which
    // is what WebKit uses
    for (const qreal zoom : {0.0, -1.0, 20.5, qQNaN(), qInf()}) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("^Zoom .* refused")));
        f.item->setZoom(zoom);
    }
    QCOMPARE(f.item->zoom(), 1.0);
    QCOMPARE(changed.count(), 0);
    QVERIFY(stub::s.calls.filter(QStringLiteral("set_zoom(")).isEmpty());

    // the limits themselves are taken
    f.item->setZoom(20.0);
    stub::finishRenders();
    f.item->setZoom(0.01);
    QCOMPARE(stub::s.calls.filter(QStringLiteral("set_zoom(")),
             (QStringList{QStringLiteral("set_zoom(20)"), QStringLiteral("set_zoom(0.01)")}));
}

void testWebViewItem::zoomSetBeforeTheViewIsAppliedWhenItIsCreated() {
    // without a size the item has no view yet: the zoom waits for it
    Fixture f(stub::functions(), QSizeF());
    f.item->setUrl(QUrl(QStringLiteral("http://localhost/")));
    f.item->zoomIn();
    f.item->zoomIn();
    QCOMPARE(f.item->zoom(), 1.25);
    QVERIFY(stub::s.view == nullptr);
    QVERIFY(stub::s.calls.filter(QStringLiteral("set_zoom(")).isEmpty());

    // the new view starts at WebKit's 100 %: the item sets its zoom right after creating it, before the first load
    f.item->setSize(QSizeF(300, 200));
    QVERIFY(stub::s.view != nullptr);
    const int created = stub::s.calls.indexOf(QRegularExpression(QStringLiteral("new\\(.*")));  // exact match
    const int zoomed = stub::s.calls.indexOf(QStringLiteral("set_zoom(1.25)"));
    const int loaded = stub::s.calls.indexOf(QStringLiteral("load_uri(http://localhost/)"));
    QVERIFY(created >= 0);
    QVERIFY(zoomed > created);
    QVERIFY(loaded > zoomed);
    QCOMPARE(stub::s.calls.filter(QStringLiteral("set_zoom(")).size(), 1);
    // nothing is rendered yet: no render wait, no indicator
    QVERIFY(!stub::s.calls.contains(QStringLiteral("when_rendered")));
    QVERIFY(!f.item->isZoomRendering());
}

void testWebViewItem::zoomWaitsForThePageToRenderAndPassesOnlyTheLatestLevel() {
    // A large page takes seconds to re-render a zoom step. WebKit does the whole relayout
    // for every level it gets, so presses during a render only move `zoom`; the latest level follows once the page
    // rendered the current one: three quick steps, two relayouts.
    Fixture    f;
    QSignalSpy rendering(f.item, &WebViewItem::zoomRenderingChanged);
    QSignalSpy changed(f.item, &WebViewItem::zoomChanged);
    auto       zoomCalls = [] { return stub::s.calls.filter(QRegularExpression(QStringLiteral("^(set_zoom|when_)"))); };

    f.item->zoomIn();
    QCOMPARE(zoomCalls(), (QStringList{QStringLiteral("set_zoom(1.1)"), QStringLiteral("when_rendered")}));
    QVERIFY(f.item->isZoomRendering());
    QCOMPARE(rendering.count(), 1);

    f.item->zoomIn();
    f.item->zoomIn();
    QCOMPARE(f.item->zoom(), 1.5);   // the level follows every press
    QCOMPARE(changed.count(), 3);
    QCOMPARE(zoomCalls().size(), 2);  // WebKit still renders 110 %
    QVERIFY(f.item->isZoomRendering());
    QCOMPARE(rendering.count(), 1);

    stub::finishRender();  // 110 % rendered: 150 % goes to WebKit, the indicator keeps running
    QCOMPARE(zoomCalls(), (QStringList{QStringLiteral("set_zoom(1.1)"), QStringLiteral("when_rendered"),
                                       QStringLiteral("set_zoom(1.5)"), QStringLiteral("when_rendered")}));
    QVERIFY(f.item->isZoomRendering());
    QCOMPARE(rendering.count(), 1);

    stub::finishRender();  // 150 % rendered: done
    QVERIFY(!f.item->isZoomRendering());
    QCOMPARE(rendering.count(), 2);
    QCOMPARE(zoomCalls().size(), 4);
    QVERIFY(stub::s.renders.isEmpty());
    QCOMPARE(f.item->zoom(), 1.5);

    // a step after the render is passed on at once
    f.item->zoomOut();
    QCOMPARE(zoomCalls().mid(4), (QStringList{QStringLiteral("set_zoom(1.25)"), QStringLiteral("when_rendered")}));
    QVERIFY(f.item->isZoomRendering());
}

void testWebViewItem::zoomBackToTheRenderedLevelNeedsNoRelayout() {
    Fixture f;
    f.item->zoomIn();  // 110 % to WebKit
    f.item->zoomIn();  // 125 % waits
    f.item->zoomOut();  // back to 110 %: what WebKit renders anyway
    QCOMPARE(f.item->zoom(), 1.1);
    stub::finishRender();
    QCOMPARE(stub::s.calls.filter(QStringLiteral("set_zoom(")), QStringList{QStringLiteral("set_zoom(1.1)")});
    QCOMPARE(stub::s.calls.count(QStringLiteral("when_rendered")), 1);
    QVERIFY(!f.item->isZoomRendering());
}

void testWebViewItem::zoomWaitEndsOnTimeoutAndCancellation() {
    Fixture f;
    auto    zoomCalls = [] { return stub::s.calls.filter(QStringLiteral("set_zoom(")); };

    // the library's timeout (60 s from 0.6.0) (page busy, view unmapped): the indicator stops
    f.item->zoomIn();
    stub::finishRender(UC_WEBVIEW_RENDERED_TIMEOUT);
    QVERIFY(!f.item->isZoomRendering());

    // a navigation commits (or the web process dies) during the render: the waiting level goes to WebKit now
    f.item->zoomIn();   // 125 %
    f.item->zoomIn();   // 150 % waits
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/2");
    stub::finishRender(UC_WEBVIEW_RENDERED_CANCELLED);
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_COMMITTED, "http://localhost/2");
    QCOMPARE(zoomCalls(), (QStringList{QStringLiteral("set_zoom(1.1)"), QStringLiteral("set_zoom(1.25)"),
                                       QStringLiteral("set_zoom(1.5)")}));
    QVERIFY(f.item->isZoomRendering());
    stub::finishRender(UC_WEBVIEW_RENDERED_CANCELLED);
    QVERIFY(!f.item->isZoomRendering());
    QCOMPARE(zoomCalls().size(), 3);

    // unknown statuses of a newer library end the wait like the others
    f.item->zoomIn();
    stub::finishRender(42);
    QVERIFY(!f.item->isZoomRendering());
}

void testWebViewItem::zoomWaitEndsQuietlyWhenTheItemGoes() {
    // uc_webview_free() ends the pending request with CANCELLED inside the call: the item, half destroyed, ignores it
    auto *f = new Fixture;
    f->item->zoomIn();
    f->item->zoomIn();  // 125 % waits
    QVERIFY(f->item->isZoomRendering());
    QVERIFY(!stub::s.renders.isEmpty());
    delete f;
    const int freed = stub::s.calls.indexOf(QStringLiteral("free"));
    QVERIFY(freed > 0);
    QVERIFY(stub::s.calls.mid(freed + 1).isEmpty());  // no zoom for a freed view
    QVERIFY(stub::s.renders.isEmpty());
}

void testWebViewItem::zoomWithoutWhenRenderedWaitsForTheFirstFrame() {
    // a library older than 0.5.0: the first frame after the zoom ends the wait (after the frame callback)
    Fixture f(stub::functionsWithoutWhenRendered());
    auto    zoomCalls = [] { return stub::s.calls.filter(QStringLiteral("set_zoom(")); };

    f.item->zoomIn();
    QVERIFY(f.item->isZoomRendering());
    f.item->zoomIn();  // 125 % waits for the frame
    QCOMPARE(zoomCalls(), QStringList{QStringLiteral("set_zoom(1.1)")});
    stub::fireFrame(1);
    QVERIFY(f.item->isZoomRendering());  // not inside the frame callback
    QTRY_COMPARE(zoomCalls(), (QStringList{QStringLiteral("set_zoom(1.1)"), QStringLiteral("set_zoom(1.25)")}));
    QVERIFY(f.item->isZoomRendering());
    stub::fireFrame(2);
    QTRY_VERIFY(!f.item->isZoomRendering());
    QVERIFY(!stub::s.calls.contains(QStringLiteral("when_rendered")));

    // frames without a zoom change nothing
    stub::fireFrame(3);
    QTest::qWait(20);
    QCOMPARE(zoomCalls().size(), 2);
    QVERIFY(!f.item->isZoomRendering());
}

void testWebViewItem::zoomWithoutWhenRenderedEndsAfterTheFallbackTimeout() {
    // no frame at all after the zoom (nothing changed on screen): the wait ends after the fallback's timeout
    WebViewItem::setZoomFallbackTimeoutForTest(100);
    Fixture       f(stub::functionsWithoutWhenRendered());
    QElapsedTimer clock;
    clock.start();
    f.item->zoomIn();
    QVERIFY(f.item->isZoomRendering());
    QTRY_VERIFY_WITH_TIMEOUT(!f.item->isZoomRendering(), 2000);
    QVERIFY(clock.elapsed() >= 100);
    WebViewItem::setZoomFallbackTimeoutForTest(0);
}

void testWebViewItem::refusedWhenRenderedFallsBackToTheFirstFrame() {
    Fixture f;
    stub::s.renderResult = -1;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("refused the request")));
    f.item->zoomIn();
    QVERIFY(f.item->isZoomRendering());
    stub::fireFrame(1);
    QTRY_VERIFY(!f.item->isZoomRendering());
}

void testWebViewItem::zoomRenderTraceLines() {
    // UC_WEBVIEW_TRACE=1: the zoom steps, the waiting level and the render time per step, in the documented format
    QStringList messages;
    capturedMessages = &messages;
    WebViewItem::setTraceEnabledForTest(true);
    const QtMessageHandler previous = qInstallMessageHandler(captureMessage);
    {
        Fixture f;
        f.item->zoomIn();
        f.item->zoomIn();
        stub::finishRender();
        stub::finishRender(UC_WEBVIEW_RENDERED_TIMEOUT);
    }
    {
        WebViewItem::setZoomFallbackTimeoutForTest(50);
        Fixture f(stub::functionsWithoutWhenRendered());
        f.item->zoomIn();
        stub::fireFrame(1);
        QTRY_VERIFY(!f.item->isZoomRendering());
        f.item->zoomIn();
        QTRY_VERIFY_WITH_TIMEOUT(!f.item->isZoomRendering(), 2000);
        WebViewItem::setZoomFallbackTimeoutForTest(0);
    }
    qInstallMessageHandler(previous);
    WebViewItem::setTraceEnabledForTest(false);
    capturedMessages = nullptr;

    const QRegularExpression traceLine(QStringLiteral("^uc\\.ui\\.webview: trace 0x[0-9a-f]+: (zoom .*)$"));
    QStringList              traced;
    for (const QString &message : messages) {
        const QRegularExpressionMatch match = traceLine.match(message);
        if (match.hasMatch()) {
            QVERIFY2(traceLineAllowed(match.captured(1)), qPrintable(message));
            traced << match.captured(1).replace(QRegularExpression(QStringLiteral("after \\d+ ms")),
                                                QStringLiteral("after N ms"));
        }
    }
    QCOMPARE(traced, (QStringList{QStringLiteral("zoom 1.1 (110 %)"),
                                  QStringLiteral("zoom 1.25 (125 %) waits until the page rendered 110 %"),
                                  QStringLiteral("zoom 110 % rendered after N ms (done)"),
                                  QStringLiteral("zoom 1.25 (125 %)"),
                                  QStringLiteral("zoom 125 % rendered after N ms (timeout)"),
                                  QStringLiteral("zoom 1.1 (110 %)"),
                                  QStringLiteral("zoom 110 % rendered after N ms (first frame after the zoom)"),
                                  QStringLiteral("zoom 1.25 (125 %)"),
                                  QStringLiteral("zoom 125 % rendered after N ms (no frame within 50 ms)")}));
}

void testWebViewItem::pageUpAndDownPressAndReleaseThePageKeys() {
    Fixture f;
    // whichever item has the focus: the page keys come from the page's button navigation, not from a key event
    QVERIFY(!f.item->hasActiveFocus());
    const int before = stub::s.calls.size();

    QStringList messages;
    capturedMessages = &messages;
    WebViewItem::setTraceEnabledForTest(true);
    const QtMessageHandler previous = qInstallMessageHandler(captureMessage);
    const qint64           start = QWindowSystemInterfacePrivate::eventTime.elapsed();
    f.item->pageDown();
    f.item->pageUp();
    const qint64 end = QWindowSystemInterfacePrivate::eventTime.elapsed();
    qInstallMessageHandler(previous);
    WebViewItem::setTraceEnabledForTest(false);
    capturedMessages = nullptr;

    // XKB_KEY_Page_Down/Page_Up, a press and a release each, no modifiers (WebKit scrolls only without them), and
    // the keycodes of a keyboard's keys (evdev + 8) for KeyboardEvent.code; nothing else is called
    QCOMPARE(stub::s.calls.mid(before),
             (QStringList{QStringLiteral("key(0xff56,1,0)"), QStringLiteral("key(0xff56,0,0)"),
                          QStringLiteral("key(0xff55,1,0)"), QStringLiteral("key(0xff55,0,0)")}));
    QCOMPARE(stub::s.keycodes, (QList<uint32_t>{117, 117, 112, 112}));
    // the time of Qt's input events on the device (eglfs stamps them with QWindowSystemInterfacePrivate::eventTime),
    // never 0, which the library would turn into CLOCK_MONOTONIC, another base for WebKit (1 in the first ms)
    QCOMPARE(stub::s.keyTimes.size(), 4);
    for (const uint32_t ms : stub::s.keyTimes) {
        QVERIFY2(ms >= std::max<qint64>(1, start) && ms <= std::max<qint64>(1, end),
                 qPrintable(QStringLiteral("%1 not in %2..%3").arg(ms).arg(start).arg(end)));
    }
    QVERIFY(!f.item->hasActiveFocus());

    // one trace line per key, in the documented format
    const QRegularExpression traceLine(QStringLiteral("^uc\\.ui\\.webview: trace 0x[0-9a-f]+: (.*)$"));
    QStringList              traced;
    for (const QString &message : messages) {
        const QRegularExpressionMatch match = traceLine.match(message);
        if (match.hasMatch()) {
            QVERIFY2(traceLineAllowed(match.captured(1)), qPrintable(message));
            traced << match.captured(1);
        }
    }
    QCOMPARE(traced,
             (QStringList{QStringLiteral("page down: Page_Down press and release (keysym 0xff56, scancode 117)"),
                          QStringLiteral("page up: Page_Up press and release (keysym 0xff55, scancode 112)")}));
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

// ---- pinch zoom (library 0.6.0) ---------------------------------------------------------------------------------

void testWebViewItem::pinchCallbacksNeedTheSizedViewAndTheMagnificationFunctions() {
    // a 0.6.0 library: the view comes from uc_webview_new_sized() with this build's struct and the pinch callbacks
    Fixture f;
    QVERIFY(f.item->isPinchZoomAvailable());
    QVERIFY(stub::s.createdSized);
    QCOMPARE(stub::s.callbacksSize, sizeof(uc_webview_callbacks));
    QVERIFY(stub::s.callbacks.pinch != nullptr);
    QVERIFY(stub::s.callbacks.magnification_changed != nullptr);
    QVERIFY(stub::s.callbacks.frame_ready != nullptr);
    QCOMPARE(f.item->magnification(), 1.0);
    QVERIFY(!f.item->isPinching());
}

void testWebViewItem::olderLibrariesHaveNoPinchZoom() {
    {
        // 0.5.x: uc_webview_new_sized() with the struct of this build, but no pinch callbacks: the library keeps
        // two fingers as they were (the first one taps or scrolls)
        Fixture f(stub::functionsWithoutPinch());
        QVERIFY(!f.item->isPinchZoomAvailable());
        QVERIFY(stub::s.createdSized);
        QVERIFY(stub::s.callbacks.pinch == nullptr);
        QVERIFY(stub::s.callbacks.magnification_changed == nullptr);
        QCOMPARE(f.item->magnification(), 1.0);
        // VOLUME zoom works as before
        f.item->zoomIn();
        QVERIFY(stub::s.calls.contains(QStringLiteral("set_zoom(1.1)")));
    }
    {
        // older than 0.4.0: uc_webview_new()
        Fixture f(stub::functionsWithoutNewSized());
        QVERIFY(!f.item->isPinchZoomAvailable());
        QVERIFY(!stub::s.createdSized);
        QVERIFY(stub::s.calls.contains(QStringLiteral("new(300x200)")));
        QVERIFY(stub::s.callbacks.pinch == nullptr);
    }
}

void testWebViewItem::pinchScalesTheFrameAndCommitsOnceAtTheEnd() {
    Fixture    f;
    QSignalSpy magnification(f.item, &WebViewItem::magnificationChanged);
    QSignalSpy pinching(f.item, &WebViewItem::pinchingChanged);
    QSignalSpy rendering(f.item, &WebViewItem::zoomRenderingChanged);

    stub::firePinch(UC_WEBVIEW_PINCH_BEGIN, 1.0, 100, 150);
    QVERIFY(f.item->isPinching());
    QCOMPARE(pinching.count(), 1);
    stub::firePinch(UC_WEBVIEW_PINCH_UPDATE, 1.5, 102, 149);
    QCOMPARE(f.item->magnification(), 1.5);
    stub::firePinch(UC_WEBVIEW_PINCH_UPDATE, 2.0, 103, 151);
    QCOMPARE(f.item->magnification(), 2.0);
    QCOMPARE(magnification.count(), 2);
    // the picture is the item's alone while the fingers move: nothing goes to the page
    QVERIFY(stub::magnificationCalls().isEmpty());
    QVERIFY(!f.item->isZoomRendering());

    // the end: one magnification at the point where the pinch began, and the wait for the page
    stub::firePinch(UC_WEBVIEW_PINCH_END, 2.0, 103, 151);
    QVERIFY(!f.item->isPinching());
    QCOMPARE(pinching.count(), 2);
    QCOMPARE(stub::magnificationCalls(),
             (QStringList{QStringLiteral("set_magnification(2,100,150)"), QStringLiteral("when_rendered")}));
    QCOMPARE(f.item->magnification(), 2.0);
    QVERIFY(f.item->isZoomRendering());
    QCOMPARE(rendering.count(), 1);

    stub::finishRender();
    QVERIFY(!f.item->isZoomRendering());
    QCOMPARE(rendering.count(), 2);
    QCOMPARE(f.item->magnification(), 2.0);
    QCOMPARE(magnification.count(), 2);  // the sharp frame shows the same magnification
}

void testWebViewItem::scaledFrameIsHeldUntilThePageRenderedTheMagnification() {
    // While the page renders the magnification the item keeps its scaled frame. The frames that arrive meanwhile are
    // reported rendered at once, so that WebKit composites on and the page's rendering update can finish; they are
    // not shown (FrameQueue::setHeld). After DONE the frames follow the usual path again (reported after the swap).
    Fixture f;
    stub::fireFrame(1);
    stub::pinch(2.0, 150, 100);
    QCOMPARE(stub::magnificationCalls().size(), 2);

    // (the test window never renders, so frame 1 still waits in the queue and each new frame replaces the last)
    const int before = stub::s.calls.size();
    stub::fireFrame(2);
    QCOMPARE(stub::s.calls.mid(before),
             (QStringList{QStringLiteral("frame_release(1)"), QStringLiteral("frame_rendered(2)")}));
    // a newer one replaces it, which goes back
    stub::fireFrame(3);
    QCOMPARE(stub::s.calls.mid(before),
             (QStringList{QStringLiteral("frame_release(1)"), QStringLiteral("frame_rendered(2)"),
                          QStringLiteral("frame_release(2)"), QStringLiteral("frame_rendered(3)")}));

    stub::finishRender();
    const int after = stub::s.calls.size();
    stub::fireFrame(4);
    // no early report: the scene graph shows this one and reports it after the swap
    QCOMPARE(stub::s.calls.mid(after), QStringList{QStringLiteral("frame_release(3)")});
    QCOMPARE(f.item->framesReceived(), 4);
}

void testWebViewItem::magnificationTimeoutAndCancelShowTheNextFrame() {
    Fixture f;
    // the library's 60 s timeout: the hold ends, the next frame shows what the page has
    stub::pinch(2.0, 150, 100);
    QVERIFY(f.item->isZoomRendering());
    stub::finishRender(UC_WEBVIEW_RENDERED_TIMEOUT);
    QVERIFY(!f.item->isZoomRendering());
    int before = stub::s.calls.size();
    stub::fireFrame(1);
    QVERIFY(!stub::s.calls.mid(before).contains(QStringLiteral("frame_rendered(1)")));
    QCOMPARE(f.item->magnification(), 2.0);

    // cancelled (a navigation committed, the web process died): the same
    stub::pinch(1.5, 150, 100);
    QCOMPARE(stub::s.calls.filter(QStringLiteral("set_magnification(")).last(),
             QStringLiteral("set_magnification(3,150,100)"));
    stub::finishRender(UC_WEBVIEW_RENDERED_CANCELLED);
    QVERIFY(!f.item->isZoomRendering());
    before = stub::s.calls.size();
    stub::fireFrame(2);
    QVERIFY(!stub::s.calls.mid(before).contains(QStringLiteral("frame_rendered(2)")));

    // a refused render request: no hold at all
    stub::s.renderResult = -1;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("refused the request")));
    stub::pinch(0.5, 150, 100);
    QVERIFY(!f.item->isZoomRendering());
    QCOMPARE(f.item->magnification(), 1.5);
    before = stub::s.calls.size();
    stub::fireFrame(3);
    QVERIFY(!stub::s.calls.mid(before).contains(QStringLiteral("frame_rendered(3)")));
}

void testWebViewItem::pinchesDuringAMagnificationRenderAreCommittedTogether() {
    // like the zoom keys: WebKit would re-render the page for every magnification it gets
    Fixture f;
    stub::pinch(2.0, 100, 100);
    QCOMPARE(stub::s.magnifications, QList<double>{2.0});
    stub::pinch(1.5, 50, 50);   // the page still renders 2x
    QCOMPARE(f.item->magnification(), 3.0);
    stub::pinch(1.2, 200, 150);
    QCOMPARE(f.item->magnification(), 3.6);
    QCOMPARE(stub::s.magnifications, QList<double>{2.0});
    QCOMPARE(stub::s.renders.size(), 1);

    stub::finishRender();  // 2x is rendered: both pinches go to the page as one magnification
    QCOMPARE(stub::s.magnifications.size(), 2);
    QVERIFY(qAbs(stub::s.magnifications.last() - 3.6) < 1e-9);
    QVERIFY(f.item->isZoomRendering());
    stub::finishRender();
    QVERIFY(!f.item->isZoomRendering());
    QVERIFY(qAbs(f.item->magnification() - 3.6) < 1e-9);
}

void testWebViewItem::smallOrCancelledPinchCommitsNothing() {
    Fixture    f;
    QSignalSpy magnification(f.item, &WebViewItem::magnificationChanged);
    // below 1 % of change: the picture snaps back, the page is not re-rendered for it
    stub::pinch(1.005, 100, 100);
    QVERIFY(stub::magnificationCalls().isEmpty());
    QCOMPARE(f.item->magnification(), 1.0);
    // a cancelled pinch (the page took the touches over, a navigation)
    stub::firePinch(UC_WEBVIEW_PINCH_BEGIN, 1.0, 100, 100);
    stub::firePinch(UC_WEBVIEW_PINCH_UPDATE, 2.5, 100, 100);
    QCOMPARE(f.item->magnification(), 2.5);
    stub::firePinch(UC_WEBVIEW_PINCH_CANCEL, 2.5, 100, 100);
    QCOMPARE(f.item->magnification(), 1.0);
    QVERIFY(!f.item->isPinching());
    QVERIFY(stub::magnificationCalls().isEmpty());
    // the limits: 1 to 4, a pinch below 1 stops at 1
    stub::pinch(0.3, 100, 100);
    QCOMPARE(f.item->magnification(), 1.0);
    QVERIFY(stub::magnificationCalls().isEmpty());
    stub::pinch(9.0, 100, 100);
    QCOMPARE(f.item->magnification(), 4.0);
    QCOMPARE(stub::s.magnifications, QList<double>{4.0});
    // a phase of a newer library is ignored
    stub::firePinch(7, 2.0, 1, 1);
    QCOMPARE(f.item->magnification(), 4.0);
}

void testWebViewItem::refusedMagnificationSnapsBack() {
    Fixture f;
    stub::s.magnificationResult = -1;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("uc_webview_set_magnification refused")));
    stub::pinch(2.0, 100, 100);
    QCOMPARE(f.item->magnification(), 1.0);
    QVERIFY(!f.item->isZoomRendering());
    QVERIFY(stub::s.renders.isEmpty());
    // frames flow as usual
    const int before = stub::s.calls.size();
    stub::fireFrame(1);
    QVERIFY(stub::s.calls.mid(before).isEmpty());
}

void testWebViewItem::navigationAndHistoryResetTheMagnification() {
    Fixture    f;
    QSignalSpy magnification(f.item, &WebViewItem::magnificationChanged);
    stub::pinch(2.0, 100, 100);
    stub::finishRender();
    QCOMPARE(magnification.count(), 2);

    // a new page: WebKit starts it at 1 (the library reports it after the commit)
    stub::fireMagnificationChanged(1.0);
    QCOMPARE(f.item->magnification(), 1.0);
    QCOMPARE(magnification.count(), 3);
    // back: WebKit restores the page's own magnification
    stub::fireMagnificationChanged(2.0);
    QCOMPARE(f.item->magnification(), 2.0);
    // a pinch from there goes from the restored value
    stub::pinch(1.5, 100, 100);
    QCOMPARE(stub::s.calls.filter(QStringLiteral("set_magnification(")).last(),
             QStringLiteral("set_magnification(3,100,100)"));

    // a navigation during the render: the render request ends as cancelled first, then the reset; the hold is over
    stub::finishRender(UC_WEBVIEW_RENDERED_CANCELLED);
    stub::fireMagnificationChanged(1.0);
    QVERIFY(!f.item->isZoomRendering());
    QCOMPARE(f.item->magnification(), 1.0);
    // a reset while the page renders (a same-document history step): the late report of that render is stale
    stub::pinch(2.0, 100, 100);
    stub::fireMagnificationChanged(1.5);
    QVERIFY(!f.item->isZoomRendering());
    stub::pinch(2.0, 100, 100);   // a new magnification and its own render
    QCOMPARE(stub::s.renders.size(), 2);
    stub::finishRender();         // the old one: not the end of the new hold
    QVERIFY(f.item->isZoomRendering());
    stub::finishRender();
    QVERIFY(!f.item->isZoomRendering());
    QCOMPARE(f.item->magnification(), 3.0);
}

void testWebViewItem::pageZoomAndMagnificationAreIndependent() {
    // VOLUME changes the page zoom (a new layout) and leaves the magnification alone; a pinch leaves the zoom alone.
    // The busy indicator covers both renders.
    Fixture f;
    stub::pinch(2.0, 100, 100);
    f.item->zoomIn();
    QCOMPARE(f.item->zoom(), 1.1);
    QCOMPARE(f.item->magnification(), 2.0);
    QCOMPARE(stub::s.calls.filter(QRegularExpression(QStringLiteral("^(set_zoom|set_magnification)"))),
             (QStringList{QStringLiteral("set_magnification(2,100,100)"), QStringLiteral("set_zoom(1.1)")}));
    QCOMPARE(stub::s.renders.size(), 2);
    QVERIFY(f.item->isZoomRendering());
    stub::finishRender();  // the magnification
    QVERIFY(f.item->isZoomRendering());  // the zoom still renders
    stub::finishRender();
    QVERIFY(!f.item->isZoomRendering());
    QCOMPARE(f.item->zoom(), 1.1);
    QCOMPARE(f.item->magnification(), 2.0);
    // a pinch does not touch the zoom
    stub::pinch(1.5, 100, 100);
    QCOMPARE(f.item->zoom(), 1.1);
    QCOMPARE(stub::s.calls.filter(QStringLiteral("set_zoom(")).size(), 1);
}

void testWebViewItem::twoFingersAreNoTapWithPinchZoom() {
    // The keyboard request on a tap into the focused field (onTap): with the library's pinch recognition a second
    // finger cancels WebKit's tap of the first one, so its release is no tap here either. One finger still is.
    const QStringList once{QStringLiteral("show")};
    const QStringList twice{QStringLiteral("show"), QStringLiteral("show")};
    auto              twoFingers = [](Fixture &f) {
        f.sendTouch(QEvent::TouchBegin, {Fixture::touchPoint(5, Qt::TouchPointPressed, QPointF(100, 50))},
                    Qt::TouchPointPressed);
        f.sendTouch(QEvent::TouchUpdate,
                    {Fixture::touchPoint(5, Qt::TouchPointStationary, QPointF(100, 50)),
                     Fixture::touchPoint(6, Qt::TouchPointPressed, QPointF(150, 50))},
                    Qt::TouchPointPressed | Qt::TouchPointStationary);
        f.sendTouch(QEvent::TouchUpdate,
                    {Fixture::touchPoint(5, Qt::TouchPointStationary, QPointF(100, 50)),
                     Fixture::touchPoint(6, Qt::TouchPointReleased, QPointF(150, 50))},
                    Qt::TouchPointReleased | Qt::TouchPointStationary);
        f.sendTouch(QEvent::TouchEnd, {Fixture::touchPoint(5, Qt::TouchPointReleased, QPointF(100, 50))},
                    Qt::TouchPointReleased);
    };
    {
        Fixture f;
        f.focusWindow();
        f.touchTap(QPointF(100, 50), QPointF(100, 50));
        stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
        QCOMPARE(stub::s.panel, once);
        // two fingers on the field: no request
        twoFingers(f);
        QTest::qWait(WebViewItem::kTapInputPanelDelayMs + 100);
        QCOMPARE(stub::s.panel, once);
        // the touches went to the library as before, with slots 0 and 1
        QVERIFY(stub::s.calls.contains(QStringLiteral("touch(1,0,150,50)")));
        // a one-finger tap still asks, also within the drag threshold
        f.touchTap(QPointF(100, 50), QPointF(105, 53));
        QTRY_COMPARE(stub::s.panel, twice);
    }
    {
        // a library without pinch zoom: WebKit's controller follows the first finger and clicks at its release,
        // so that release stays a tap (the behaviour before 0.6.0)
        Fixture f(stub::functionsWithoutPinch());
        f.focusWindow();
        f.touchTap(QPointF(100, 50), QPointF(100, 50));
        stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PIN, "", 0);
        QCOMPARE(stub::s.panel, once);
        twoFingers(f);
        QTRY_COMPARE(stub::s.panel, twice);
    }
}

void testWebViewItem::magnificationWaitEndsQuietlyWhenTheItemGoes() {
    // uc_webview_free() ends a pending render request with CANCELLED inside the call: the item has ended its hold
    // before and ignores it, without signals from the destructor
    auto      *f = new Fixture();
    QSignalSpy rendering(f->item, &WebViewItem::zoomRenderingChanged);
    stub::pinch(2.0, 100, 100);
    QCOMPARE(rendering.count(), 1);
    delete f;
    QCOMPARE(stub::s.calls.count(QStringLiteral("free")), 1);
    QVERIFY(stub::s.renders.isEmpty());
}

void testWebViewItem::pinchTraceLines() {
    // UC_WEBVIEW_TRACE=1: begin and end of a pinch, the magnification with its focal point and render time, WebKit's
    // own changes; numbers only, in the documented formats
    QStringList messages;
    capturedMessages = &messages;
    WebViewItem::setTraceEnabledForTest(true);
    const QtMessageHandler previous = qInstallMessageHandler(captureMessage);
    {
        Fixture f;
        stub::pinch(2.0, 100, 150);
        stub::finishRender();
        stub::firePinch(UC_WEBVIEW_PINCH_BEGIN, 1.0, 10, 10);
        stub::firePinch(UC_WEBVIEW_PINCH_CANCEL, 1.0, 10, 10);
        stub::pinch(1.001, 10, 10);
        stub::fireMagnificationChanged(1.0);
    }
    qInstallMessageHandler(previous);
    WebViewItem::setTraceEnabledForTest(false);
    capturedMessages = nullptr;

    const QRegularExpression traceLine(QStringLiteral("^uc\\.ui\\.webview: trace 0x[0-9a-f]+: (.*)$"));
    QStringList              traced;
    for (const QString &message : messages) {
        const QRegularExpressionMatch match = traceLine.match(message);
        if (match.hasMatch() && !match.captured(1).startsWith(QLatin1String("load"))) {
            QVERIFY2(traceLineAllowed(match.captured(1)), qPrintable(message));
            traced.append(match.captured(1));
        }
    }
    const QStringList pinchLines = traced.filter(QRegularExpression(QStringLiteral("^(pinch|magnification)")));
    QCOMPARE(pinchLines,
             (QStringList{QStringLiteral("pinch begin at 100,150, magnification 1"),
                          QStringLiteral("pinch end, scale 2, magnification 2"),
                          QStringLiteral("magnification 1 -> 2 at 100,150"), pinchLines.value(3),
                          QStringLiteral("pinch begin at 10,10, magnification 2"), QStringLiteral("pinch cancelled"),
                          QStringLiteral("pinch begin at 10,10, magnification 2"),
                          QStringLiteral("pinch end, scale 1.001, magnification 2 (no change)"),
                          QStringLiteral("magnification changed to 1 by a navigation or history step")}));
    QVERIFY(pinchLines.at(3).startsWith(QLatin1String("magnification 2 rendered after ")));
    QVERIFY(pinchLines.at(3).endsWith(QLatin1String(" ms (done)")));
}

QTEST_MAIN(testWebViewItem)
#include "test_webview_item.moc"
