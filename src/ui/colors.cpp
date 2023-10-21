// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "colors.h"

namespace uc {
namespace ui {

Colors::Colors(const QString &baseColor, QObject *parent) : QObject(parent) {
    generateColorPalette(QColor(baseColor));
}

Colors::~Colors() {}

void Colors::generateColorPalette(QColor primaryColor) {
    m_baseColor = QColor(primaryColor);
    emit baseChanged();

    m_normalisedBaseColor.setHsv(m_baseColor.hsvHue(), 70, 200);

    m_dark.setHsv(m_baseColor.hsvHue(), 200, 22);
    emit darkChanged();

    m_medium.setHsv(m_baseColor.hsvHue(), 200, 35);
    emit mediumChanged();

    m_light.setHsv(m_baseColor.hsvHue(), 40, 120);
    emit lightChanged();

    m_highlight.setHsv(m_baseColor.hsvHue(), 160, 200);
    emit highlightChanged();

    m_inactive = QColor("#606060");
    emit inactiveChanged();

    m_primaryButton.setHsv(m_baseColor.hsvHue(), m_baseColor.hslSaturation() / 2, 90);
    emit primaryButtonChanged();

    m_secondaryButton.setHsv(m_baseColor.hsvHue(), m_baseColor.hslSaturation() / 2, 30);
    emit secondaryButtonChanged();
}

}  // namespace ui
}  // namespace uc
