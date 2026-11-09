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

uint32_t eventTime(const QInputEvent *event) {
    return static_cast<uint32_t>(event->timestamp());
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
}

WebViewItem::~WebViewItem() {
    destroyView();
}

void WebViewItem::registerQmlTypes() {
    qmlRegisterType<WebViewItem>("UC.WebView", 1, 0, "WebView");
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
    QInputMethod *inputMethod = QGuiApplication::inputMethod();

    if (!focused) {
        if (!m_imeEnabled) {
            return;
        }
        m_imeEnabled = false;
        m_preeditActive = false;
        m_surroundingText.clear();
        m_cursorPosition = -1;
        m_anchorPosition = -1;
        updateInputMethod(Qt::ImEnabled | Qt::ImHints | Qt::ImQueryInput);
        inputMethod->hide();
        return;
    }

    const bool wasEnabled = m_imeEnabled;
    m_imeEnabled = true;
    m_imeHints = inputMethodHintsForIme(hints, purpose);
    if (surrounding) {
        m_surroundingText = QString::fromUtf8(surrounding);
        m_cursorPosition = cursor;
        m_anchorPosition = cursor;
    }
    // the page can only focus an editable element while the view is focused, so this is normally a no-op
    forceActiveFocus(Qt::OtherFocusReason);
    updateInputMethod(Qt::ImEnabled | Qt::ImHints | Qt::ImQueryInput);

    if (hints & UC_WEBVIEW_IME_HINT_INHIBIT_OSK) {
        qCDebug(lcWebView()) << "Page inhibits the on-screen keyboard";
        return;
    }
    if (!wasEnabled || !inputMethod->isVisible()) {
        inputMethod->show();
    }
}

void WebViewItem::onImeSurrounding(const char *text, int cursor, int anchor) {
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
    if (m_imeEnabled) {
        m_imeEnabled = false;
        QGuiApplication::inputMethod()->hide();
    }
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

void WebViewItem::setUrl(const QUrl &url) {
    m_pendingHtml.clear();
    if (url != m_url) {
        m_url = url;
        emit urlChanged();
    }
    if (m_view && !url.isEmpty()) {
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
    m_pendingHtml = html;
    m_pendingBaseUrl = baseUrl;
    if (m_view) {
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
    if (event->type() == QEvent::TouchBegin) {
        forceActiveFocus(Qt::MouseFocusReason);
    }
    const bool     cancel = event->type() == QEvent::TouchCancel;
    const uint32_t time = eventTime(event);
    for (const QTouchEvent::TouchPoint &point : event->touchPoints()) {
        const int phase = cancel ? UC_WEBVIEW_TOUCH_CANCEL : touchPhaseForState(point.state());
        if (phase < 0) {
            continue;  // stationary
        }
        const QPointF pos = devicePoint(point.pos());
        lib().touch(m_view, point.id(), phase, pos.x(), pos.y(), time);
    }
    event->accept();
}

void WebViewItem::mousePressEvent(QMouseEvent *event) {
    const int button = pointerButtonForQt(event->button());
    if (!m_view || button == 0) {
        event->ignore();
        return;
    }
    forceActiveFocus(Qt::MouseFocusReason);
    const QPointF pos = devicePoint(event->localPos());
    lib().pointer_button(m_view, button, 1, pos.x(), pos.y(), modifiersForEvent(event->modifiers()), eventTime(event));
    event->accept();
}

void WebViewItem::mouseMoveEvent(QMouseEvent *event) {
    if (!m_view) {
        event->ignore();
        return;
    }
    const QPointF pos = devicePoint(event->localPos());
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
    lib().pointer_button(m_view, button, 0, pos.x(), pos.y(), modifiersForEvent(event->modifiers()), eventTime(event));
    event->accept();
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
