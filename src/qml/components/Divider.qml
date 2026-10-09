// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

// The 2 px separator between sections and between rows that are not cards (docs/design-system.md section 5),
// inset by the 20 px gutter. In a layout it fills the width minus the gutter, elsewhere it sizes itself.
Rectangle {
    property int inset: 20

    x: inset
    width: parent ? parent.width - 2 * inset : 0
    height: 2
    implicitHeight: 2
    color: colors.divider

    Layout.fillWidth: true
    Layout.leftMargin: inset
    Layout.rightMargin: inset
}
