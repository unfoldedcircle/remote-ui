// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewitem.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QThread>
#include <QTimer>
#include <QTouchEvent>
#include <QWheelEvent>
#include <qpa/qwindowsysteminterface_p.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

#include "../logging.h"
#include "webviewinput.h"
#include "webviewlibrary.h"
#include "webviewtexture.h"

namespace uc {
namespace webview {

// Library 0.7.0 appended the six request callbacks: 17 callbacks, 136 bytes on aarch64 (and x86_64). The item passes
// this size to uc_webview_new_sized(); an older library copies only its own part (uc_webview.h, "Compatibility").
static_assert(sizeof(uc_webview_callbacks) == 17 * sizeof(void (*)()), "uc_webview_callbacks of library 0.7.0");

namespace {

const WebViewFunctions &lib() {
    return WebViewLibrary::instance()->functions();
}

// Qt's event timestamp: milliseconds from a monotonic source (on eglfs the QPA layer stamps the evdev events
// with the time since the application started, QWindowSystemInterfacePrivate::eventTime). WebKit only needs the
// source to be monotonic and in ms: it measures its event times from the first one it sees
// (WebEventConversion.cpp:578-600); 0 means "now" to the library.
uint32_t eventTime(const QInputEvent *event) {
    return static_cast<uint32_t>(event->timestamp());
}

// Now on the clock of Qt's input events, for the keys the item makes itself (pageUp()/pageDown()). eglfs' evdev and
// libinput handlers pass no timestamp (qevdevkeyboardhandler.cpp:247, qlibinputtouch.cpp:198), so Qt stamps their
// events with QWindowSystemInterfacePrivate::eventTime (qwindowsysteminterface.cpp:516, 766), which QGuiApplication
// starts at its initialisation (qguiapplication.cpp:1647). Never 0: the library's "now" for 0 is CLOCK_MONOTONIC,
// another base than the one WebKit anchored its event times to.
uint32_t eventTimeNow() {
    return static_cast<uint32_t>(std::max<qint64>(1, QWindowSystemInterfacePrivate::eventTime.elapsed()));
}

// UC_WEBVIEW_TRACE=1: every forwarded input event and every input method callback at INFO level, which is what
// the device journal keeps. A trace is made to be shared, so it never carries what the user types or what the
// page's editable element holds, whatever its input purpose: keys are "printable" or a named special key, input
// method events and surrounding text updates appear without text, lengths and caret offsets (docs/webview.md,
// "What the trace logs").
bool &traceSwitch() {
    static bool enabled = qEnvironmentVariableIntValue("UC_WEBVIEW_TRACE") > 0;
    return enabled;
}

bool traceEnabled() {
    return traceSwitch();
}

// the same loop form as Qt's qCInfo(): the stream statement runs once when tracing is on
#define UC_WEBVIEW_TRACE_IF()                                                   \
    for (bool ucTraceOnce = traceEnabled(); ucTraceOnce; ucTraceOnce = false) \
    qCInfo(lcWebView()).nospace() << "trace " << static_cast<const void *>(this) << ": "

const char *touchPhaseName(int phase) {
    switch (phase) {
        case UC_WEBVIEW_TOUCH_DOWN:
            return "down";
        case UC_WEBVIEW_TOUCH_MOVE:
            return "move";
        case UC_WEBVIEW_TOUCH_UP:
            return "up";
        case UC_WEBVIEW_TOUCH_CANCEL:
            return "cancel";
        default:
            return "?";
    }
}

std::function<void(bool)> &inputPanelHook() {
    static std::function<void(bool)> hook;
    return hook;
}

// the fallback's ends of a zoom render wait, next to the UC_WEBVIEW_RENDERED_* values (0..2) of the library
constexpr int kZoomFirstFrame = -1;  // the first frame after the zoom came
constexpr int kZoomNoFrame = -2;     // no frame within zoomFallbackTimeoutMs()

int &zoomFallbackTimeoutMs() {
    static int ms = WebViewItem::kZoomFallbackTimeoutMs;
    return ms;
}

// how a zoom render wait ended, for the trace
QString zoomRenderEndName(int end) {
    switch (end) {
        case UC_WEBVIEW_RENDERED_DONE:
            return QStringLiteral("done");
        case UC_WEBVIEW_RENDERED_TIMEOUT:
            return QStringLiteral("timeout");
        case UC_WEBVIEW_RENDERED_CANCELLED:
            return QStringLiteral("cancelled");
        case kZoomFirstFrame:
            return QStringLiteral("first frame after the zoom");
        case kZoomNoFrame:
            return QStringLiteral("no frame within %1 ms").arg(zoomFallbackTimeoutMs());
        default:
            return QStringLiteral("status %1").arg(end);
    }
}

const char *newWindowResultName(int result) {
    switch (result) {
        case UC_WEBVIEW_NEW_WINDOW_LOADED:
            return "loaded into this view";
        case UC_WEBVIEW_NEW_WINDOW_REFUSED:
            return "refused";
        default:
            return "with an unknown result";
    }
}

const char *loadEventName(int event) {
    switch (event) {
        case UC_WEBVIEW_LOAD_STARTED:
            return "started";
        case UC_WEBVIEW_LOAD_REDIRECTED:
            return "redirected";
        case UC_WEBVIEW_LOAD_COMMITTED:
            return "committed";
        case UC_WEBVIEW_LOAD_FINISHED:
            return "finished";
        case UC_WEBVIEW_LOAD_FAILED:
            return "failed";
        default:
            return "?";
    }
}

// UC_WEBVIEW_ALLOW_FILE_URLS: the library reads it once in uc_webview_global_init(); the process environment does
// not change after that, so reading it here gives the same answer
bool fileUrlsAllowed() {
    return envSwitchEnabled(qgetenv("UC_WEBVIEW_ALLOW_FILE_URLS"));
}

// URLs carry credentials, and tokens in their query and fragment: every log line shows them through
// loggableUrl(), in full (without credentials) only with UC_WEBVIEW_LOG_FULL_URLS=1, which the library reads too
QString logUrl(const QUrl &url) {
    return loggableUrl(url, envSwitchEnabled(qgetenv("UC_WEBVIEW_LOG_FULL_URLS")));
}

// The zoom levels of zoomIn()/zoomOut() in percent, a browser's steps from 50 to 300 % (docs/webview.md, "Keys of the
// remote on the developer page"). The only copy of the table.
constexpr int kZoomStepsPercent[] = {50, 67, 75, 80, 90, 100, 110, 125, 150, 175, 200, 250, 300};

// uc_webview_set_zoom() ignores a zoom outside 0 < zoom <= 20 with a warning (uc_webview.c); the item refuses it
// itself, so that `zoom` never shows a value WebKit does not use
constexpr qreal kMaxZoom = 20.0;

// The zoom level after zoom in the direction (+1 in, -1 out): the first level beyond it, or zoom itself when there is
// none. A zoom within kZoomStepTolerance of a level counts as that level.
qreal nextZoomStep(qreal zoom, int direction) {
    constexpr qreal kZoomStepTolerance = 0.001;
    constexpr int   kCount = static_cast<int>(std::size(kZoomStepsPercent));
    for (int i = 0; i < kCount; ++i) {
        const qreal step = kZoomStepsPercent[direction > 0 ? i : kCount - 1 - i] / 100.0;
        if (direction > 0 ? step > zoom + kZoomStepTolerance : step < zoom - kZoomStepTolerance) {
            return step;
        }
    }
    return zoom;
}

// XKB keycodes (evdev + 8) of Page_Up and Page_Down for pageUp()/pageDown(): WebKit makes KeyboardEvent.code
// "PageUp"/"PageDown" of them (WebKeyboardEventWPE.cpp:641-652), as for the keys of a real keyboard
constexpr uint32_t kKeycodePageUp = 112;    // KEY_PAGEUP 104 + 8
constexpr uint32_t kKeycodePageDown = 117;  // KEY_PAGEDOWN 109 + 8

// Marks the library calls during which WebKit reports the page's focused element synchronously because the view's
// focus changes (uc_webview.h threading contract, WPEWebViewPlatform.cpp:100-110): set_focused, and a touch or
// pointer press with which WebKit focuses its view itself (WPEWebViewPlatform.cpp:368-370, 468). An ime_focus
// callback inside such a call repeats the state WebKit stored when the element got its focus.
class FocusReplayScope {
 public:
    explicit FocusReplayScope(int *depth) : m_depth(depth) { ++*m_depth; }
    ~FocusReplayScope() { --*m_depth; }
    FocusReplayScope(const FocusReplayScope &) = delete;
    FocusReplayScope &operator=(const FocusReplayScope &) = delete;

 private:
    int *m_depth;
};

}  // namespace

WebViewItem::WebViewItem(QQuickItem *parent)
    : QQuickItem(parent),
      m_available(WebViewLibrary::instance()->isAvailable()),
      m_pinchZoomAvailable(m_available && WebViewLibrary::instance()->pinchZoomAvailable()),
      m_requestsAvailable(m_available && WebViewLibrary::instance()->requestsAvailable()),
      m_requests(this) {
    setFlag(ItemHasContents, true);
    setFlag(ItemAcceptsInputMethod, true);
    setFlag(ItemIsFocusScope, true);
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    m_tapInputPanelTimer.setSingleShot(true);
    m_tapInputPanelTimer.setInterval(kTapInputPanelDelayMs);
    connect(&m_tapInputPanelTimer, &QTimer::timeout, this, &WebViewItem::onTapInputPanelTimeout);

    m_zoomFallbackTimer.setSingleShot(true);
    connect(&m_zoomFallbackTimer, &QTimer::timeout, this, [this] { finishZoomRenderWait(kZoomNoFrame); });

    // kind, id and outcome of every request: never its texts, the answer or the credentials
    m_requests.setTrace([this](const QString &line) { UC_WEBVIEW_TRACE_IF() << qUtf8Printable(line); });
}

WebViewItem::~WebViewItem() {
    destroyView();
}

void WebViewItem::registerQmlTypes() {
    qmlRegisterType<WebViewItem>("UC.WebView", 1, 0, "WebView");
    // the requests of dialogRequested() and authenticationRequested(): typed QML properties and their enums
    qmlRegisterUncreatableType<WebViewDialogRequest>("UC.WebView", 1, 0, "WebViewDialogRequest",
                                                     QStringLiteral("created by the WebView item"));
    qmlRegisterUncreatableType<WebViewAuthRequest>("UC.WebView", 1, 0, "WebViewAuthRequest",
                                                   QStringLiteral("created by the WebView item"));
}

void WebViewItem::setInputPanelHookForTest(std::function<void(bool)> hook) {
    inputPanelHook() = std::move(hook);
}

void WebViewItem::setTraceEnabledForTest(bool enabled) {
    traceSwitch() = enabled;
}

void WebViewItem::setZoomFallbackTimeoutForTest(int ms) {
    zoomFallbackTimeoutMs() = ms > 0 ? ms : kZoomFallbackTimeoutMs;
}

// ---- callbacks (GUI thread, from the GLib main context) -------------------------------------------------------

const uc_webview_callbacks &WebViewItem::callbacks() {
    // the API version 1 callbacks; ensureView() adds the pinch callbacks when the library has pinch zoom
    static const uc_webview_callbacks cb = [] {
        uc_webview_callbacks c{};
        c.frame_ready = &WebViewItem::onFrameReadyCb;
        c.load_changed = &WebViewItem::onLoadChangedCb;
        c.title_changed = &WebViewItem::onTitleChangedCb;
        c.progress_changed = &WebViewItem::onProgressChangedCb;
        c.web_process_terminated = &WebViewItem::onWebProcessTerminatedCb;
        c.ime_focus = &WebViewItem::onImeFocusCb;
        c.ime_cursor_area = &WebViewItem::onImeCursorAreaCb;
        c.ime_surrounding = &WebViewItem::onImeSurroundingCb;
        return c;
    }();
    return cb;
}

void WebViewItem::onFrameReadyCb(uc_webview *, const uc_webview_frame *frame, void *user) {
    static_cast<WebViewItem *>(user)->onFrameReady(*frame);
}

void WebViewItem::onLoadChangedCb(uc_webview *, int event, const char *uri, void *user) {
    static_cast<WebViewItem *>(user)->onLoadChanged(event, uri);
}

void WebViewItem::onTitleChangedCb(uc_webview *, const char *title, void *user) {
    auto         *item = static_cast<WebViewItem *>(user);
    const QString newTitle = QString::fromUtf8(title ? title : "");
    if (item->m_title != newTitle) {
        item->m_title = newTitle;
        emit item->titleChanged();
    }
    // a single-page application names its new route without a load (see onFrameReady)
    item->updateNavigationHistory();
}

void WebViewItem::onProgressChangedCb(uc_webview *, double progress, void *user) {
    auto     *item = static_cast<WebViewItem *>(user);
    const int percent = qBound(0, qRound(progress * 100.0), 100);
    if (item->m_loadProgress != percent) {
        item->m_loadProgress = percent;
        emit item->loadProgressChanged();
    }
}

void WebViewItem::onWebProcessTerminatedCb(uc_webview *, int reason, void *user) {
    static_cast<WebViewItem *>(user)->onWebProcessTerminated(reason);
}

void WebViewItem::onImeFocusCb(uc_webview *, int focused, unsigned hints, unsigned purpose, const char *surrounding,
                               int cursor, void *user) {
    static_cast<WebViewItem *>(user)->onImeFocus(focused != 0, hints, purpose, surrounding, cursor);
}

void WebViewItem::onImeCursorAreaCb(uc_webview *, int x, int y, int w, int h, void *user) {
    auto       *item = static_cast<WebViewItem *>(user);
    const qreal ratio = item->m_devicePixelRatio;
    item->m_cursorRectangle = QRectF(x / ratio, y / ratio, w / ratio, h / ratio);
    if (traceEnabled()) {
        qCInfo(lcWebView()).nospace() << "trace " << static_cast<const void *>(item) << ": ime_cursor_area " << x
                                      << "," << y << " " << w << "x" << h << " (view px)";
    }
    item->updateInputMethod(Qt::ImCursorRectangle);
}

void WebViewItem::onImeSurroundingCb(uc_webview *, const char *text, int cursor, int anchor, void *user) {
    static_cast<WebViewItem *>(user)->onImeSurrounding(text, cursor, anchor);
}

void WebViewItem::onZoomRenderedCb(uc_webview *, int status, void *user) {
    static_cast<WebViewItem *>(user)->finishZoomRenderWait(status);
}

void WebViewItem::onPinchCb(uc_webview *, int phase, double scale, double x, double y, void *user) {
    static_cast<WebViewItem *>(user)->onPinch(phase, scale, QPointF(x, y));
}

void WebViewItem::onMagnificationChangedCb(uc_webview *, double magnification, void *user) {
    static_cast<WebViewItem *>(user)->onMagnificationChanged(magnification);
}

void WebViewItem::onMagnificationRenderedCb(uc_webview *, int status, void *user) {
    static_cast<WebViewItem *>(user)->finishMagnificationRender(status);
}

void WebViewItem::onScriptDialogCb(uc_webview *, uint64_t request, int type, const char *origin, const char *message,
                                   const char *defaultText, void *user) {
    static_cast<WebViewItem *>(user)->onScriptDialog(request, type, origin, message, defaultText);
}

void WebViewItem::onAuthenticateCb(uc_webview *, uint64_t request, const char *origin, const char *realm, int scheme,
                                   unsigned flags, void *user) {
    static_cast<WebViewItem *>(user)->onAuthenticate(request, origin, realm, scheme, flags);
}

void WebViewItem::onRequestCancelledCb(uc_webview *, uint64_t request, void *user) {
    // a navigation call of the item (from an idle after it), a commit, a crash or WebKit ended it: close its dialog
    static_cast<WebViewItem *>(user)->m_requests.cancel(request);
}

void WebViewItem::onCloseRequestedCb(uc_webview *, void *user) {
    static_cast<WebViewItem *>(user)->onCloseRequested();
}

void WebViewItem::onNewWindowCb(uc_webview *, const char *uri, int result, void *user) {
    static_cast<WebViewItem *>(user)->onNewWindow(uri, result);
}

void WebViewItem::onResponseRefusedCb(uc_webview *, const char *uri, const char *mimeType, int reason, void *user) {
    static_cast<WebViewItem *>(user)->onResponseRefused(uri, mimeType, reason);
}

void WebViewItem::onFrameReady(const uc_webview_frame &frame) {
    Q_ASSERT(QThread::currentThread() == thread());
    ++m_framesReceived;
    if (void *replaced = m_frames.push(frame)) {
        // the scene graph never saw the replaced frame (item not rendered in between)
        releaseFrame(replaced);
    }
    if (m_frames.isHeld()) {
        // the scaled frame stays on screen while the page renders the magnification: report this one at once, or
        // WebKit would wait with the next frame, and the page's rendering update that ends the hold with it
        // (FrameQueue::takePendingForRendered). The library completes the report after this callback.
        if (void *handle = m_frames.takePendingForRendered()) {
            lib().frame_rendered(m_view, handle);
        }
    }
    emit frameStatsChanged();
    update();
    if (m_zoomWaitsForFrame) {
        // the fallback's end of a zoom render wait, after this callback: the next zoom level would go to WebKit from
        // inside its frame delivery
        m_zoomWaitsForFrame = false;
        const quint64 generation = m_zoomWaitGeneration;
        QTimer::singleShot(0, this, [this, generation] {
            if (generation == m_zoomWaitGeneration) {
                finishZoomRenderWait(kZoomFirstFrame);
            }
        });
    }
    // A same-document navigation (history.pushState of a single-page application, a fragment link, going back or
    // forward between such entries) changes the back/forward list without any load event: WebKit's glib navigation
    // client has no same-document hook and WPE's PageClientImpl::didSameDocumentNavigationForMainFrame is empty
    // (PageClientImpl.cpp:414-416); only WebKitBackForwardList::changed reports it, which the library does not pass
    // on. Such a navigation shows new content, so the history is read again with every frame (and title change):
    // can_go_back/forward only look at the UI process' list (WebKitWebView.cpp:3898-3934), no IPC.
    updateNavigationHistory();
}

void WebViewItem::onLoadChanged(int event, const char *uri) {
    const QUrl url = QUrl::fromEncoded(uri ? QByteArray(uri) : QByteArray());
    UC_WEBVIEW_TRACE_IF() << "load " << loadEventName(event) << " " << logUrl(url);
    // the page went on: a login whose user name waited for a retry is over (WebViewRequests, "Retry")
    m_requests.forgetUser();
    switch (event) {
        case UC_WEBVIEW_LOAD_STARTED:
            m_loadFailed = false;
            // a new document: what the item learnt about the old one's focused element is void (the page may
            // focus another element while the view has no focus, which WebKit reports to nobody)
            m_imeInhibitHonouredOnTap = false;
            setLoading(true);
            setUrlFromPage(url);
            break;
        case UC_WEBVIEW_LOAD_REDIRECTED:
        case UC_WEBVIEW_LOAD_COMMITTED:
            setUrlFromPage(url);
            break;
        case UC_WEBVIEW_LOAD_FINISHED:
            if (!m_loadFailed) {
                // a complete load after a crash earns the next crash another automatic reload
                m_reloadedAfterCrash = false;
            }
            setLoading(false);
            break;
        case UC_WEBVIEW_LOAD_FAILED:
            m_loadFailed = true;
            qCWarning(lcWebView()) << "Load failed:" << logUrl(url);
            emit loadFailed(url);
            break;
        default:
            break;
    }
    updateNavigationHistory();
}

void WebViewItem::onWebProcessTerminated(int reason) {
    qCWarning(lcWebView()) << "Web process terminated, reason" << reason << "while showing" << logUrl(m_url);
    setLoading(false);
    emit crashed(reason);

    if (m_reloadedAfterCrash) {
        qCWarning(lcWebView()) << "Not reloading again after a second crash";
        return;
    }
    m_reloadedAfterCrash = true;
    // not from inside the callback: WebKit is still cleaning up the dead process
    QTimer::singleShot(0, this, [this] {
        if (m_view) {
            qCInfo(lcWebView()) << "Reloading after the web process crash";
            lib().reload(m_view);
        }
    });
}

void WebViewItem::onImeFocus(bool focused, unsigned hints, unsigned purpose, const char *surrounding, int cursor) {
    // WebKit reports the focus of the page's editable element while the view is focused. The item's own focus loss
    // (itemChange -> uc_webview_set_focused(0) -> WPEWebViewPlatform.cpp:100-110 notifyFocusedOut) produces a
    // focus-out callback too, so the global input method may only be touched while this item owns the focus:
    // otherwise the keyboard of the item that took the focus (a TextField next to the view) would be closed, and
    // taking the focus back from here would start a show/hide loop.
    const bool owner = hasActiveFocus();
    // a report inside a library call that changed the view's focus repeats the element's stored state; any other
    // report comes from the web process and is a change of the page's focus (FocusReplayScope)
    const bool replay = m_focusReplayDepth > 0;
    const bool inhibit = focused && (hints & UC_WEBVIEW_IME_HINT_INHIBIT_OSK) != 0;
    if (!replay) {
        // WebKit sets INHIBIT_OSK for inputmode="none" and for an element focused without a user gesture, one bit
        // for both (InputMethodState.cpp:38-39, WebPageGLib.cpp:212-217). A report that arrives within the tap
        // window after the user's tap or key press follows a user gesture, which WebKit does not flag
        // (m_userIsInteracting): then the bit means inputmode="none" and holds for taps too. Otherwise (the page
        // focused the element itself: autofocus, a script) a tap on the element shows the keyboard.
        const bool afterGesture = m_lastUserGesture.isValid() && m_lastUserGesture.elapsed() <= kTapInputPanelDelayMs;
        m_imeInhibitHonouredOnTap = inhibit && afterGesture;
    }
    // the element's text and caret (surrounding, cursor) are never traced: in a password field the caret offset is
    // the number of characters typed
    UC_WEBVIEW_TRACE_IF() << "ime_focus " << focused << " hints 0x" << Qt::hex << hints << Qt::dec << " purpose "
                          << purpose << (replay ? " replayed with the view's focus" : "")
                          << (inhibit ? (m_imeInhibitHonouredOnTap ? " (keyboard inhibited, also on taps)"
                                                                   : " (keyboard inhibited until a tap)")
                                      : "")
                          << (owner ? " (focus owner)" : " (another item has the focus)");

    if (!focused) {
        if (!m_imeEnabled) {
            return;
        }
        // also cancels a tap's pending keyboard request: the tap moved the page's focus away from the element
        resetImeState();
        if (!owner) {
            return;
        }
        updateInputMethod(Qt::ImEnabled | Qt::ImHints | Qt::ImQueryInput);
        requestInputPanel(false);
        return;
    }

    const bool wasEnabled = m_imeEnabled;
    m_imeEnabled = true;
    m_imeHints = inputMethodHintsForIme(hints, purpose);
    m_imeInhibitOsk = inhibit;
    if (surrounding) {
        m_surroundingText = QString::fromUtf8(surrounding);
        m_cursorPosition = cursor;
        m_anchorPosition = cursor;
    }
    if (!owner) {
        // the state is answered when the item gets the focus back; the focus itself moves only on a user press
        return;
    }
    updateInputMethod(Qt::ImEnabled | Qt::ImHints | Qt::ImQueryInput);

    if (m_imeInhibitOsk) {
        // inputmode="none", or an element the page focused without a user gesture (WebPageGLib.cpp:216-217): no
        // keyboard for the focus itself. A tap on the element brings no new report (FocusController.cpp:1175-1178:
        // elementDidRefocus keeps the stored state), so onTap() decides about the keyboard then.
        qCDebug(lcWebView()) << "Page inhibits the on-screen keyboard"
                             << (m_imeInhibitHonouredOnTap ? "(inputmode none)" : "until a tap on the element");
        return;
    }
    if (!wasEnabled || !QGuiApplication::inputMethod()->isVisible()) {
        requestInputPanel(true);
    }
}

void WebViewItem::resetImeState() {
    // m_imeInhibitHonouredOnTap stays: it describes the page's focused element, which keeps its focus when only the
    // view loses it, and the replayed report on the way back cannot tell (onImeFocus)
    m_imeEnabled = false;
    m_imeInhibitOsk = false;
    m_tapInputPanelTimer.stop();
    m_preeditActive = false;
    m_surroundingText.clear();
    m_cursorPosition = -1;
    m_anchorPosition = -1;
}

void WebViewItem::requestInputPanel(bool show) {
    UC_WEBVIEW_TRACE_IF() << (show ? "input panel show" : "input panel hide");
    if (inputPanelHook()) {
        inputPanelHook()(show);
        return;
    }
    if (show) {
        QGuiApplication::inputMethod()->show();
    } else {
        QGuiApplication::inputMethod()->hide();
    }
}

void WebViewItem::onTap() {
    // WebKit reports the focus of the page's editable element only when it changes: the web process sends the
    // input method state again only when it differs (WebPageGLib.cpp:212-223; a click into the element that
    // already has the focus only refocuses it, FocusController.cpp:1175-1178) and InputMethodFilter::setState
    // notifies focus-in only when an element gains it (InputMethodFilter.cpp:91-102). After the keyboard's hide key
    // a tap on the same field therefore produces no ime_focus callback. Qt's TextInput re-opens the input panel on a
    // press into the field that already has the focus (qquicktextinput.cpp:1901-1913); this does the same for the
    // page's field.
    // Which element the tap hits is only known to WebKit, so the request waits until WebKit had the time to turn
    // the tap into a click: a tap on a button or the page background moves the page's focus away, ime_focus(0)
    // arrives first and resetImeState() cancels the request, no keyboard flashes up.
    // A page-inhibited keyboard (INHIBIT_OSK) holds for the tap only when it is the page's inputmode="none": an
    // element the page focused by itself (an autofocused PIN field) is inhibited as well, and a tap on it is the
    // user asking for the keyboard (onImeFocus has the distinction).
    m_lastUserGesture.restart();
    if (!m_imeEnabled || !hasActiveFocus() || keyboardInhibitedOnTap()) {
        return;
    }
    UC_WEBVIEW_TRACE_IF() << "tap with a focused editable element: keyboard request in " << kTapInputPanelDelayMs
                          << " ms unless the page moves its focus";
    m_tapInputPanelTimer.start();
}

bool WebViewItem::keyboardInhibitedOnTap() const {
    return m_imeInhibitOsk && m_imeInhibitHonouredOnTap;
}

void WebViewItem::onTapInputPanelTimeout() {
    // the tap may have focused another element by now: its report decides (a user-tapped inputmode="none" element
    // is honoured, see onImeFocus)
    if (!m_imeEnabled || !hasActiveFocus() || keyboardInhibitedOnTap()) {
        return;
    }
    // The keyboard's hide key hides the input method itself: HideKeyboardKey.qml's own handler runs
    // InputContext.priv.hideInputPanel() (PlatformInputContext::hideInputPanel) before the layout's
    // `root.keyboard.hide()` (QML runs both handlers). show() makes it visible again, and main.qml's keyboard
    // controller opens its panel on that change (Qt.inputMethod.visibleChanged). A visible input method needs
    // nothing: the keyboard is up.
    if (!QGuiApplication::inputMethod()->isVisible()) {
        requestInputPanel(true);
    }
}

void WebViewItem::onImeSurrounding(const char *text, int cursor, int anchor) {
    // text, length, caret and anchor offsets are never traced (in a password field they count the typed characters)
    UC_WEBVIEW_TRACE_IF() << "ime_surrounding changed";
    m_surroundingText = QString::fromUtf8(text ? text : "");
    m_cursorPosition = cursor;
    m_anchorPosition = anchor;
    updateInputMethod(Qt::ImSurroundingText | Qt::ImCursorPosition | Qt::ImAnchorPosition);
}

// ---- requests of the page (library 0.7.0) ------------------------------------------------------------------------

void WebViewItem::onScriptDialog(quint64 id, int type, const char *origin, const char *message,
                                 const char *defaultText) {
    WebViewDialogRequest *request = m_requests.addDialog(id, type, origin, message, defaultText);
    if (!request) {
        return;  // a type of a newer library, answered "no"
    }
    // the item deletes it after it ended: QML must not collect it
    QQmlEngine::setObjectOwnership(request, QQmlEngine::CppOwnership);
    QPointer<WebViewDialogRequest> guard(request);
    emit dialogRequested(request);
    // answering from inside the library's callback is allowed: the library closes the dialog from an idle
    m_requests.answerIfUnhandled(guard.data());
}

void WebViewItem::onAuthenticate(quint64 id, const char *origin, const char *realm, int scheme, unsigned flags) {
    WebViewAuthRequest *request = m_requests.addAuth(id, origin, realm, scheme, flags);
    if (!request) {
        return;  // not a user name and password login, continued without credentials
    }
    QQmlEngine::setObjectOwnership(request, QQmlEngine::CppOwnership);
    QPointer<WebViewAuthRequest> guard(request);
    emit authenticationRequested(request);
    m_requests.answerIfUnhandled(guard.data());
}

void WebViewItem::onCloseRequested() {
    UC_WEBVIEW_TRACE_IF() << "close requested by the page (window.close())";
    // not from inside the callback: a handler that leaves the page frees the view, which uc_webview_free() forbids
    // there (uc_webview.h, "Host callbacks")
    QTimer::singleShot(0, this, [this] {
        if (m_view) {
            emit closeRequested();
        }
    });
}

void WebViewItem::onNewWindow(const char *uri, int result) {
    const QUrl url = QUrl::fromEncoded(uri ? QByteArray(uri) : QByteArray());
    UC_WEBVIEW_TRACE_IF() << "new window " << newWindowResultName(result) << ": " << logUrl(url);
    if (result == UC_WEBVIEW_NEW_WINDOW_LOADED || result == UC_WEBVIEW_NEW_WINDOW_REFUSED) {
        emit newWindow(url, result == UC_WEBVIEW_NEW_WINDOW_LOADED);
    }
}

void WebViewItem::onResponseRefused(const char *uri, const char *mimeType, int reason) {
    const QUrl    url = QUrl::fromEncoded(uri ? QByteArray(uri) : QByteArray());
    const QString type = QString::fromUtf8(mimeType ? mimeType : "");
    const bool    download = reason == UC_WEBVIEW_RESPONSE_DOWNLOAD;
    // a reason of a newer library counts as "cannot be shown"
    UC_WEBVIEW_TRACE_IF() << "response refused, " << (download ? "download " : "cannot be shown ") << logUrl(url)
                          << " (" << (type.isEmpty() ? "type unknown" : qUtf8Printable(type)) << ")";
    emit responseRefused(url, type, download);
}

// ---- frames ------------------------------------------------------------------------------------------------------

QSGNode *WebViewItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    // render thread, GUI thread blocked: the only place that touches the frame queue from this thread
    auto      *node = static_cast<WebViewNode *>(oldNode);
    const bool swapped = m_frames.swap();

    if (!m_frames.hasCurrent()) {
        return node;  // nothing to show yet, or the frames were released: keep what is there
    }

    const bool created = node == nullptr;
    if (created) {
        node = new WebViewNode();
    }
    if (swapped || created || !node->texture()) {
        if (!node->setFrame(window(), m_frames.current())) {
            if (!m_renderWarned) {
                m_renderWarned = true;
                qCWarning(lcWebView()) << "Cannot show web view frames in this scene graph";
            }
            // updateDirtyNode() stores the returned pointer without deleting the old one (qquickwindow.cpp:3901)
            delete node;
            return nullptr;
        }
    }
    if (swapped && m_pinch.waitsForFrame()) {
        // the first frame after a magnification's render: sharp at the new scale, the picture is relative to it now
        // (the magnification the user sees does not change, PinchZoom::rebase)
        m_pinch.rebase();
    }
    // the pinch's picture: the frame scaled around where the pinch began, cut to the item (webviewpinch.h)
    const uc_webview_frame &frame = m_frames.current();
    const NodeGeometry geometry =
        nodeGeometry(m_pinch.transform(), size(), m_devicePixelRatio, QSize(frame.width, frame.height));
    node->setRect(geometry.rect);
    node->setSourceRect(geometry.sourceRect);
    return node;
}

void WebViewItem::onFrameSwapped() {
    // GUI thread, queued from the render thread's frameSwapped: the frame bound in the last updatePaintNode is on
    // screen, the frames it replaced are no longer sampled
    if (!m_view) {
        return;
    }
    const FrameQueue::Report report = m_frames.takeReport();
    if (report.rendered) {
        lib().frame_rendered(m_view, report.rendered);
    }
    for (void *handle : report.released) {
        releaseFrame(handle);
    }
}

void WebViewItem::releaseFrame(void *handle) {
    lib().frame_release(m_view, handle);
    ++m_framesReleased;
    emit frameStatsChanged();
}

void WebViewItem::releaseFrames() {
    if (!m_view) {
        return;
    }
    const QVector<void *> handles = m_frames.drain();
    for (void *handle : handles) {
        releaseFrame(handle);
    }
}

void WebViewItem::releaseResources() {
    // GUI thread, before the scene graph is invalidated or the item changes its window (qquickitem.cpp:3881): the
    // node and its GL texture go with the scene graph, the frames go back to the web process now
    releaseFrames();
    QQuickItem::releaseResources();
}

// ---- lifecycle ---------------------------------------------------------------------------------------------------

void WebViewItem::itemChange(ItemChange change, const ItemChangeData &value) {
    QQuickItem::itemChange(change, value);
    switch (change) {
        case ItemSceneChange:
            attachWindow(value.window);
            break;
        case ItemVisibleHasChanged:
            updateMapped();
            break;
        case ItemDevicePixelRatioHasChanged:
            updateSize();
            break;
        case ItemActiveFocusHasChanged:
            UC_WEBVIEW_TRACE_IF() << "active focus " << value.boolValue;
            if (!value.boolValue && m_imeEnabled) {
                // another item owns the input method session now (or nobody does): forget the page's editable
                // element without touching the keyboard; WebKit reports the focus-out right after set_focused(0)
                resetImeState();
            }
            if (m_view) {
                FocusReplayScope replay(&m_focusReplayDepth);
                lib().set_focused(m_view, value.boolValue ? 1 : 0);
            }
            break;
        default:
            break;
    }
}

void WebViewItem::attachWindow(QQuickWindow *window) {
    QObject::disconnect(m_frameSwappedConnection);
    QObject::disconnect(m_windowVisibleConnection);
    QObject::disconnect(m_screenChangedConnection);
    watchScreen(nullptr);
    releaseFrames();

    if (window) {
        // frameSwapped is emitted on the render thread: the queued connection hands it to the GUI thread
        m_frameSwappedConnection =
            connect(window, &QQuickWindow::frameSwapped, this, &WebViewItem::onFrameSwapped, Qt::QueuedConnection);
        // the display-off path hides the window (src/system/power.h): unmap so the web process stops rendering
        m_windowVisibleConnection = connect(window, &QWindow::visibleChanged, this, [this] { updateMapped(); });
        if (m_available) {
            m_screenChangedConnection = connect(window, &QWindow::screenChanged, this, [this](QScreen *screen) {
                watchScreen(screen);
                updateRefreshRate();
            });
            watchScreen(window->screen());
            updateRefreshRate();
        }
    }
    updateSize();
    updateMapped();
}

void WebViewItem::watchScreen(QScreen *screen) {
    QObject::disconnect(m_refreshRateConnection);
    if (screen) {
        m_refreshRateConnection = connect(screen, &QScreen::refreshRateChanged, this, [this] { updateRefreshRate(); });
    }
}

void WebViewItem::updateRefreshRate() {
    // a no-op until the library is initialised (ensureView() calls this again right before the first view); the
    // library is process-wide, so the last item to report wins, which is fine with the remote's single screen
    QQuickWindow *win = window();
    QScreen      *screen = win ? win->screen() : nullptr;
    if (screen) {
        WebViewLibrary::instance()->setDisplayRefreshRate(screen->refreshRate());
    }
}

void WebViewItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        updateSize();
    }
}

QSize WebViewItem::devicePixelSize() const {
    return QSize(qMax(1, qRound(width() * m_devicePixelRatio)), qMax(1, qRound(height() * m_devicePixelRatio)));
}

QPointF WebViewItem::devicePoint(const QPointF &itemPoint) const {
    return itemPoint * m_devicePixelRatio;
}

void WebViewItem::ensureView() {
    if (m_view || !m_available || !window() || width() < 1 || height() < 1) {
        return;
    }
    WebViewLibrary *library = WebViewLibrary::instance();
    if (!library->ensureInitialized()) {
        return;
    }

    // before the view and its first load: WebKit takes its nominal frame rate when the page's display link is set up
    updateRefreshRate();

    m_devicePixelRatio = window()->effectiveDevicePixelRatio();
    m_viewSize = devicePixelSize();
    // uc_webview_new_sized() (library 0.4.0) passes this build's whole callback struct; the pinch callbacks switch the
    // library's pinch recognition on (0.6.0), so they are set only when the item can follow up with a magnification.
    // An older library reads the API version 1 callbacks of either call.
    uc_webview_callbacks cb = callbacks();
    if (m_pinchZoomAvailable) {
        cb.pinch = &WebViewItem::onPinchCb;
        cb.magnification_changed = &WebViewItem::onMagnificationChangedCb;
    }
    // library 0.7.0: the page's requests reach the item only with these set; an older library never reads them
    if (m_requestsAvailable) {
        cb.script_dialog = &WebViewItem::onScriptDialogCb;
        cb.authenticate = &WebViewItem::onAuthenticateCb;
        cb.request_cancelled = &WebViewItem::onRequestCancelledCb;
        cb.close_requested = &WebViewItem::onCloseRequestedCb;
        cb.new_window = &WebViewItem::onNewWindowCb;
        cb.response_refused = &WebViewItem::onResponseRefusedCb;
    }
    if (lib().create_sized) {
        m_view = lib().create_sized(m_viewSize.width(), m_viewSize.height(), &cb, sizeof cb, this);
    } else {
        m_view = lib().create(m_viewSize.width(), m_viewSize.height(), &cb, this);
    }
    if (!m_view) {
        qCWarning(lcWebView()) << (lib().create_sized ? "uc_webview_new_sized failed" : "uc_webview_new failed");
        return;
    }
    m_pinch.reset(1.0);
    m_requests.setView(m_view, lib().request_reply, lib().request_auth);
    qCDebug(lcWebView()) << "Web view created" << m_viewSize << "at scale" << m_devicePixelRatio;

    lib().set_scale(m_view, m_devicePixelRatio);
    lib().set_mapped(m_view, m_mapped ? 1 : 0);
    {
        FocusReplayScope replay(&m_focusReplayDepth);
        lib().set_focused(m_view, hasActiveFocus() ? 1 : 0);
    }
    applyBackgroundColor();
    // every new view starts at WebKit's zoom of 1 (WebPageProxy.h:3971): a zoom set before it existed, before its
    // first load (nothing is rendered yet, so nothing to wait for)
    m_zoomApplied = 1.0;
    if (!qFuzzyCompare(m_zoom, 1.0)) {
        UC_WEBVIEW_TRACE_IF() << "zoom " << m_zoom << " (" << qRound(m_zoom * 100) << " %)";
        lib().set_zoom(m_view, m_zoom);
        m_zoomApplied = m_zoom;
    }

    if (!m_pendingHtml.isEmpty()) {
        const QByteArray base = m_pendingBaseUrl.toEncoded();
        lib().load_html(m_view, m_pendingHtml.toUtf8().constData(), base.isEmpty() ? nullptr : base.constData());
    } else if (!m_url.isEmpty()) {
        lib().load_uri(m_view, m_url.toEncoded().constData());
    }
}

void WebViewItem::destroyView() {
    m_tapInputPanelTimer.stop();
    m_imeInhibitHonouredOnTap = false;
    m_tapTouchId = -1;
    m_mouseTapCandidate = false;
    if (m_imeEnabled) {
        const bool owner = hasActiveFocus();
        resetImeState();
        if (owner) {
            requestInputPanel(false);
        }
    }
    m_activeTouches.clear();
    m_touchStarts.clear();
    m_touchSlots.clear();
    m_pressedButtons = Qt::NoButton;
    // uc_webview_free() ends a pending uc_webview_when_rendered() request with CANCELLED inside the call: the wait is
    // over before, so that callback finds nothing (no signal from the destructor)
    m_zoomWaiting = false;
    m_zoomWaitsForFrame = false;
    ++m_zoomWaitGeneration;
    m_zoomFallbackTimer.stop();
    m_zoomRendering = false;
    // the same for the magnification's render request, and a new view starts at WebKit's magnification of 1
    m_magnificationRequests = 0;
    m_magnificationRendering = false;
    m_pinch.reset(1.0);
    m_frames.setHeld(false);
    // uc_webview_free() ends the pending requests without a callback: their objects go now, without a signal (QML
    // properties that hold them turn null)
    m_requests.clear();
    m_requests.setView(nullptr, nullptr, nullptr);
    if (!m_view) {
        return;
    }
    releaseFrames();
    lib().destroy(m_view);
    m_view = nullptr;
    qCDebug(lcWebView()) << "Web view destroyed, frames received" << m_framesReceived << "released" << m_framesReleased;
}

void WebViewItem::updateMapped() {
    const bool mapped = isVisible() && window() && window()->isVisible();
    if (mapped == m_mapped) {
        return;
    }
    m_mapped = mapped;
    if (m_view) {
        lib().set_mapped(m_view, mapped ? 1 : 0);
    }
}

void WebViewItem::updateSize() {
    if (!m_view) {
        ensureView();
        return;
    }
    const qreal ratio = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    if (!qFuzzyCompare(ratio, m_devicePixelRatio)) {
        m_devicePixelRatio = ratio;
        lib().set_scale(m_view, ratio);
    }
    const QSize size = devicePixelSize();
    if (size != m_viewSize) {
        m_viewSize = size;
        lib().resize(m_view, size.width(), size.height());
    }
}

// ---- properties and navigation -----------------------------------------------------------------------------------

bool WebViewItem::loadAllowed(const QUrl &url, const char *what) {
    // the library's scheme allowlist (uc_uri_policy.c) would refuse the load and report it as failed only later,
    // from the main loop; refusing it here keeps the item's url and pending state untouched and answers at once
    const QString reason = urlRejectionReason(url, fileUrlsAllowed());
    if (reason.isEmpty()) {
        return true;
    }
    qCWarning(lcWebView()).noquote().nospace() << "Not loading " << what << " " << logUrl(url) << ": " << reason;
    emit loadFailed(url);
    return false;
}

void WebViewItem::setUrl(const QUrl &url) {
    if (!url.isEmpty() && !loadAllowed(url, "URL")) {
        return;
    }
    m_pendingHtml.clear();
    if (url != m_url) {
        m_url = url;
        emit urlChanged();
    }
    if (m_view && !url.isEmpty()) {
        // the user goes elsewhere: a pending leave-page question is answered "leave" (see the class comment)
        m_requests.answerLeavePage(true);
        UC_WEBVIEW_TRACE_IF() << "load_uri " << logUrl(url);
        lib().load_uri(m_view, url.toEncoded().constData());
    }
}

void WebViewItem::setUrlFromPage(const QUrl &url) {
    if (url.isEmpty() || url == m_url) {
        return;
    }
    m_url = url;
    emit urlChanged();
}

void WebViewItem::setLoading(bool loading) {
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

void WebViewItem::updateNavigationHistory() {
    if (!m_view) {
        return;
    }
    const bool back = lib().can_go_back(m_view) != 0;
    const bool forward = lib().can_go_forward(m_view) != 0;
    if (back != m_canGoBack || forward != m_canGoForward) {
        m_canGoBack = back;
        m_canGoForward = forward;
        emit navigationHistoryChanged();
    }
}

void WebViewItem::setBackgroundColor(const QColor &color) {
    if (color == m_backgroundColor) {
        return;
    }
    m_backgroundColor = color;
    emit backgroundColorChanged();
    applyBackgroundColor();
}

void WebViewItem::applyBackgroundColor() {
    if (m_view) {
        lib().set_background_rgba(m_view, static_cast<float>(m_backgroundColor.redF()),
                                  static_cast<float>(m_backgroundColor.greenF()),
                                  static_cast<float>(m_backgroundColor.blueF()),
                                  static_cast<float>(m_backgroundColor.alphaF()));
    }
}

void WebViewItem::setZoom(qreal zoom) {
    if (!(zoom > 0.0) || zoom > kMaxZoom) {
        qCWarning(lcWebView()) << "Zoom" << zoom << "refused: the web view takes 0 < zoom <= 20";
        return;
    }
    if (qFuzzyCompare(zoom, m_zoom)) {
        return;
    }
    m_zoom = zoom;
    applyZoom();
    emit zoomChanged();
}

void WebViewItem::applyZoom() {
    // WebKit keeps the zoom across the view's navigations and web processes (see the class comment): it is set once
    // per change, never again after a load or a crash
    if (!m_view) {
        return;
    }
    if (m_zoomWaiting) {
        // the page still renders the level before: WebKit would do the whole relayout for every level it gets, so
        // only the latest one follows, when the current render is done (finishZoomRenderWait)
        UC_WEBVIEW_TRACE_IF() << "zoom " << m_zoom << " (" << qRound(m_zoom * 100)
                              << " %) waits until the page rendered " << qRound(m_zoomApplied * 100) << " %";
        setZoomRendering(true);
        return;
    }
    if (qFuzzyCompare(m_zoom, m_zoomApplied)) {
        // back to the level WebKit has (in and out again during a render): nothing to do
        setZoomRendering(false);
        return;
    }
    UC_WEBVIEW_TRACE_IF() << "zoom " << m_zoom << " (" << qRound(m_zoom * 100) << " %)";
    lib().set_zoom(m_view, m_zoom);
    m_zoomApplied = m_zoom;
    startZoomRenderWait();
}

void WebViewItem::startZoomRenderWait() {
    m_zoomWaiting = true;
    ++m_zoomWaitGeneration;
    m_zoomRenderClock.start();
    setZoomRendering(true);
    // library 0.5.0: the page reports its rendering update after the zoom (uc_webview.h)
    if (lib().when_rendered && lib().when_rendered(m_view, &WebViewItem::onZoomRenderedCb, this) == 0) {
        return;
    }
    if (lib().when_rendered) {
        qCWarning(lcWebView()) << "uc_webview_when_rendered refused the request: the zoom counts as rendered at the "
                                  "first frame";
    }
    // Fallback: the first frame after the zoom, at most zoomFallbackTimeoutMs(). On a page that animates, or with a
    // frame already on its way, that frame can be older than the relayout: the indicator then ends early.
    m_zoomWaitsForFrame = true;
    m_zoomFallbackTimer.start(zoomFallbackTimeoutMs());
}

void WebViewItem::finishZoomRenderWait(int end) {
    if (!m_zoomWaiting) {
        return;  // a stale report: the view was freed (CANCELLED from uc_webview_free()) or the wait ended already
    }
    m_zoomWaiting = false;
    m_zoomWaitsForFrame = false;
    m_zoomFallbackTimer.stop();
    UC_WEBVIEW_TRACE_IF() << "zoom " << qRound(m_zoomApplied * 100) << " % rendered after "
                          << m_zoomRenderClock.elapsed() << " ms (" << qUtf8Printable(zoomRenderEndName(end)) << ")";
    if (!m_view) {
        setZoomRendering(false);
        return;
    }
    // a timeout or a cancellation (navigation, web process crash) ends the wait as well: the latest level goes to
    // WebKit now, or the indicator stops
    applyZoom();
}

void WebViewItem::setZoomRendering(bool rendering) {
    if (m_zoomRendering == rendering) {
        return;
    }
    const bool was = isZoomRendering();
    m_zoomRendering = rendering;
    if (isZoomRendering() != was) {
        emit zoomRenderingChanged();
    }
}

void WebViewItem::setMagnificationRendering(bool rendering) {
    if (m_magnificationRendering == rendering) {
        return;
    }
    const bool was = isZoomRendering();
    m_magnificationRendering = rendering;
    if (isZoomRendering() != was) {
        emit zoomRenderingChanged();
    }
}

// ---- pinch zoom (library 0.6.0) ----------------------------------------------------------------------------------

void WebViewItem::pinchStateChanged(qreal oldMagnification, bool oldPinching) {
    // the queue holds the scaled frame while a magnification renders; frames swap again once it is done, and the
    // first one becomes the picture's reference (updatePaintNode)
    m_frames.setHeld(m_pinch.holding());
    if (!qFuzzyCompare(oldMagnification, magnification())) {
        emit magnificationChanged();
    }
    if (oldPinching != m_pinch.pinching()) {
        emit pinchingChanged();
    }
    update();
}

void WebViewItem::onPinch(int phase, double scale, const QPointF &centre) {
    const qreal oldMagnification = magnification();
    const bool  oldPinching = isPinching();
    switch (phase) {
        case UC_WEBVIEW_PINCH_BEGIN:
            UC_WEBVIEW_TRACE_IF() << "pinch begin at " << centre.x() << "," << centre.y() << ", magnification "
                                  << magnification();
            m_pinch.begin(centre);
            // two fingers are no tap: the library gives WebKit no click for them either
            m_tapTouchId = -1;
            m_tapInputPanelTimer.stop();
            break;
        case UC_WEBVIEW_PINCH_UPDATE:
            m_pinch.update(scale);
            break;
        case UC_WEBVIEW_PINCH_END: {
            const PinchZoom::Commit commit = m_pinch.end();
            UC_WEBVIEW_TRACE_IF() << "pinch end, scale " << scale << ", magnification " << magnification()
                                  << (commit.valid ? "" : (m_pinch.holding() ? " (after the running render)"
                                                                             : " (no change)"));
            if (commit.valid) {
                commitMagnification(commit);
            }
            break;
        }
        case UC_WEBVIEW_PINCH_CANCEL:
            UC_WEBVIEW_TRACE_IF() << "pinch cancelled";
            m_pinch.cancel();
            break;
        default:
            break;  // a phase of a newer library: ignored
    }
    pinchStateChanged(oldMagnification, oldPinching);
}

void WebViewItem::commitMagnification(const PinchZoom::Commit &commit) {
    if (!m_view || !lib().set_magnification) {
        m_pinch.commitRefused();
        return;
    }
    UC_WEBVIEW_TRACE_IF() << "magnification " << m_pinch.appliedMagnification() << " -> " << commit.magnification
                          << " at " << commit.focal.x() << "," << commit.focal.y();
    if (lib().set_magnification(m_view, commit.magnification, commit.focal.x(), commit.focal.y()) != 0) {
        qCWarning(lcWebView()) << "uc_webview_set_magnification refused" << commit.magnification;
        m_pinch.commitRefused();
        return;
    }
    m_pinch.committed(commit);
    m_magnificationClock.start();
    setMagnificationRendering(true);
    // the scaled frame stays until the page rendered the new magnification; its first frame after that is sharp
    if (lib().when_rendered(m_view, &WebViewItem::onMagnificationRenderedCb, this) == 0) {
        ++m_magnificationRequests;
        return;
    }
    qCWarning(lcWebView()) << "uc_webview_when_rendered refused the request: the next frame ends the magnification wait";
    // the hold ends at once: the next frame shows what the page has (perhaps still the old scale for a moment)
    m_pinch.renderFinished();
    setMagnificationRendering(false);
}

void WebViewItem::finishMagnificationRender(int status) {
    // only the last request's report ends the hold: an earlier one belongs to a magnification the page has replaced
    if (m_magnificationRequests > 0) {
        --m_magnificationRequests;
    }
    if (m_magnificationRequests > 0 || !m_pinch.holding()) {
        return;  // a stale report (the view was freed, a navigation reset the magnification)
    }
    UC_WEBVIEW_TRACE_IF() << "magnification " << m_pinch.appliedMagnification() << " rendered after "
                          << m_magnificationClock.elapsed() << " ms (" << qUtf8Printable(zoomRenderEndName(status))
                          << ")";
    const qreal                oldMagnification = magnification();
    const bool                 oldPinching = isPinching();
    const PinchZoom::Commit    next = m_pinch.renderFinished();
    // DONE, and also TIMEOUT or CANCELLED: the next frame shows what the page has; pinches made meanwhile go on now
    if (next.valid) {
        commitMagnification(next);
    } else {
        setMagnificationRendering(false);
    }
    pinchStateChanged(oldMagnification, oldPinching);
}

void WebViewItem::onMagnificationChanged(double magnification) {
    // WebKit changed the magnification by itself: a new page starts at 1, a history step restores the page's own
    // (the library reports it after the navigation's commit, which has ended a pending render request already)
    UC_WEBVIEW_TRACE_IF() << "magnification changed to " << magnification << " by a navigation or history step";
    const qreal oldMagnification = this->magnification();
    const bool  oldPinching = isPinching();
    m_pinch.reset(magnification);
    // a render request still out (a same-document history step does not cancel it) stays counted: its late report
    // must not end the hold of a magnification made after this
    setMagnificationRendering(false);
    pinchStateChanged(oldMagnification, oldPinching);
}

void WebViewItem::zoomIn() {
    setZoom(nextZoomStep(m_zoom, 1));
}

void WebViewItem::zoomOut() {
    setZoom(nextZoomStep(m_zoom, -1));
}

void WebViewItem::reload() {
    if (m_view) {
        m_requests.answerLeavePage(true);
        lib().reload(m_view);
    }
}

void WebViewItem::stop() {
    if (m_view) {
        // the user stays: the navigation that asked stops as well
        m_requests.answerLeavePage(false);
        lib().stop(m_view);
    }
}

void WebViewItem::goBack() {
    if (m_view) {
        // the library ends pending requests for back and forward only when there is somewhere to go
        if (lib().can_go_back(m_view)) {
            m_requests.answerLeavePage(true);
        }
        lib().go_back(m_view);
    }
}

void WebViewItem::goForward() {
    if (m_view) {
        if (lib().can_go_forward(m_view)) {
            m_requests.answerLeavePage(true);
        }
        lib().go_forward(m_view);
    }
}

void WebViewItem::runJavaScript(const QString &script) {
    if (m_view) {
        lib().run_javascript(m_view, script.toUtf8().constData());
    }
}

void WebViewItem::loadHtml(const QString &html, const QUrl &baseUrl) {
    if (!baseUrl.isEmpty() && !loadAllowed(baseUrl, "HTML with the base URL")) {
        return;
    }
    m_pendingHtml = html;
    m_pendingBaseUrl = baseUrl;
    if (m_view) {
        m_requests.answerLeavePage(true);
        UC_WEBVIEW_TRACE_IF() << "load_html " << html.size() << " chars, base " << logUrl(baseUrl);
        const QByteArray base = baseUrl.toEncoded();
        lib().load_html(m_view, html.toUtf8().constData(), base.isEmpty() ? nullptr : base.constData());
    }
}

// ---- input -------------------------------------------------------------------------------------------------------

void WebViewItem::touchEvent(QTouchEvent *event) {
    if (!m_view) {
        event->ignore();
        return;
    }
    if (event->type() == QEvent::TouchCancel) {
        // Qt delivers a TouchCancel without touch points to every item holding a grab (qquickwindow.cpp
        // deliverTouchCancelEvent, qquickevents.cpp:1018-1035): end what is still down from our own records
        cancelActiveTouches("touch cancel");
        event->accept();
        return;
    }
    if (event->type() == QEvent::TouchBegin) {
        forceActiveFocus(Qt::MouseFocusReason);
    }
    const uint32_t time = eventTime(event);
    for (const QTouchEvent::TouchPoint &point : event->touchPoints()) {
        const int phase = touchPhaseForState(point.state());
        if (phase < 0) {
            continue;  // stationary: WebKit keeps the point at its last position (WPEWebViewPlatform.cpp:327-358)
        }
        const int     id = point.id();
        const QPointF pos = devicePoint(point.pos());
        int           slot;
        if (phase == UC_WEBVIEW_TOUCH_DOWN) {
            if (m_touchSlots.contains(id)) {
                // a press for an id we still consider down (its release never reached us): end that sequence
                // first, WebKit keys its touch points and its gesture by the id
                sendTouch(id, m_touchSlots.value(id), UC_WEBVIEW_TOUCH_CANCEL, m_activeTouches.value(id), time);
                m_touchSlots.remove(id);
                if (id == m_tapTouchId) {
                    m_tapTouchId = -1;
                }
            }
            slot = allocateTouchSlot(id);
            m_activeTouches.insert(id, pos);
            m_touchStarts.insert(id, pos);
            if (m_touchSlots.size() == 1) {
                // the first finger of a sequence, the one WebKit's gesture controller follows: a tap candidate. A
                // new press also ends a previous tap's wait for the keyboard: the user is scrolling or tapping on.
                m_tapTouchId = id;
                m_tapInputPanelTimer.stop();
            } else if (m_pinchZoomAvailable) {
                // a second finger: the library's pinch recognition cancels WebKit's tap of the first one
                // (libucwebview 0.6.0, uc_pinch.h), so its release is no tap for the keyboard either
                m_tapTouchId = -1;
            }
        } else {
            if (!m_touchSlots.contains(id)) {
                UC_WEBVIEW_TRACE_IF() << "touch id " << id << " " << touchPhaseName(phase)
                                      << " without a press, dropped";
                continue;
            }
            slot = m_touchSlots.value(id);
            if (phase == UC_WEBVIEW_TOUCH_MOVE) {
                m_activeTouches.insert(id, pos);
                if (id == m_tapTouchId && exceedsTapDistance(m_touchStarts.value(id), pos)) {
                    m_tapTouchId = -1;  // WebKit scrolls from here on, no click (WPEGestureDetector.cpp:63-69)
                }
            }
        }
        sendTouch(id, slot, phase, pos, time);
        if (phase == UC_WEBVIEW_TOUCH_UP) {
            const bool tap = id == m_tapTouchId && contains(point.pos());
            if (id == m_tapTouchId) {
                m_tapTouchId = -1;
            }
            m_activeTouches.remove(id);
            m_touchStarts.remove(id);
            m_touchSlots.remove(id);
            if (tap) {
                onTap();
            }
        }
    }
    event->accept();
}

bool WebViewItem::exceedsTapDistance(const QPointF &from, const QPointF &to) const {
    // WebKit's own drag threshold, which the library sets on its display (16 px from library 0.3.0)
    const QPointF delta = to - from;
    return std::hypot(delta.x(), delta.y()) > WebViewLibrary::instance()->dragThreshold();
}

int WebViewItem::allocateTouchSlot(int id) {
    int slot = 0;
    while (std::find(m_touchSlots.cbegin(), m_touchSlots.cend(), slot) != m_touchSlots.cend()) {
        ++slot;
    }
    m_touchSlots.insert(id, slot);
    return slot;
}

void WebViewItem::sendTouch(int id, int slot, int phase, const QPointF &devicePos, uint32_t time) {
    if (traceEnabled()) {
        // the distance from the press tells a tap from a drag: WebKit turns the sequence into a drag (no click)
        // once a move is more than WPE_SETTING_DRAG_THRESHOLD (WebViewLibrary::dragThreshold) away from the press
        const QPointF start = m_touchStarts.value(id, devicePos);
        const QPointF delta = devicePos - start;
        qCInfo(lcWebView()).nospace() << "trace " << static_cast<const void *>(this) << ": touch slot " << slot
                                      << " (qt id " << id << ") " << touchPhaseName(phase) << " " << devicePos.x()
                                      << "," << devicePos.y() << " ms " << time << " moved "
                                      << std::hypot(delta.x(), delta.y()) << " px, " << m_touchSlots.size()
                                      << " down";
    }
    FocusReplayScope replay(&m_focusReplayDepth);
    lib().touch(m_view, slot, phase, devicePos.x(), devicePos.y(), time);
}

void WebViewItem::cancelActiveTouches(const char *why) {
    m_tapTouchId = -1;
    if (m_touchSlots.isEmpty() || !m_view) {
        m_activeTouches.clear();
        m_touchStarts.clear();
        m_touchSlots.clear();
        return;
    }
    UC_WEBVIEW_TRACE_IF() << "cancelling " << m_touchSlots.size() << " touch point(s): " << why;
    const QHash<int, int> activeSlots = m_touchSlots;
    for (auto it = activeSlots.cbegin(); it != activeSlots.cend(); ++it) {
        sendTouch(it.key(), it.value(), UC_WEBVIEW_TOUCH_CANCEL, m_activeTouches.value(it.key()), 0);
    }
    m_activeTouches.clear();
    m_touchStarts.clear();
    m_touchSlots.clear();
}

void WebViewItem::touchUngrabEvent() {
    // another item took the touch grab (a filtering Flickable, an item made invisible): Qt 5 sends no TouchCancel
    // in this case, only this call (qquickevents.cpp:925-927, qquickwindow.cpp:1052-1058)
    cancelActiveTouches("touch grab taken by another item");
}

void WebViewItem::mousePressEvent(QMouseEvent *event) {
    const int button = pointerButtonForQt(event->button());
    if (!m_view || button == 0) {
        event->ignore();
        return;
    }
    forceActiveFocus(Qt::MouseFocusReason);
    const QPointF pos = devicePoint(event->localPos());
    m_pressedButtons |= event->button();
    m_lastMousePos = pos;
    if (event->button() == Qt::LeftButton) {
        // a click without a drag counts as a tap for the keyboard, like the touch path
        m_mouseTapCandidate = true;
        m_mousePressPos = pos;
        m_tapInputPanelTimer.stop();
    }
    UC_WEBVIEW_TRACE_IF() << "pointer button " << button << " down " << pos.x() << "," << pos.y() << " ms "
                          << eventTime(event);
    FocusReplayScope replay(&m_focusReplayDepth);
    lib().pointer_button(m_view, button, 1, pos.x(), pos.y(), modifiersForEvent(event->modifiers()), eventTime(event));
    event->accept();
}

void WebViewItem::mouseMoveEvent(QMouseEvent *event) {
    if (!m_view) {
        event->ignore();
        return;
    }
    const QPointF pos = devicePoint(event->localPos());
    m_lastMousePos = pos;
    if (m_mouseTapCandidate && exceedsTapDistance(m_mousePressPos, pos)) {
        m_mouseTapCandidate = false;
    }
    UC_WEBVIEW_TRACE_IF() << "pointer move " << pos.x() << "," << pos.y() << " ms " << eventTime(event);
    lib().pointer_move(m_view, pos.x(), pos.y(), modifiersForEvent(event->modifiers(), event->buttons()),
                       eventTime(event));
    event->accept();
}

void WebViewItem::mouseReleaseEvent(QMouseEvent *event) {
    const int button = pointerButtonForQt(event->button());
    if (!m_view || button == 0) {
        event->ignore();
        return;
    }
    const QPointF pos = devicePoint(event->localPos());
    m_pressedButtons &= ~event->button();
    m_lastMousePos = pos;
    UC_WEBVIEW_TRACE_IF() << "pointer button " << button << " up " << pos.x() << "," << pos.y() << " ms "
                          << eventTime(event);
    lib().pointer_button(m_view, button, 0, pos.x(), pos.y(), modifiersForEvent(event->modifiers()), eventTime(event));
    event->accept();
    if (event->button() == Qt::LeftButton) {
        const bool tap = m_mouseTapCandidate && contains(event->localPos());
        m_mouseTapCandidate = false;
        if (tap) {
            onTap();
        }
    }
}

void WebViewItem::mouseUngrabEvent() {
    m_mouseTapCandidate = false;
    // the mouse grab went elsewhere while buttons were down: release them for the page (simulator only)
    if (!m_view || m_pressedButtons == Qt::NoButton) {
        m_pressedButtons = Qt::NoButton;
        return;
    }
    UC_WEBVIEW_TRACE_IF() << "mouse grab taken by another item, releasing buttons";
    for (Qt::MouseButton qtButton : {Qt::LeftButton, Qt::MiddleButton, Qt::RightButton}) {
        if (m_pressedButtons & qtButton) {
            lib().pointer_button(m_view, pointerButtonForQt(qtButton), 0, m_lastMousePos.x(), m_lastMousePos.y(), 0,
                                 0);
        }
    }
    m_pressedButtons = Qt::NoButton;
}

void WebViewItem::wheelEvent(QWheelEvent *event) {
    if (!m_view) {
        event->ignore();
        return;
    }
    const QPointF pos = devicePoint(event->position());
    const QPoint  pixels = event->pixelDelta();
    if (!pixels.isNull()) {
        const QPointF delta = devicePoint(pixels);
        lib().scroll(m_view, delta.x(), delta.y(), pos.x(), pos.y(), 1, eventTime(event));
    } else {
        // Qt reports eighths of a degree, 120 per wheel notch; the library takes notches when precise is 0
        const QPointF ticks = QPointF(event->angleDelta()) / 120.0;
        lib().scroll(m_view, ticks.x(), ticks.y(), pos.x(), pos.y(), 0, eventTime(event));
    }
    event->accept();
}

void WebViewItem::keyPressEvent(QKeyEvent *event) {
    keyEvent(event, true);
}

void WebViewItem::keyReleaseEvent(QKeyEvent *event) {
    keyEvent(event, false);
}

void WebViewItem::keyEvent(QKeyEvent *event, bool pressed) {
    const uint32_t keysym = m_view ? xkbKeysymForKey(event->key(), event->text()) : 0;
    if (keysym == 0) {
        // not a key of the page (BACK, HOME, the remote's other buttons): leave it to the QML focus chain
        event->ignore();
        return;
    }
    if (traceEnabled()) {
        // a character key appears as "printable" only: its keysym and its scan code (which names the key on a
        // keyboard) would spell what was typed, a PIN into a password field just as well
        if (const char *name = specialKeyName(keysym)) {
            UC_WEBVIEW_TRACE_IF() << "key special " << name << (pressed ? " down" : " up") << " (keysym 0x"
                                  << Qt::hex << keysym << Qt::dec << ", scancode " << event->nativeScanCode()
                                  << ") ms " << eventTime(event);
        } else {
            UC_WEBVIEW_TRACE_IF() << "key printable" << (pressed ? " down" : " up") << " ms " << eventTime(event);
        }
    }
    if (pressed) {
        // a user gesture for the page, like a tap: an element it focuses in response is not flagged as
        // script-focused by WebKit (onImeFocus)
        m_lastUserGesture.restart();
    }
    lib().key(m_view, keysym, event->nativeScanCode(), pressed ? 1 : 0, modifiersForEvent(event->modifiers()),
              eventTime(event));
    event->accept();
}

void WebViewItem::pageUp() {
    sendPageKey(Qt::Key_PageUp);
}

void WebViewItem::pageDown() {
    sendPageKey(Qt::Key_PageDown);
}

void WebViewItem::sendPageKey(int key) {
    // WebKit takes key events whether the view has the focus or not (WPEWebViewPlatform.cpp:387-405) and sends them
    // to the page's focused frame. A Page_Up/Down keydown without modifiers that the page does not handle itself
    // scrolls by a page (EventHandler.cpp:5036-5046): the first scroller around the page's focused element (or the
    // element last clicked) that can still move, else the document (EventHandler.cpp:1467-1540, RenderBox.cpp:
    // 1153-1180; the keyboard scroll animator that a key release would stop is off outside macOS,
    // EventHandler.cpp:5028-5033, UnifiedWebPreferences.yaml:3035-3048). In a focused text area or contenteditable
    // element the key moves the caret a page instead (WebEditorClientWPE.cpp:87-88, EditorCommand.cpp:225-238), like
    // in a desktop browser.
    if (!m_view) {
        return;
    }
    const uint32_t keysym = xkbKeysymForKey(key, QString());
    const uint32_t keycode = key == Qt::Key_PageUp ? kKeycodePageUp : kKeycodePageDown;
    UC_WEBVIEW_TRACE_IF() << (key == Qt::Key_PageUp ? "page up: " : "page down: ") << specialKeyName(keysym)
                          << " press and release (keysym 0x" << Qt::hex << keysym << Qt::dec << ", scancode "
                          << keycode << ")";
    // a user gesture for the page, like a forwarded key (keyEvent)
    m_lastUserGesture.restart();
    const uint32_t time = eventTimeNow();
    lib().key(m_view, keysym, keycode, 1, 0, time);
    lib().key(m_view, keysym, keycode, 0, 0, time);
}

// ---- text input (the Qt Virtual Keyboard contract) ---------------------------------------------------------------

QVariant WebViewItem::inputMethodQuery(Qt::InputMethodQuery query) const {
    switch (query) {
        case Qt::ImEnabled:
            return m_imeEnabled;
        case Qt::ImHints:
            return static_cast<int>(m_imeHints);
        case Qt::ImCursorRectangle:
            return m_cursorRectangle;
        case Qt::ImSurroundingText:
            return m_surroundingText;
        case Qt::ImCursorPosition:
            return qMax(0, m_cursorPosition);
        case Qt::ImAnchorPosition:
            return qMax(0, m_anchorPosition);
        default:
            return QQuickItem::inputMethodQuery(query);
    }
}

void WebViewItem::inputMethodEvent(QInputMethodEvent *event) {
    // which parts the event has, never their text, length or offsets
    if (traceEnabled()) {
        const bool commit = !event->commitString().isEmpty();
        const bool preedit = !event->preeditString().isEmpty();
        const bool replacement = event->replacementLength() > 0;
        UC_WEBVIEW_TRACE_IF() << "input method event:" << (commit ? " commit" : "")
                              << (preedit ? " preedit" : (m_preeditActive ? " preedit end" : ""))
                              << (replacement ? " replacement" : "")
                              << (commit || preedit || replacement || m_preeditActive ? "" : " empty")
                              << (m_imeEnabled ? "" : " (ignored, no editable element)");
    }
    if (!m_view || !m_imeEnabled) {
        event->ignore();
        return;
    }

    if (event->replacementLength() > 0) {
        // the keyboard replaces text around the caret (word correction): offset relative to the caret, like WPE's
        // delete-surrounding
        lib().ime_delete_surrounding(m_view, event->replacementStart(), event->replacementLength());
    }

    const QString &commit = event->commitString();
    if (!commit.isEmpty()) {
        lib().ime_commit(m_view, commit.toUtf8().constData());
        m_preeditActive = false;
    }

    const QString &preedit = event->preeditString();
    if (!preedit.isEmpty()) {
        int cursor = preedit.size();
        for (const QInputMethodEvent::Attribute &attribute : event->attributes()) {
            if (attribute.type == QInputMethodEvent::Cursor) {
                cursor = attribute.start;
                break;
            }
        }
        lib().ime_preedit(m_view, preedit.toUtf8().constData(), cursor);
        m_preeditActive = true;
    } else if (m_preeditActive) {
        lib().ime_preedit(m_view, "", 0);
        m_preeditActive = false;
    }

    event->accept();
}

}  // namespace webview
}  // namespace uc
