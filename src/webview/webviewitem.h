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
    int     framesReceived() const { return m_framesReceived; }
    int     framesReleased() const { return m_framesReleased; }

    Q_INVOKABLE void reload();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void goForward();
    Q_INVOKABLE void runJavaScript(const QString &script);
    Q_INVOKABLE void loadHtml(const QString &html, const QUrl &baseUrl = QUrl());

    // what the page's focused editable element reports, for Qt's input method (public in QQuickItem)
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

    // Tests: receives the item's QInputMethod::show() (true) / hide() (false) requests instead of the real input
    // method. nullptr restores the default.
    static void setInputPanelHookForTest(std::function<void(bool)> hook);
    // Tests: switches the input trace on or off (default: UC_WEBVIEW_TRACE, read at the first trace line).
    static void setTraceEnabledForTest(bool enabled);

    // How long a tap into the page waits before it asks for the keyboard again (see the class comment): enough
    // for WebKit to turn the tap into a click and report a focus change of the page (a few IPC round trips).
    static constexpr int kTapInputPanelDelayMs = 200;

 signals:
    void urlChanged();
    void titleChanged();
    void loadingChanged();
    void loadProgressChanged();
    void navigationHistoryChanged();
    void backgroundColorChanged();
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
