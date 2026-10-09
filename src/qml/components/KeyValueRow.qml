// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

// A key and its value (docs/design-system.md sections 4 and 7): the key in the help role, the value in the value
// role, a divider below. On one line the key keeps its width and the value takes the rest and elides; a long value
// (stacked) goes under the key and wraps. A value that has to be read in full (fullValue), such as a MAC or IP
// address, stays on the line while it fits next to the key and goes under the key otherwise, so it is never cut
// off in any language. 8 px vertical padding, denser than the selectable rows: a key/value row is read, not
// selected, and the About page fits its information and its first entry on the Remote 3. 20 px gutter.
Item {
    id: keyValueRow
    width: parent ? parent.width : 0
    height: implicitHeight
    implicitHeight: (isStacked ? valueText.y + valueText.height : Math.max(keyText.height, valueText.height))
                    + 2 * padding + (showDivider ? divider.height : 0)

    Layout.fillWidth: true

    property alias key: keyText.text
    property alias value: valueText.text
    property bool stacked: false
    property bool fullValue: false
    property bool showDivider: true

    readonly property int gutter: 20
    readonly property int padding: 8
    readonly property int spacing: 20

    // measured with the fonts of the two roles, not with the Text items, whose size depends on the layout
    readonly property bool isStacked: stacked || (fullValue && width > 0 && keyMetrics.advanceWidth(key) + spacing
                                                  + valueMetrics.advanceWidth(value) > width - 2 * gutter)

    FontMetrics {
        id: keyMetrics
        font: fonts.help()
    }

    FontMetrics {
        id: valueMetrics
        font: fonts.value()
    }

    Text {
        id: keyText
        x: keyValueRow.gutter
        y: keyValueRow.padding
        width: keyValueRow.isStacked ? keyValueRow.width - 2 * keyValueRow.gutter
                                     : Math.min(implicitWidth, (keyValueRow.width - 2 * keyValueRow.gutter) * 0.6)
        elide: Text.ElideRight
        maximumLineCount: 1
        color: colors.textSecondary
        font: fonts.help()
    }

    Text {
        id: valueText
        x: keyValueRow.isStacked ? keyValueRow.gutter : keyText.x + keyText.width + keyValueRow.spacing
        y: keyValueRow.isStacked ? keyText.y + keyText.height + 4 : keyValueRow.padding
        width: keyValueRow.isStacked ? keyValueRow.width - 2 * keyValueRow.gutter
                                     : keyValueRow.width - keyValueRow.gutter - x
        horizontalAlignment: keyValueRow.isStacked ? Text.AlignLeft : Text.AlignRight
        wrapMode: keyValueRow.isStacked ? Text.WrapAtWordBoundaryOrAnywhere : Text.NoWrap
        elide: keyValueRow.isStacked ? Text.ElideNone : Text.ElideRight
        maximumLineCount: keyValueRow.isStacked ? 6 : 1
        color: colors.textPrimary
        font: fonts.value()
    }

    Components.Divider {
        id: divider
        visible: keyValueRow.showDivider
        anchors.bottom: parent.bottom
    }
}
