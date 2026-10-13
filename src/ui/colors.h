// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QObject>

namespace uc {
namespace ui {

class Colors : public QObject {
    Q_OBJECT

    Q_PROPERTY(QColor base MEMBER m_baseColor NOTIFY baseChanged)

    Q_PROPERTY(QColor white MEMBER white CONSTANT)
    Q_PROPERTY(QColor offwhite MEMBER offwhite CONSTANT)
    Q_PROPERTY(QColor black MEMBER black CONSTANT)
    Q_PROPERTY(QColor transparent MEMBER transparent CONSTANT)

    Q_PROPERTY(QColor light MEMBER m_light NOTIFY lightChanged)
    Q_PROPERTY(QColor medium MEMBER m_medium NOTIFY mediumChanged)
    Q_PROPERTY(QColor dark MEMBER m_dark NOTIFY darkChanged)

    Q_PROPERTY(QColor highlight MEMBER m_highlight NOTIFY highlightChanged)

    Q_PROPERTY(QColor green MEMBER green CONSTANT)
    Q_PROPERTY(QColor red MEMBER red CONSTANT)
    Q_PROPERTY(QColor orange MEMBER orange CONSTANT)
    Q_PROPERTY(QColor yellow MEMBER yellow CONSTANT)
    Q_PROPERTY(QColor blue MEMBER blue CONSTANT)

    Q_PROPERTY(QColor remoteGreen MEMBER remoteGreen CONSTANT)
    Q_PROPERTY(QColor remoteRed MEMBER remoteRed CONSTANT)
    Q_PROPERTY(QColor remoteYellow MEMBER remoteYellow CONSTANT)
    Q_PROPERTY(QColor remoteBlue MEMBER remoteBlue CONSTANT)

    // Design system tokens (docs/design-system.md, ADR 0019). A token that is an existing colour under a new name
    // reads the same member, so both names stay in step while the screens move to the token names. An old name
    // goes once no screen uses it: inactiveText, primaryButton and secondaryButton are gone.
    Q_PROPERTY(QColor bg MEMBER black CONSTANT)
    Q_PROPERTY(QColor textPrimary MEMBER offwhite CONSTANT)
    Q_PROPERTY(QColor textSecondary MEMBER m_light NOTIFY lightChanged)
    Q_PROPERTY(QColor textDisabled MEMBER m_inactive NOTIFY inactiveChanged)
    Q_PROPERTY(QColor textOnButton MEMBER white CONSTANT)
    Q_PROPERTY(QColor surface MEMBER m_dark NOTIFY darkChanged)
    Q_PROPERTY(QColor surfaceRaised MEMBER m_medium NOTIFY mediumChanged)
    Q_PROPERTY(QColor surfaceSelected MEMBER surfaceSelected CONSTANT)
    Q_PROPERTY(QColor divider MEMBER divider CONSTANT)
    Q_PROPERTY(QColor focusRing MEMBER m_highlight NOTIFY highlightChanged)
    Q_PROPERTY(QColor buttonPrimary MEMBER m_primaryButton NOTIFY primaryButtonChanged)
    Q_PROPERTY(QColor redPressed MEMBER redPressed CONSTANT)

 public:
    explicit Colors(const QString &baseColor = "#000000", QObject *parent = nullptr);
    ~Colors();

    QColor white = QColor("#FFFFFF");
    QColor offwhite = QColor("#d0d0d0");
    QColor black = QColor("#000000");
    QColor transparent = QColor("#00000000");

    QColor green = QColor("#769990");
    QColor red = QColor("#ff3e54");
    QColor orange = QColor("#FF7241");
    QColor yellow = QColor("#ffe78a");
    QColor blue = QColor("#335266");

    QColor remoteGreen = QColor("#198240");
    QColor remoteRed = QColor("#e61e25");
    QColor remoteYellow = QColor("#f9c940");
    QColor remoteBlue = QColor("#416bb2");

    // provisional until it is checked on both remotes (docs/design-system.md, Q-3)
    QColor surfaceSelected = QColor("#595959");
    QColor divider = QColor("#3A3A3A");
    QColor redPressed = QColor("#FF0D2A");

    Q_INVOKABLE void generateColorPalette(QColor primaryColor);

 signals:
    void baseChanged();
    void lightChanged();
    void mediumChanged();
    void darkChanged();
    void highlightChanged();
    void inactiveChanged();
    void primaryButtonChanged();

 private:
    QColor m_baseColor;
    QColor m_normalisedBaseColor;

    QColor m_dark;
    QColor m_medium;
    QColor m_light;

    QColor m_highlight;
    QColor m_inactive;

    QColor m_primaryButton;
};
}  // namespace ui
}  // namespace uc
