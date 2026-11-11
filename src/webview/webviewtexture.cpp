// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewtexture.h"

#include <QImage>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickWindow>

#include "../logging.h"

namespace uc {
namespace webview {

namespace {

// glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) of GL_OES_EGL_image; the image is a void *.
typedef void(QOPENGLF_APIENTRYP ImageTargetTexture2DFn)(GLenum target, void *image);

// Resolved once through the scene graph's context: QOpenGLContext::getProcAddress() is eglGetProcAddress() on
// the EGL platforms (qeglplatformcontext.cpp:451). The address does not depend on the context.
ImageTargetTexture2DFn imageTargetTexture2D(QOpenGLContext *context) {
    static ImageTargetTexture2DFn function = nullptr;
    static bool                   resolved = false;
    if (!resolved) {
        resolved = true;
        function = reinterpret_cast<ImageTargetTexture2DFn>(context->getProcAddress("glEGLImageTargetTexture2DOES"));
        if (!function) {
            qCWarning(lcWebView()) << "glEGLImageTargetTexture2DOES is not available, DMA-BUF frames cannot be shown";
        }
    }
    return function;
}

}  // namespace

WebViewNode::WebViewNode() {
    // the node deletes the QSGTexture it shows; the GL texture behind an EGL image is ours (see the destructor)
    setOwnsTexture(true);
    setFiltering(QSGTexture::Linear);
    setTextureCoordinatesTransform(flipFromEnvironment() ? QSGSimpleTextureNode::MirrorVertically
                                                         : QSGSimpleTextureNode::NoTransform);
}

WebViewNode::~WebViewNode() {
    if (m_glTexture == 0) {
        return;
    }
    // node destructors run on the render thread with the context current (cleanupNodes() in the sync phase)
    if (QOpenGLContext *context = QOpenGLContext::currentContext()) {
        context->functions()->glDeleteTextures(1, &m_glTexture);
    } else {
        qCWarning(lcWebView()) << "No GL context while deleting the web view texture, leaking texture" << m_glTexture;
    }
}

bool WebViewNode::flipFromEnvironment() {
    static const bool flip = qEnvironmentVariableIntValue("UC_WEBVIEW_FLIP") == 1;
    return flip;
}

bool WebViewNode::setFrame(QQuickWindow *window, const uc_webview_frame &frame) {
    const bool ok = frame.egl_image ? bindEglImage(window, frame) : uploadPixels(window, frame);
    if (ok) {
        // same texture object, new content: make sure the renderer re-reads it
        markDirty(QSGNode::DirtyMaterial);
    }
    return ok;
}

bool WebViewNode::bindEglImage(QQuickWindow *window, const uc_webview_frame &frame) {
    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (!context) {
        qCWarning(lcWebView()) << "No current GL context in updatePaintNode";
        return false;
    }
    ImageTargetTexture2DFn targetTexture = imageTargetTexture2D(context);
    if (!targetTexture) {
        return false;
    }

    QOpenGLFunctions *gl = context->functions();
    if (m_glTexture == 0) {
        gl->glGenTextures(1, &m_glTexture);
        gl->glBindTexture(GL_TEXTURE_2D, m_glTexture);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        gl->glBindTexture(GL_TEXTURE_2D, m_glTexture);
    }
    // the texture's storage becomes the DMA-BUF of the frame, zero copies
    targetTexture(GL_TEXTURE_2D, frame.egl_image);
    gl->glBindTexture(GL_TEXTURE_2D, 0);

    const QSize size(frame.width, frame.height);
    if (!texture() || !m_eglTexture || m_textureSize != size) {
        // Wraps the GL texture id without owning it (qquickwindow.cpp:4953-4958, the non-RHI path); the QSGTexture
        // itself is owned by the node (setOwnsTexture) and replaced when the frame size changes.
        QSGTexture *sgTexture = window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture, &m_glTexture,
                                                                      0, size, QQuickWindow::TextureHasAlphaChannel);
        if (!sgTexture) {
            qCWarning(lcWebView()) << "createTextureFromNativeObject failed for a" << size << "frame";
            return false;
        }
        setTexture(sgTexture);
        setFiltering(QSGTexture::Linear);
        m_eglTexture = true;
        m_textureSize = size;
    }
    return true;
}

bool WebViewNode::uploadPixels(QQuickWindow *window, const uc_webview_frame &frame) {
    if (!frame.pixels || frame.stride <= 0) {
        return false;
    }
    // ARGB8888 little endian (B, G, R, A bytes), top row first: QImage::Format_ARGB32 on this platform. The copy
    // frees the node from the frame's lifetime; this path exists for a web process without a GPU only.
    const QImage image(static_cast<const uchar *>(frame.pixels), frame.width, frame.height, frame.stride,
                       QImage::Format_ARGB32);
    QSGTexture *sgTexture = window->createTextureFromImage(image.copy(), QQuickWindow::TextureHasAlphaChannel);
    if (!sgTexture) {
        return false;
    }
    setTexture(sgTexture);
    setFiltering(QSGTexture::Linear);
    m_eglTexture = false;
    m_textureSize = image.size();
    return true;
}

}  // namespace webview
}  // namespace uc
