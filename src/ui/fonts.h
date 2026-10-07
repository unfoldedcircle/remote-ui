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
    Q_PROPERTY(qreal proseLineHeight READ proseLineHeight CONSTANT)

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

    /**
     * Type roles of the design system (docs/design-system.md section 4). New and reworked text uses a role
     * instead of a pixel size. No role is smaller than minimumPixelSize, and the light weight only exists
     * from minimumDisplaySize on. Poppins is for reading, Space Mono only for values.
     */
    static constexpr int minimumPixelSize = 22;
    static constexpr int minimumDisplaySize = 56;

    /// Title bar of pages, sheets and dialogs.
    Q_INVOKABLE QFont title() { return primaryFont(28, "Medium"); }
    /// Section heading inside a page ("Known networks"), drawer title.
    Q_INVOKABLE QFont heading() { return primaryFont(26, "Medium"); }
    /// Menu rows, setting labels, tile names, list rows.
    Q_INVOKABLE QFont label() { return primaryFont(30); }
    /// Rows of a popup menu, including its Close row.
    Q_INVOKABLE QFont menuRow() { return primaryFont(28); }
    /// Release notes, legal texts, driver instructions, dialog text. The Text sets lineHeight: fonts.proseLineHeight.
    Q_INVOKABLE QFont prose() { return primaryFont(26); }
    /// Description under a setting, states, subtitles, the key of a key/value row. Drawn in textSecondary.
    Q_INVOKABLE QFont help() { return primaryFont(26); }
    /// Timestamps, badges, tab labels: the smallest text there is, never weaker than textSecondary.
    Q_INVOKABLE QFont caption() { return primaryFont(minimumPixelSize); }
    /// Versions, IP and MAC addresses, times, units, PIN digits, URLs.
    Q_INVOKABLE QFont value() { return secondaryFont(26); }
    /// Button labels.
    Q_INVOKABLE QFont button() { return primaryFont(26, "Medium"); }
    /// Large numbers and states: sensor values, entity states, volume. Light weight, at least minimumDisplaySize.
    Q_INVOKABLE QFont display(int size = 90) { return primaryFont(qMax(size, minimumDisplaySize), "Light"); }

    /// Line height of the prose role.
    qreal proseLineHeight() const { return 1.3; }

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
