// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QMetaObject>
#include <QQuickItem>
#include <QRectF>
#include <QString>
#include <QUrl>

#include "uc_webview.h"
#include "webviewframequeue.h"

class QQuickWindow;

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
    // a load failed; `loading` turns false with the following finished event
    void loadFailed(const QUrl &url);

 protected:
    void      geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void      itemChange(ItemChange change, const ItemChangeData &value) override;
    QSGNode  *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void      releaseResources() override;
    void      touchEvent(QTouchEvent *event) override;
    void      mousePressEvent(QMouseEvent *event) override;
    void      mouseMoveEvent(QMouseEvent *event) override;
    void      mouseReleaseEvent(QMouseEvent *event) override;
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
    void   ensureView();
    void   destroyView();
    void   updateMapped();
    void   updateSize();
    void   updateNavigationHistory();
    void   applyBackgroundColor();
    void   releaseFrames();
    void   releaseFrame(void *handle);
    void   keyEvent(QKeyEvent *event, bool pressed);
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

    QMetaObject::Connection m_frameSwappedConnection;
    QMetaObject::Connection m_windowVisibleConnection;
};

}  // namespace webview
}  // namespace uc
