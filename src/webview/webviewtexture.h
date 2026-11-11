// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSGSimpleTextureNode>
#include <QSize>
#include <QtGui/qopengl.h>

#include "uc_webview.h"

class QQuickWindow;

namespace uc {
namespace webview {

/**
 * The scene graph node of a WebViewItem: a textured rectangle whose texture is the current libucwebview frame.
 *
 * Everything here runs on the render thread with the scene graph's OpenGL context current: the node is created,
 * updated and deleted from QQuickItem::updatePaintNode() or by the scene graph's node cleanup
 * (QQuickWindowPrivate::cleanupNodes(), qquickwindow.cpp:3577-3582, called from the sync phase). It never calls
 * the library; the frame lifecycle is the item's business on the GUI thread.
 *
 * A DMA-BUF frame comes with an EGLImage already imported on Qt's EGLDisplay; the node binds it to its own GL
 * texture with glEGLImageTargetTexture2DOES and wraps that texture for the scene graph with
 * QQuickWindow::createTextureFromNativeObject() (Qt 5.14+). A shared memory frame (no EGLImage) is uploaded as
 * a QImage, the slow path for a web process without a GPU.
 */
class WebViewNode : public QSGSimpleTextureNode {
 public:
    WebViewNode();
    ~WebViewNode() override;

    // Show this frame; false when the GL context or the EGL image extension is missing (the node then has no
    // texture and must not be rendered).
    bool setFrame(QQuickWindow *window, const uc_webview_frame &frame);

    // UC_WEBVIEW_FLIP=1 mirrors the texture vertically (buffers with a bottom-left origin). Default off: WebKit's
    // DMA-BUF frames have their first row at the top, like a Wayland buffer without the y-invert flag
    // (qtwayland 5.15 linuxdmabufclientbufferintegration.cpp:489-491 maps only that flag to a mirrored texture).
    static bool flipFromEnvironment();

 private:
    bool bindEglImage(QQuickWindow *window, const uc_webview_frame &frame);
    bool uploadPixels(QQuickWindow *window, const uc_webview_frame &frame);

    GLuint m_glTexture = 0;
    QSize  m_textureSize;
    bool   m_eglTexture = false;  // the node's QSGTexture wraps m_glTexture
};

}  // namespace webview
}  // namespace uc
