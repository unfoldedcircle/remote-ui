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
    Q_PROPERTY(QString iconFamily READ iconFamily NOTIFY iconFamilyChanged)

 public:
    explicit Fonts(QObject* parent = nullptr) : QObject(parent) {}
    ~Fonts() {}

    /**
     * @brief The font family of the embedded icon font, set once at startup. It depends on
     * the embedded edition of the icon set, so QML must not hard code it (docs/icon-font.md).
     */
    QString iconFamily() const { return m_iconFamily; }

    void setIconFamily(const QString& family) {
        if (m_iconFamily != family) {
            m_iconFamily = family;
            emit iconFamilyChanged();
        }
    }

    Q_INVOKABLE QFont iconFont(int size = 30) {
        QFont font(m_iconFamily);
        font.setPixelSize(size);
        return font;
    }

    Q_INVOKABLE QFont primaryFont(int size = 30, const QString& style = "Normal") {
        QFont font = QFont("Poppins");
        font.setPixelSize(size);
        font.setStyleName(style);
        return font;
    }

    Q_INVOKABLE QFont primaryFontCapitalizedFirst(int size = 30, const QString& style = "Normal") {
        QFont font = primaryFont(size, style);
        font.setCapitalization(QFont::Capitalize);
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

    Q_INVOKABLE QFont secondaryFontCapitalizedFirst(int size = 30, const QString& style = "Normal") {
        QFont font = secondaryFont(size, style);
        font.setCapitalization(QFont::Capitalize);
        return font;
    }

    Q_INVOKABLE QFont secondaryFontCapitalized(int size = 30, const QString& style = "Normal") {
        QFont font = secondaryFont(size, style);
        font.setCapitalization(QFont::AllUppercase);
        return font;
    }

 signals:
    void iconFamilyChanged();

 public:
    QFont statusbarClock() {
        QFont font = primaryFont(24);
        font.setLetterSpacing(QFont::PercentageSpacing, 110);
        return font;
    }

 private:
    QString m_iconFamily;
};
}  // namespace ui
}  // namespace uc
