// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QHash>
#include <QMetaObject>
#include <QPointF>
#include <QQuickItem>
#include <QRectF>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <functional>

#include "uc_webview.h"
#include "webviewframequeue.h"
#include "webviewpinch.h"

class QQuickWindow;
class QScreen;

namespace uc {
namespace webview {

/**
 * QML `WebView` (module UC.WebView 1.0): a WPE WebKit web view rendered by the firmware's libucwebview.so.1 into the
 * Qt Quick scene graph.
 *
 * The item owns one uc_webview and does everything that touches the library on the GUI thread: creation, resize,
 * mapping, input, text input and the frame reports. Frames arrive through the frame_ready callback (GUI thread)
 * and are queued; updatePaintNode() (render thread, GUI thread blocked) binds the newest one to the WebViewNode
 * texture; QQuickWindow::frameSwapped (emitted on the render thread) is delivered through a queued connection to
 * onFrameSwapped() on the GUI thread, which reports uc_webview_frame_rendered() for the frame now on screen and
 * uc_webview_frame_release() for the frames the scene graph no longer samples (docs/webview.md).
 *
 * Without the library (desktop simulator, firmware without the uc-webview package) `available` is false and the
 * item draws nothing.
 *
 * Input rules (docs/webview.md):
 * - Touch points are forwarded one by one with the ids WebKit sees from WPE's own DRM platform: the lowest free
 *   slot (0 for a single finger, WPEDRMSeat.cpp:491-545) instead of Qt's ids, which on the device are the kernel's
 *   ever-incrementing tracking ids. Every point that went down is remembered until its release, so that a Qt
 *   TouchCancel (delivered without touch points) or a grab taken by another item (touchUngrabEvent(), no cancel
 *   event at all) ends the sequence in WebKit with a touch-cancel; moves and releases of points whose press was
 *   not seen are dropped like the DRM platform does. WebKit's gesture controller keys its sequence by id, and a
 *   point that is never released would otherwise block every later tap.
 * - The item takes the keyboard focus only on a user press. It asks the global QInputMethod to show or hide the
 *   on-screen keyboard only while it has the active focus; an `ime_focus` callback that arrives while another
 *   item owns the focus (WebKit reports focus-out when the item itself loses the focus) changes the item's state
 *   and nothing else. Losing the active focus resets the input method state without touching the keyboard.
 * - A tap (press and release within WebKit's drag distance, WebViewLibrary::dragThreshold) while the page's
 *   editable element is focused asks for the keyboard again, like Qt's TextInput re-opens it on a press into the
 *   focused field: WebKit reports the element's focus only when it changes, so after the keyboard's hide key a tap
 *   on the same field would bring nothing. The request waits kTapInputPanelDelayMs; a tap that moves the page's
 *   focus elsewhere (a button, the page background) reports `ime_focus(0)` before that and cancels it.
 * - WebKit's INHIBIT_OSK hint means inputmode="none" or "focused without a user gesture" (autofocus, a script).
 *   The focus report itself never opens the keyboard then. A tap on the element does, unless the report came from
 *   the web process within the tap window after the user's own tap or key press: that focus was the user's, so
 *   the hint is inputmode="none" and holds for taps too. Reports WebKit repeats inside a library call that changes
 *   the view's focus (set_focused, a press that focuses the view) are replays and keep what the item knew.
 * - canGoBack/canGoForward are read from the library again on every load event, frame and title change: a
 *   same-document navigation (history.pushState, a fragment link) changes the history without a load event.
 * - `zoom` is the page zoom WebKit uses (1 = 100 %), the item being its only writer: WebKit keeps it for the view
 *   across navigations, reloads and web process swaps (WebPageProxy::pageZoomFactor, WebPageProxy.cpp:6742-6748;
 *   a new web process gets it with its creation parameters, WebPageProxy.cpp:14262, WebPage.cpp:1067) and starts
 *   every new view at 1 (WebPageProxy.h:3971). So the zoom lasts as long as the item, and a zoom set before the
 *   view exists is applied when it is created. zoomIn()/zoomOut() step through browser-like levels.
 * - A zoom step re-renders the whole page in the web process, seconds on a large page. `zoomRendering` is true from a
 *   zoom change until the page has rendered the latest level: uc_webview_when_rendered() (library 0.5.0) reports it;
 *   with an older library the first frame after the zoom counts, at most kZoomFallbackTimeoutMs. Zoom changes while
 *   the page renders only move `zoom`; the item passes the latest level to WebKit once the current render is done,
 *   because WebKit rebuilds the style and lays the page out again for every zoom it gets (LocalFrame.cpp:1085-1130):
 *   three quick steps cost at most two relayouts.
 * - pageUp()/pageDown() press and release Page_Up/Page_Down for the page, whichever item has the focus: WebKit
 *   scrolls the focused scroller by a page, or moves the caret a page in a focused text area (docs/webview.md).
 * - Pinch zoom (library 0.6.0, docs/webview.md "Pinch zoom"): the library recognises a two-finger pinch in the touches
 *   the page leaves alone and reports it (the pinch callback; the view is then created with uc_webview_new_sized()).
 *   While the fingers move, updatePaintNode() draws the frame scaled around the point the pinch began at (the node's
 *   rect and source rect, no new frame needed); at the end the item asks for the magnification once
 *   (uc_webview_set_magnification(), clamped to 1..4) and keeps the scaled frame (the frame queue is held) until
 *   uc_webview_when_rendered() reports; the first frame after it is sharp at the new magnification. A timeout or a
 *   cancellation ends the hold the same way: the next frame shows what the page has. `magnification` is what the user
 *   sees, `pinching` is true between the pinch's begin and end; `zoomRendering` covers the magnification's render too.
 *   Page zoom (`zoom`, VOLUME) and magnification are separate WebKit factors: the keys leave the magnification alone.
 *   A new page starts at 1; back, forward and reload restore the page's magnification (WebKit, reported by the
 *   library). Without the library's 0.6.0 functions two fingers behave as before.
 * - Loads go through the library's URL scheme allowlist (http, https, about, data; file with
 *   UC_WEBVIEW_ALLOW_FILE_URLS=1): the item refuses the others itself with `loadFailed(url)` and a log line.
 * - UC_WEBVIEW_TRACE=1 logs every forwarded input event, input method callback, load request and load event at
 *   INFO level (uc.ui.webview), never what the user types or what the page's editable element holds: keys as
 *   "printable" or a named special key, input method events and surrounding text updates without text, lengths
 *   or caret offsets. URLs in every log line lose credentials, query and fragment (UC_WEBVIEW_LOG_FULL_URLS=1 keeps
 *   the last two). docs/webview.md, "What the trace logs".
 *
 * Frame pacing: WebKit ticks requestAnimationFrame and animations at the refresh rate the library is given, 60 Hz
 * otherwise. The item passes QScreen::refreshRate() of its window's screen (eglfs_kms: the DRM mode's vrefresh, 50 on
 * the Remote 3) before it creates the view, because WebKit reads the rate once at the first page load, and again
 * when the window moves to another screen or the screen's rate changes. UC_WEBVIEW_REFRESH_RATE in the environment
 * wins inside the library (docs/webview.md).
 */
class WebViewItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int loadProgress READ loadProgress NOTIFY loadProgressChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY navigationHistoryChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY navigationHistoryChanged)
    Q_PROPERTY(bool available READ isAvailable CONSTANT)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    // page zoom factor, 1 = 100 %; 0 < zoom <= 20 like uc_webview_set_zoom(), other values are refused
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    // a zoom or magnification change has not been rendered by the page yet (busy indicator)
    Q_PROPERTY(bool zoomRendering READ isZoomRendering NOTIFY zoomRenderingChanged)
    // pinch zoom (library 0.6.0): the magnification the user sees, 1 = none; a pinch in progress; the library can
    Q_PROPERTY(qreal magnification READ magnification NOTIFY magnificationChanged)
    Q_PROPERTY(bool pinching READ isPinching NOTIFY pinchingChanged)
    Q_PROPERTY(bool pinchZoomAvailable READ isPinchZoomAvailable CONSTANT)
    // frame counters for the validation on the device: both must stay equal after the last frame
    Q_PROPERTY(int framesReceived READ framesReceived NOTIFY frameStatsChanged)
    Q_PROPERTY(int framesReleased READ framesReleased NOTIFY frameStatsChanged)

 public:
    explicit WebViewItem(QQuickItem *parent = nullptr);
    ~WebViewItem() override;

    // qmlRegisterType<WebViewItem>("UC.WebView", 1, 0, "WebView")
    static void registerQmlTypes();

    QUrl    url() const { return m_url; }
    void    setUrl(const QUrl &url);
    QString title() const { return m_title; }
    bool    isLoading() const { return m_loading; }
    int     loadProgress() const { return m_loadProgress; }
    bool    canGoBack() const { return m_canGoBack; }
    bool    canGoForward() const { return m_canGoForward; }
    bool    isAvailable() const { return m_available; }
    QColor  backgroundColor() const { return m_backgroundColor; }
    void    setBackgroundColor(const QColor &color);
    qreal   zoom() const { return m_zoom; }
    void    setZoom(qreal zoom);
    bool    isZoomRendering() const { return m_zoomRendering || m_magnificationRendering; }
    qreal   magnification() const { return m_pinch.magnification(); }
    bool    isPinching() const { return m_pinch.pinching(); }
    bool    isPinchZoomAvailable() const { return m_pinchZoomAvailable; }
    int     framesReceived() const { return m_framesReceived; }
    int     framesReleased() const { return m_framesReleased; }

    Q_INVOKABLE void reload();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void goForward();
    Q_INVOKABLE void runJavaScript(const QString &script);
    Q_INVOKABLE void loadHtml(const QString &html, const QUrl &baseUrl = QUrl());
    // One zoom level up or down in browser-like steps from 50 to 300 % (the table in webviewitem.cpp); a zoom
    // between two levels goes to the next one in that direction. Nothing happens beyond the first or last level.
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    // The Page_Up / Page_Down key, pressed and released, as if typed on the page (the item's focus does not matter)
    Q_INVOKABLE void pageUp();
    Q_INVOKABLE void pageDown();

    // what the page's focused editable element reports, for Qt's input method (public in QQuickItem)
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

    // Tests: receives the item's QInputMethod::show() (true) / hide() (false) requests instead of the real input
    // method. nullptr restores the default.
    static void setInputPanelHookForTest(std::function<void(bool)> hook);
    // Tests: switches the input trace on or off (default: UC_WEBVIEW_TRACE, read at the first trace line).
    static void setTraceEnabledForTest(bool enabled);
    // Tests: the fallback's wait for a frame after a zoom (default kZoomFallbackTimeoutMs); 0 restores the default.
    static void setZoomFallbackTimeoutForTest(int ms);

    // How long a tap into the page waits before it asks for the keyboard again (see the class comment): enough
    // for WebKit to turn the tap into a click and report a focus change of the page (a few IPC round trips).
    static constexpr int kTapInputPanelDelayMs = 200;

    // Without uc_webview_when_rendered() (library older than 0.5.0) a zoom counts as rendered at the first frame after
    // it, or after this long without one (a zoom that changes nothing on screen brings no frame).
    static constexpr int kZoomFallbackTimeoutMs = 5000;

 signals:
    void urlChanged();
    void titleChanged();
    void loadingChanged();
    void loadProgressChanged();
    void navigationHistoryChanged();
    void backgroundColorChanged();
    void zoomChanged();
    void zoomRenderingChanged();
    void magnificationChanged();
    void pinchingChanged();
    void frameStatsChanged();
    // the web process died (UC_WEBVIEW_TERMINATED_*); the item reloads once by itself
    void crashed(int reason);
    // a load failed; `loading` turns false with the following finished event. Also emitted, without any other
    // change, for a URL outside the scheme allowlist (setUrl(), loadHtml() base URL): nothing is loaded then.
    void loadFailed(const QUrl &url);

 protected:
    void      geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void      itemChange(ItemChange change, const ItemChangeData &value) override;
    QSGNode  *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void      releaseResources() override;
    void      touchEvent(QTouchEvent *event) override;
    void      touchUngrabEvent() override;
    void      mousePressEvent(QMouseEvent *event) override;
    void      mouseMoveEvent(QMouseEvent *event) override;
    void      mouseReleaseEvent(QMouseEvent *event) override;
    void      mouseUngrabEvent() override;
    void      wheelEvent(QWheelEvent *event) override;
    void      keyPressEvent(QKeyEvent *event) override;
    void      keyReleaseEvent(QKeyEvent *event) override;
    void      inputMethodEvent(QInputMethodEvent *event) override;

 private:
    // libucwebview callbacks (GUI thread, from the GLib main context)
    static const uc_webview_callbacks &callbacks();
    static void onFrameReadyCb(uc_webview *, const uc_webview_frame *frame, void *user);
    static void onLoadChangedCb(uc_webview *, int event, const char *uri, void *user);
    static void onTitleChangedCb(uc_webview *, const char *title, void *user);
    static void onProgressChangedCb(uc_webview *, double progress, void *user);
    static void onWebProcessTerminatedCb(uc_webview *, int reason, void *user);
    static void onImeFocusCb(uc_webview *, int focused, unsigned hints, unsigned purpose, const char *surrounding,
                             int cursor, void *user);
    static void onImeCursorAreaCb(uc_webview *, int x, int y, int w, int h, void *user);
    static void onImeSurroundingCb(uc_webview *, const char *text, int cursor, int anchor, void *user);
    static void onZoomRenderedCb(uc_webview *, int status, void *user);
    static void onPinchCb(uc_webview *, int phase, double scale, double x, double y, void *user);
    static void onMagnificationChangedCb(uc_webview *, double magnification, void *user);
    static void onMagnificationRenderedCb(uc_webview *, int status, void *user);

    void onFrameReady(const uc_webview_frame &frame);
    void onLoadChanged(int event, const char *uri);
    void onWebProcessTerminated(int reason);
    void onImeFocus(bool focused, unsigned hints, unsigned purpose, const char *surrounding, int cursor);
    void onImeSurrounding(const char *text, int cursor, int anchor);
    void onFrameSwapped();

    void   attachWindow(QQuickWindow *window);
    void   watchScreen(QScreen *screen);
    void   updateRefreshRate();
    void   ensureView();
    void   destroyView();
    void   updateMapped();
    void   updateSize();
    void   updateNavigationHistory();
    void   applyBackgroundColor();
    void   applyZoom();
    void   startZoomRenderWait();
    // end: UC_WEBVIEW_RENDERED_* from the library, or one of the fallback's ends (webviewitem.cpp)
    void   finishZoomRenderWait(int end);
    void   setZoomRendering(bool rendering);
    void   onPinch(int phase, double scale, const QPointF &centre);
    void   onMagnificationChanged(double magnification);
    void   commitMagnification(const PinchZoom::Commit &commit);
    void   finishMagnificationRender(int status);
    // after every change of the pinch state: the frame queue's hold, the properties, a new picture
    void   pinchStateChanged(qreal oldMagnification, bool oldPinching);
    void   setMagnificationRendering(bool rendering);
    void   sendPageKey(int key);
    void   releaseFrames();
    void   releaseFrame(void *handle);
    void   keyEvent(QKeyEvent *event, bool pressed);
    void   sendTouch(int id, int slot, int phase, const QPointF &devicePos, uint32_t time);
    int    allocateTouchSlot(int id);
    void   cancelActiveTouches(const char *why);
    void   requestInputPanel(bool show);
    void   onTap();
    void   onTapInputPanelTimeout();
    bool   keyboardInhibitedOnTap() const;
    bool   exceedsTapDistance(const QPointF &from, const QPointF &to) const;
    bool   loadAllowed(const QUrl &url, const char *what);
    void   resetImeState();
    void   setLoading(bool loading);
    void   setUrlFromPage(const QUrl &url);
    QSize  devicePixelSize() const;
    QPointF devicePoint(const QPointF &itemPoint) const;

    uc_webview *m_view = nullptr;
    const bool  m_available;

    QUrl    m_url;
    QString m_title;
    bool    m_loading = false;
    int     m_loadProgress = 0;
    bool    m_canGoBack = false;
    bool    m_canGoForward = false;
    bool    m_loadFailed = false;
    bool    m_reloadedAfterCrash = false;
    QColor  m_backgroundColor = Qt::black;
    qreal   m_zoom = 1.0;  // what WebKit uses for the view: its default until the item sets another one
    // the zoom render wait (class comment): the level last passed to WebKit, a wait running for it (a library
    // request or the fallback), the fallback's state, the time since the zoom for the trace
    qreal         m_zoomApplied = 1.0;
    bool          m_zoomWaiting = false;
    bool          m_zoomRendering = false;
    bool          m_zoomWaitsForFrame = false;  // fallback: the next frame ends the wait
    quint64       m_zoomWaitGeneration = 0;     // ignores a deferred fallback end of an earlier wait
    QTimer        m_zoomFallbackTimer;
    QElapsedTimer m_zoomRenderClock;
    // pinch zoom (library 0.6.0): the picture and the hold (webviewpinch.h), the magnification renders requested and
    // not reported yet (only the last one's report ends the hold), the time since the last commit for the trace
    const bool    m_pinchZoomAvailable;
    PinchZoom     m_pinch;
    int           m_magnificationRequests = 0;
    bool          m_magnificationRendering = false;
    QElapsedTimer m_magnificationClock;
    QString m_pendingHtml;
    QUrl    m_pendingBaseUrl;

    bool  m_mapped = false;
    qreal m_devicePixelRatio = 1.0;
    QSize m_viewSize;

    FrameQueue m_frames;
    int        m_framesReceived = 0;
    int        m_framesReleased = 0;
    bool       m_renderWarned = false;

    // text input state reported by the page, answered to Qt's input method (the virtual keyboard)
    bool                 m_imeEnabled = false;
    Qt::InputMethodHints m_imeHints = Qt::ImhNone;
    QString              m_surroundingText;
    int                  m_cursorPosition = -1;
    int                  m_anchorPosition = -1;
    QRectF               m_cursorRectangle;
    bool                 m_preeditActive = false;
    bool                 m_imeInhibitOsk = false;  // UC_WEBVIEW_IME_HINT_INHIBIT_OSK of the focused element
    // the page's focused element got INHIBIT_OSK with a focus the user gave it: inputmode="none", which also holds
    // for taps. Set from fresh reports only (onImeFocus), kept while the view alone loses the focus.
    bool                 m_imeInhibitHonouredOnTap = false;
    QTimer               m_tapInputPanelTimer;      // a tap's deferred keyboard request (kTapInputPanelDelayMs)
    QElapsedTimer        m_lastUserGesture;         // the last tap or forwarded key press
    int                  m_focusReplayDepth = 0;    // > 0 inside a library call that can change the view's focus

    // touch points that went down and were not released yet: id -> last position in device pixels; the position
    // the sequence started at, for the trace (tap or drag: WebKit's drag threshold is 8 px)
    QHash<int, QPointF> m_activeTouches;
    QHash<int, QPointF> m_touchStarts;
    QHash<int, int>     m_touchSlots;  // Qt touch id -> slot sent to the library
    // the Qt id of the point that may still end as a tap: the first finger of a sequence (WebKit's gesture
    // controller follows only that one) until a move takes it farther than the drag distance; -1 for none
    int                 m_tapTouchId = -1;
    Qt::MouseButtons    m_pressedButtons = Qt::NoButton;
    QPointF             m_lastMousePos;
    QPointF             m_mousePressPos;
    bool                m_mouseTapCandidate = false;  // primary button down and not dragged farther than a tap

    QMetaObject::Connection m_frameSwappedConnection;
    QMetaObject::Connection m_windowVisibleConnection;
    QMetaObject::Connection m_screenChangedConnection;
    QMetaObject::Connection m_refreshRateConnection;
};

}  // namespace webview
}  // namespace uc
