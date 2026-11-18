// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace uc {
namespace webview {

/**
 * A uniform scale with a translation in view pixels: a point p of the frame on screen shows at p * scale + offset.
 * The pinch's picture of the WebView item (docs/webview.md, "Pinch zoom").
 */
struct ViewTransform {
    double  scale = 1.0;
    QPointF offset;

    // scale by s around the point c: c + s * (p - c)
    static ViewTransform scaledAround(double s, const QPointF &c);

    QPointF       map(const QPointF &p) const { return p * scale + offset; }
    // first this, then next
    ViewTransform then(const ViewTransform &next) const;
    ViewTransform inverted() const;
    bool          isIdentity() const;
    bool          fuzzyEquals(const ViewTransform &other) const;
};

/**
 * Where to draw the frame and which part of it: the WebViewNode's rect (item pixels) and source rect (texture
 * pixels) for a frame of textureSize pixels that fills an item of itemSize (item pixels, ratio device pixels each)
 * under transform t (view pixels = device pixels). The part of the scaled frame outside the item is cut off, so no
 * clip node is needed (QSGSimpleTextureNode::setSourceRect takes texture pixels, qsgsimpletexturenode.cpp:62-89).
 * An empty rect: nothing of the frame is visible.
 */
struct NodeGeometry {
    QRectF rect;
    QRectF sourceRect;
};
NodeGeometry nodeGeometry(const ViewTransform &t, const QSizeF &itemSize, qreal ratio, const QSize &textureSize);

/**
 * The pinch zoom of a WebViewItem, without the library and without Qt Quick (unit-tested in
 * test/webview/test_webview_pinch.cpp). All in view pixels.
 *
 * The library reports a two-finger pinch: BEGIN at the fingers' midpoint, UPDATE with the scale since BEGIN, END or
 * CANCEL. While the fingers move the item scales the frame it shows on the GPU around the point the pinch began at
 * (transform()); at END it asks the library for the magnification once (a Commit) and keeps showing its scaled frame
 * (holding()) until the page has rendered the new magnification (renderFinished()). The first frame after that is
 * at the new magnification: rebase() makes it the reference, and the transform is the identity again.
 *
 * - The pinch zooms around the point where it began: a two-finger pan during the pinch does not move the page. The
 *   library sets the magnification with that point as its focal point, so the sharp frame matches the scaled one.
 * - The magnification the user sees is clamped to kMinMagnification..kMaxMagnification (no pinch below 1: page zoom
 *   with VOLUME- gives an overview, a page smaller than the view would only show an empty border). A magnification
 *   outside the range (restored by a history step) is kept and only not exceeded further.
 * - Pinches while a magnification renders change the picture at once and go to the library together when the render
 *   is done (like the item's zoom steps: WebKit would re-render for every one).
 * - A pinch that changed the magnification by less than kMinChange is no commit: the picture snaps back.
 */
class PinchZoom {
 public:
    static constexpr double kMinMagnification = 1.0;
    static constexpr double kMaxMagnification = 4.0;
    static constexpr double kMinChange = 0.01;

    struct Commit {
        bool    valid = false;
        double  magnification = 1.0;  // for uc_webview_set_magnification()
        QPointF focal;                // view pixels: the content there stays there
    };

    // The magnification WebKit has, from the library (a new view: 1; a navigation or history step:
    // magnification_changed). Forgets the pinch, the hold and the picture.
    void reset(double magnification);

    // what the user sees: the reference frame's magnification times the picture's scale
    double magnification() const { return m_reference * m_transform.scale; }
    // what the library was asked for last (WebKit's state once everything rendered)
    double appliedMagnification() const { return m_applied; }

    bool pinching() const { return m_pinching; }
    // a commit renders: the item shows the reference frame scaled and takes no new frames
    bool holding() const { return m_holding; }
    // the render is done: the next frame replaces the reference frame (rebase())
    bool waitsForFrame() const { return m_waitsForFrame; }

    // the picture of the reference frame
    const ViewTransform &transform() const { return m_transform; }

    void begin(const QPointF &centre);
    // scale: since BEGIN, as the library reports it
    void update(double scale);
    // END: the commit to make now (valid), or none (nothing changed, or a render runs: renderFinished() returns it)
    Commit end();
    // CANCEL: back to the picture before the pinch
    void cancel();

    // The library took the commit (uc_webview_set_magnification() returned 0) and a render request runs.
    void committed(const Commit &commit);
    // The library refused it: the picture goes back to what was committed before.
    void commitRefused();
    // The render of the last commit ended (DONE, TIMEOUT or CANCELLED: the next frame shows what the page has). Returns
    // the commit of pinches made meanwhile, if any (then the hold goes on with committed()).
    Commit renderFinished();
    // The first frame after renderFinished() is on screen: it shows the committed state, the picture is relative to it
    // from now on.
    void rebase();

 private:
    Commit commitFor(const ViewTransform &relative) const;
    double clampScale(double s) const;

    double        m_reference = 1.0;  // magnification of the frame the picture transforms
    double        m_applied = 1.0;
    ViewTransform m_transform;        // the picture
    ViewTransform m_committed;        // the picture WebKit will have rendered once the commits are done
    ViewTransform m_beginTransform;   // the picture when the pinch began
    QPointF       m_anchor;           // where the pinch began
    bool          m_pinching = false;
    bool          m_holding = false;
    bool          m_waitsForFrame = false;
};

}  // namespace webview
}  // namespace uc
