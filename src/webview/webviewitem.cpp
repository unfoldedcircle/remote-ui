// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewitem.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QThread>
#include <QTimer>
#include <QTouchEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

#include "../logging.h"
#include "webviewinput.h"
#include "webviewlibrary.h"
#include "webviewtexture.h"

namespace uc {
namespace webview {

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

// UC_WEBVIEW_TRACE=1: every forwarded input event and every input method callback at INFO level, which is what
// the device journal keeps (docs/webview.md)
bool traceEnabled() {
    static const bool enabled = qEnvironmentVariableIntValue("UC_WEBVIEW_TRACE") > 0;
    return enabled;
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

// WebKit's tap/drag distance in view pixels, as the library configured it (UC_WEBVIEW_DRAG_THRESHOLD or 8 px)
double tapDistance() {
    static const double distance = dragThresholdFromEnv(qgetenv("UC_WEBVIEW_DRAG_THRESHOLD"));
    return distance;
}

// URLs may carry credentials: never log them in full
QString loggableUrl(const QUrl &url) {
    return url.toString(QUrl::RemoveUserInfo);
}

}  // namespace

WebViewItem::WebViewItem(QQuickItem *parent)
    : QQuickItem(parent), m_available(WebViewLibrary::instance()->isAvailable()) {
    setFlag(ItemHasContents, true);
    setFlag(ItemAcceptsInputMethod, true);
    setFlag(ItemIsFocusScope, true);
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    m_tapInputPanelTimer.setSingleShot(true);
    m_tapInputPanelTimer.setInterval(kTapInputPanelDelayMs);
    connect(&m_tapInputPanelTimer, &QTimer::timeout, this, &WebViewItem::onTapInputPanelTimeout);
}

WebViewItem::~WebViewItem() {
    destroyView();
}

void WebViewItem::registerQmlTypes() {
    qmlRegisterType<WebViewItem>("UC.WebView", 1, 0, "WebView");
}

void WebViewItem::setInputPanelHookForTest(std::function<void(bool)> hook) {
    inputPanelHook() = std::move(hook);
}

// ---- callbacks (GUI thread, from the GLib main context) -------------------------------------------------------

const uc_webview_callbacks &WebViewItem::callbacks() {
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

void WebViewItem::onFrameReady(const uc_webview_frame &frame) {
    Q_ASSERT(QThread::currentThread() == thread());
    ++m_framesReceived;
    if (void *replaced = m_frames.push(frame)) {
        // the scene graph never saw the replaced frame (item not rendered in between)
        releaseFrame(replaced);
    }
    emit frameStatsChanged();
    update();
}

void WebViewItem::onLoadChanged(int event, const char *uri) {
    const QUrl url = QUrl::fromEncoded(uri ? QByteArray(uri) : QByteArray());
    UC_WEBVIEW_TRACE_IF() << "load " << loadEventName(event) << " " << loggableUrl(url);
    switch (event) {
        case UC_WEBVIEW_LOAD_STARTED:
            m_loadFailed = false;
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
            qCWarning(lcWebView()) << "Load failed:" << loggableUrl(url);
            emit loadFailed(url);
            break;
        default:
            break;
    }
    updateNavigationHistory();
}

void WebViewItem::onWebProcessTerminated(int reason) {
    qCWarning(lcWebView()) << "Web process terminated, reason" << reason << "while showing" << loggableUrl(m_url);
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
    UC_WEBVIEW_TRACE_IF() << "ime_focus " << focused << " hints 0x" << Qt::hex << hints << Qt::dec << " purpose "
                          << purpose << " cursor " << cursor << (surrounding ? " with surrounding text" : "")
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
    m_imeInhibitOsk = (hints & UC_WEBVIEW_IME_HINT_INHIBIT_OSK) != 0;
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
        // inputmode="none", or an element the page focused by script without a user gesture (WebPageGLib.cpp:
        // 216-217); a user tap on it sends the hints again without the flag, and the keyboard follows below
        qCDebug(lcWebView()) << "Page inhibits the on-screen keyboard";
        return;
    }
    if (!wasEnabled || !QGuiApplication::inputMethod()->isVisible()) {
        requestInputPanel(true);
    }
}

void WebViewItem::resetImeState() {
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
    // input method state again only when it differs (WebPageGLib.cpp:212-223, FocusController.cpp:1209-1211 for a
    // click into the element that already has the focus) and InputMethodFilter::setState notifies focus-in only
    // when an element gains it (InputMethodFilter.cpp:91-102). After the keyboard's hide key a tap on the same
    // field therefore produces no ime_focus callback. Qt's TextInput re-opens the input panel on a press into the
    // field that already has the focus (qquicktextinput.cpp:1901-1913); this does the same for the page's field.
    // Which element the tap hits is only known to WebKit, so the request waits until WebKit had the time to turn
    // the tap into a click: a tap on a button or the page background moves the page's focus away, ime_focus(0)
    // arrives first and resetImeState() cancels the request, no keyboard flashes up.
    if (!m_imeEnabled || !hasActiveFocus() || m_imeInhibitOsk) {
        return;
    }
    UC_WEBVIEW_TRACE_IF() << "tap with a focused editable element: keyboard request in " << kTapInputPanelDelayMs
                          << " ms unless the page moves its focus";
    m_tapInputPanelTimer.start();
}

void WebViewItem::onTapInputPanelTimeout() {
    if (!m_imeEnabled || !hasActiveFocus() || m_imeInhibitOsk) {
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
    UC_WEBVIEW_TRACE_IF() << "ime_surrounding cursor " << cursor << " anchor " << anchor << " length "
                          << (text ? static_cast<int>(strlen(text)) : 0);
    m_surroundingText = QString::fromUtf8(text ? text : "");
    m_cursorPosition = cursor;
    m_anchorPosition = anchor;
    updateInputMethod(Qt::ImSurroundingText | Qt::ImCursorPosition | Qt::ImAnchorPosition);
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
    node->setRect(boundingRect());
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
    releaseFrames();

    if (window) {
        // frameSwapped is emitted on the render thread: the queued connection hands it to the GUI thread
        m_frameSwappedConnection =
            connect(window, &QQuickWindow::frameSwapped, this, &WebViewItem::onFrameSwapped, Qt::QueuedConnection);
        // the display-off path hides the window (src/system/power.h): unmap so the web process stops rendering
        m_windowVisibleConnection = connect(window, &QWindow::visibleChanged, this, [this] { updateMapped(); });
    }
    updateSize();
    updateMapped();
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

    m_devicePixelRatio = window()->effectiveDevicePixelRatio();
    m_viewSize = devicePixelSize();
    m_view = lib().create(m_viewSize.width(), m_viewSize.height(), &callbacks(), this);
    if (!m_view) {
        qCWarning(lcWebView()) << "uc_webview_new failed";
        return;
    }
    qCDebug(lcWebView()) << "Web view created" << m_viewSize << "at scale" << m_devicePixelRatio;

    lib().set_scale(m_view, m_devicePixelRatio);
    lib().set_mapped(m_view, m_mapped ? 1 : 0);
    lib().set_focused(m_view, hasActiveFocus() ? 1 : 0);
    applyBackgroundColor();

    if (!m_pendingHtml.isEmpty()) {
        const QByteArray base = m_pendingBaseUrl.toEncoded();
        lib().load_html(m_view, m_pendingHtml.toUtf8().constData(), base.isEmpty() ? nullptr : base.constData());
    } else if (!m_url.isEmpty()) {
        lib().load_uri(m_view, m_url.toEncoded().constData());
    }
}

void WebViewItem::destroyView() {
    m_tapInputPanelTimer.stop();
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
    qCWarning(lcWebView()).noquote().nospace() << "Not loading " << what << " " << loggableUrl(url) << ": " << reason;
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
        UC_WEBVIEW_TRACE_IF() << "load_uri " << loggableUrl(url);
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

void WebViewItem::reload() {
    if (m_view) {
        lib().reload(m_view);
    }
}

void WebViewItem::stop() {
    if (m_view) {
        lib().stop(m_view);
    }
}

void WebViewItem::goBack() {
    if (m_view) {
        lib().go_back(m_view);
    }
}

void WebViewItem::goForward() {
    if (m_view) {
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
        UC_WEBVIEW_TRACE_IF() << "load_html " << html.size() << " chars, base " << loggableUrl(baseUrl);
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
    const QPointF delta = to - from;
    return std::hypot(delta.x(), delta.y()) > tapDistance();
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
        // once a move is more than WPE_SETTING_DRAG_THRESHOLD (8 px) away from the press position
        const QPointF start = m_touchStarts.value(id, devicePos);
        const QPointF delta = devicePos - start;
        qCInfo(lcWebView()).nospace() << "trace " << static_cast<const void *>(this) << ": touch slot " << slot
                                      << " (qt id " << id << ") " << touchPhaseName(phase) << " " << devicePos.x()
                                      << "," << devicePos.y() << " ms " << time << " moved "
                                      << std::hypot(delta.x(), delta.y()) << " px, " << m_touchSlots.size()
                                      << " down";
    }
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
    UC_WEBVIEW_TRACE_IF() << "key 0x" << Qt::hex << keysym << Qt::dec << (pressed ? " down" : " up") << " scancode "
                          << event->nativeScanCode() << " ms " << eventTime(event);
    lib().key(m_view, keysym, event->nativeScanCode(), pressed ? 1 : 0, modifiersForEvent(event->modifiers()),
              eventTime(event));
    event->accept();
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
    UC_WEBVIEW_TRACE_IF() << "input method event: commit " << event->commitString().size() << " chars, preedit "
                          << event->preeditString().size() << " chars, replace " << event->replacementStart() << "/"
                          << event->replacementLength() << (m_imeEnabled ? "" : " (ignored, no editable element)");
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
