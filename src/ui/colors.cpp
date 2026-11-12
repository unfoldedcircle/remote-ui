// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "colors.h"

namespace uc {
namespace ui {

Colors::Colors(const QString& baseColor, QObject* parent)
    : QObject(parent), m_selectionFill(selectionFillAt(SELECTION_FILL_DEFAULT_LEVEL)) {
    generateColorPalette(QColor(baseColor));
}

Colors::~Colors() {}

void Colors::generateColorPalette(QColor primaryColor) {
    m_baseColor = QColor(primaryColor);
    emit baseChanged();

    m_normalisedBaseColor.setHsv(m_baseColor.hsvHue(), 70, 200);

    // the values give the design system tokens for the default black base colour (docs/design-system.md)
    m_dark.setHsv(m_baseColor.hsvHue(), 200, 30);  // surface #1E1E1E
    emit darkChanged();

    m_medium.setHsv(m_baseColor.hsvHue(), 200, 44);  // surfaceRaised #2C2C2C
    emit mediumChanged();

    m_light.setHsv(m_baseColor.hsvHue(), 40, 160);  // textSecondary #A0A0A0
    emit lightChanged();

    m_highlight.setHsv(m_baseColor.hsvHue(), 160, 208);  // focusRing #D0D0D0
    emit highlightChanged();

    m_inactive = QColor("#7A7A7A");  // textDisabled
    emit inactiveChanged();

    m_primaryButton.setHsv(m_baseColor.hsvHue(), m_baseColor.hslSaturation() / 2, 90);  // buttonPrimary
    emit primaryButtonChanged();
}

QColor Colors::selectionFillAt(int level) const {
    level = qBound(0, level, SELECTION_FILL_MAX_LEVEL);
    if (level == 0) {
        return transparent;
    }

    auto step = [level](int channel) {
        return qRound(channel * level / static_cast<double>(SELECTION_FILL_MAX_LEVEL));
    };
    return QColor(step(surfaceSelected.red()), step(surfaceSelected.green()), step(surfaceSelected.blue()));
}

void Colors::setSelectionFillLevel(int level) {
    const QColor fill = selectionFillAt(level);
    if (fill != m_selectionFill) {
        m_selectionFill = fill;
        emit selectionFillChanged();
    }
}

}  // namespace ui
}  // namespace uc
