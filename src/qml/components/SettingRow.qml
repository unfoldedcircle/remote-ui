// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

// One setting of a settings page (docs/design-system.md sections 5 to 7): the label, an optional value at the end
// of the label line, an optional help text, and the control, followed by a divider. The control is the child of the
// row: a switch or button sits at the end of the label line, a slider (controlBelow) under the texts. The section
// sizes to its content with 14 px vertical padding and keeps the 20 px gutter.
// It only lays out: the control keeps its id, its KeyNavigation links and its own selection ring.
Item {
    id: settingRow
    width: parent ? parent.width : 0
    height: implicitHeight
    implicitHeight: body.height + 2 * padding + (showDivider ? divider.height : 0)

    Layout.fillWidth: true

    property alias title: titleText.text
    property alias help: helpText.text
    // a value shown at the end of the label line, e.g. the position of the slider below
    property alias value: valueText.text
    property bool controlBelow: false
    property bool showDivider: true
    // room under a control below the texts for what it draws outside its own height, e.g. the end labels of a slider
    property int controlBottomSpace: 0

    default property alias control: slot.data

    readonly property int gutter: 20
    readonly property int padding: 14
    readonly property real titleHeight: titleText.text !== "" ? titleText.height : 0
    readonly property real headerHeight: controlBelow ? titleHeight : Math.max(titleHeight, slot.height)

    Item {
        id: body
        x: settingRow.gutter
        y: settingRow.padding
        width: settingRow.width - 2 * settingRow.gutter
        height: settingRow.controlBelow ? slot.y + slot.height + settingRow.controlBottomSpace
                                        : (helpText.visible ? helpText.y + helpText.height : settingRow.headerHeight)

        Text {
            id: titleText
            visible: text !== ""
            y: settingRow.controlBelow ? 0 : Math.max(0, (settingRow.headerHeight - height) / 2)
            width: body.width - (settingRow.controlBelow ? (valueText.visible ? valueText.width + 20 : 0) : slot.width + 20)
            wrapMode: Text.WordWrap
            color: colors.textPrimary
            font: fonts.label()
        }

        Text {
            id: valueText
            visible: text !== ""
            anchors { right: parent.right; baseline: titleText.baseline }
            color: colors.textPrimary
            font: fonts.value()
        }

        Text {
            id: helpText
            visible: text !== ""
            y: settingRow.headerHeight + (settingRow.titleHeight > 0 ? 6 : 0)
            width: body.width
            wrapMode: Text.WordWrap
            color: colors.textSecondary
            font: fonts.help()
        }

        Item {
            id: slot
            x: settingRow.controlBelow ? 0 : body.width - width
            y: settingRow.controlBelow ? (helpText.visible ? helpText.y + helpText.height + 14
                                                           : settingRow.headerHeight + (settingRow.titleHeight > 0 ? 14 : 0))
                                       : Math.max(0, (settingRow.headerHeight - height) / 2)
            width: settingRow.controlBelow ? body.width : childrenRect.width
            height: childrenRect.height
        }
    }

    Components.Divider {
        id: divider
        visible: settingRow.showDivider
        anchors.bottom: parent.bottom
    }
}
