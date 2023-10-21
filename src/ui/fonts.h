// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFont>
#include <QObject>

namespace uc {
namespace ui {

class Fonts : public QObject {
    Q_OBJECT

    Q_PROPERTY(QFont statusbarClock READ statusbarClock CONSTANT)

 public:
    explicit Fonts(QObject* parent = nullptr) : QObject(parent) {}
    ~Fonts() {}

    Q_INVOKABLE QFont primaryFont(int size = 30, const QString& style = "Normal") {
        QFont font = QFont("Poppins");
        font.setPixelSize(size);
        font.setStyleName(style);
        return font;
    }

    Q_INVOKABLE QFont primaryFontCapitalized(int size = 30, const QString& style = "Normal") {
        QFont font = primaryFont(size, style);
        font.setCapitalization(QFont::AllUppercase);
        return font;
    }

    Q_INVOKABLE QFont secondaryFont(int size = 24, const QString& style = "Normal") {
        QFont font = QFont("Space Mono");
        font.setPixelSize(size);
        font.setStyleName(style);
        return font;
    }

    Q_INVOKABLE QFont secondaryFontCapitalized(int size = 30, const QString& style = "Normal") {
        QFont font = secondaryFont(size, style);
        font.setCapitalization(QFont::AllUppercase);
        return font;
    }

    QFont statusbarClock() {
        QFont font = primaryFont(24);
        font.setLetterSpacing(QFont::PercentageSpacing, 110);
        return font;
    }
};
}  // namespace ui
}  // namespace uc
