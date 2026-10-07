// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

// A key and its value (docs/design-system.md sections 4 and 7): the key in the help role, the value in the value
// role, a divider below. On one line the key keeps its width and the value takes the rest and elides; a long value
// (stacked) goes under the key and wraps. 14 px vertical padding, 20 px gutter.
Item {
    id: keyValueRow
    width: parent ? parent.width : 0
    height: implicitHeight
    implicitHeight: (stacked ? valueText.y + valueText.height : Math.max(keyText.height, valueText.height))
                    + 2 * padding + (showDivider ? divider.height : 0)

    Layout.fillWidth: true

    property alias key: keyText.text
    property alias value: valueText.text
    property bool stacked: false
    property bool showDivider: true

    readonly property int gutter: 20
    readonly property int padding: 14

    Text {
        id: keyText
        x: keyValueRow.gutter
        y: keyValueRow.padding
        width: keyValueRow.stacked ? keyValueRow.width - 2 * keyValueRow.gutter
                                   : Math.min(implicitWidth, (keyValueRow.width - 2 * keyValueRow.gutter) * 0.6)
        elide: Text.ElideRight
        maximumLineCount: 1
        color: colors.textSecondary
        font: fonts.help()
    }

    Text {
        id: valueText
        x: keyValueRow.stacked ? keyValueRow.gutter : keyText.x + keyText.width + 20
        y: keyValueRow.stacked ? keyText.y + keyText.height + 4 : keyValueRow.padding
        width: keyValueRow.stacked ? keyValueRow.width - 2 * keyValueRow.gutter
                                   : keyValueRow.width - keyValueRow.gutter - x
        horizontalAlignment: keyValueRow.stacked ? Text.AlignLeft : Text.AlignRight
        wrapMode: keyValueRow.stacked ? Text.WrapAtWordBoundaryOrAnywhere : Text.NoWrap
        elide: keyValueRow.stacked ? Text.ElideNone : Text.ElideRight
        maximumLineCount: keyValueRow.stacked ? 6 : 1
        color: colors.textPrimary
        font: fonts.value()
    }

    Components.Divider {
        id: divider
        visible: keyValueRow.showDivider
        anchors.bottom: parent.bottom
    }
}
