// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

// Keypad focus outline for a tappable row (a HapticMouseArea) in the dock / integration details.
Rectangle {
    anchors { fill: parent; leftMargin: -10; rightMargin: -10 }
    radius: ui.cornerRadiusSmall
    color: colors.transparent
    border {
        width: 2
        color: parent.activeFocus && ui.keyNavigationActive ? colors.highlight : colors.transparent
    }
}
