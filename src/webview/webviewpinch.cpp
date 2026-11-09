// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewpinch.h"

#include <QtGlobal>

#include <cmath>

namespace uc {
namespace webview {

namespace {

constexpr double kScaleEpsilon = 1e-9;
constexpr double kOffsetEpsilon = 1e-6;

}  // namespace

ViewTransform ViewTransform::scaledAround(double s, const QPointF &c) {
    ViewTransform t;
    t.scale = s;
    t.offset = c * (1.0 - s);
    return t;
}

ViewTransform ViewTransform::then(const ViewTransform &next) const {
    ViewTransform t;
    t.scale = next.scale * scale;
    t.offset = offset * next.scale + next.offset;
    return t;
}

ViewTransform ViewTransform::inverted() const {
    ViewTransform t;
    t.scale = 1.0 / scale;
    t.offset = -offset / scale;
    return t;
}

bool ViewTransform::isIdentity() const {
    return std::abs(scale - 1.0) < kScaleEpsilon && std::abs(offset.x()) < kOffsetEpsilon &&
           std::abs(offset.y()) < kOffsetEpsilon;
}

bool ViewTransform::fuzzyEquals(const ViewTransform &other) const {
    return std::abs(scale - other.scale) < 1e-6 && std::abs(offset.x() - other.offset.x()) < 1e-3 &&
           std::abs(offset.y() - other.offset.y()) < 1e-3;
}

NodeGeometry nodeGeometry(const ViewTransform &t, const QSizeF &itemSize, qreal ratio, const QSize &textureSize) {
    NodeGeometry geometry;
    const QSizeF view = itemSize * ratio;  // view (device) pixels
    if (view.isEmpty() || textureSize.isEmpty() || !(t.scale > 0)) {
        return geometry;
    }
    const QRectF viewRect(QPointF(0, 0), view);
    const QRectF drawn(t.offset, view * t.scale);
    const QRectF visible = drawn.intersected(viewRect);
    if (visible.isEmpty()) {
        return geometry;
    }
    // the visible part in the frame's own view pixels, then in texture pixels (the frame fills the view; during a
    // resize the texture can still have the old size)
    const QRectF frame((visible.topLeft() - t.offset) / t.scale, visible.size() / t.scale);
    const qreal  sx = textureSize.width() / view.width();
    const qreal  sy = textureSize.height() / view.height();
    geometry.sourceRect = QRectF(frame.x() * sx, frame.y() * sy, frame.width() * sx, frame.height() * sy);
    geometry.rect = QRectF(visible.topLeft() / ratio, visible.size() / ratio);
    return geometry;
}

void PinchZoom::reset(double magnification) {
    m_reference = magnification > 0 ? magnification : 1.0;
    m_applied = m_reference;
    m_transform = ViewTransform();
    m_committed = ViewTransform();
    m_beginTransform = ViewTransform();
    m_pinching = false;
    m_holding = false;
    m_waitsForFrame = false;
}

double PinchZoom::clampScale(double s) const {
    // the magnification at the pinch's begin; a value outside the range (a history step restored it) is not exceeded
    const double base = m_reference * m_beginTransform.scale;
    const double lo = qMin(kMinMagnification, base);
    const double hi = qMax(kMaxMagnification, base);
    if (!(s > 0)) {
        s = 1.0;
    }
    return qBound(lo / base, s, hi / base);
}

void PinchZoom::begin(const QPointF &centre) {
    m_beginTransform = m_transform;
    m_anchor = centre;
    m_pinching = true;
}

void PinchZoom::update(double scale) {
    if (!m_pinching) {
        return;
    }
    m_transform = m_beginTransform.then(ViewTransform::scaledAround(clampScale(scale), m_anchor));
}

PinchZoom::Commit PinchZoom::commitFor(const ViewTransform &relative) const {
    Commit commit;
    if (std::abs(relative.scale - 1.0) < kMinChange) {
        return commit;
    }
    commit.valid = true;
    commit.magnification = m_applied * relative.scale;
    // the point the relative picture keeps in place: q = scale * q + offset
    commit.focal = relative.offset / (1.0 - relative.scale);
    return commit;
}

PinchZoom::Commit PinchZoom::end() {
    if (!m_pinching) {
        return Commit();
    }
    m_pinching = false;
    if (m_holding) {
        return Commit();  // after the running render (renderFinished)
    }
    // relative to what WebKit has (or will have once the frame after the last render arrives)
    const Commit commit = commitFor(m_committed.inverted().then(m_transform));
    if (!commit.valid) {
        m_transform = m_committed;  // too small to re-render the page for: snap back
    }
    return commit;
}

void PinchZoom::cancel() {
    if (!m_pinching) {
        return;
    }
    m_pinching = false;
    m_transform = m_beginTransform;
}

void PinchZoom::committed(const Commit &commit) {
    m_applied = commit.magnification;
    m_committed = m_transform;
    m_holding = true;
    m_waitsForFrame = false;
}

void PinchZoom::commitRefused() {
    m_transform = m_committed;
}

PinchZoom::Commit PinchZoom::renderFinished() {
    if (!m_holding) {
        return Commit();
    }
    m_holding = false;
    if (!m_pinching) {
        // pinches that ended while the page rendered go to the library now, together
        const Commit commit = commitFor(m_committed.inverted().then(m_transform));
        if (commit.valid) {
            return commit;
        }
        m_transform = m_committed;
    }
    m_waitsForFrame = true;
    return Commit();
}

void PinchZoom::rebase() {
    if (!m_waitsForFrame) {
        return;
    }
    // the new frame shows the committed picture: a point r of the old reference frame is at committed(r) in it
    const ViewTransform undo = m_committed.inverted();
    m_transform = undo.then(m_transform);
    if (m_pinching) {
        m_beginTransform = undo.then(m_beginTransform);
    }
    m_reference = m_applied;
    m_committed = ViewTransform();
    m_waitsForFrame = false;
}

}  // namespace webview
}  // namespace uc
