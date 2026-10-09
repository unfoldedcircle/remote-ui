// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/components" as Components

// One row of a menu (docs/design-system.md sections 5 to 7): 80 px, an optional 60 px icon box, the label, and at
// the end a value, a badge, a check mark for the current value, or a chevron for a row that opens another screen.
// The selection spans 8 px to width - 8, the content keeps the 20 px gutter. Pressed inverts the row.
// It only renders and reports taps: the host list keeps the selection and the keys.
Item {
    id: menuRow
    width: parent ? parent.width : 0
    height: 80
    implicitHeight: 80

    Layout.fillWidth: true

    property string icon: ""
    property alias text: label.text
    // shown at the end in the help role, e.g. the current language of a picker row
    property string value: ""
    // a small count at the end of the label, e.g. integrations that need attention; empty hides it
    property string badge: ""
    // the badge asks for attention, e.g. an available software update
    property bool badgeAlert: false
    // the row opens another screen
    property bool chevron: false
    // the row is the current value of a choice
    property bool checked: false

    property bool selected: false
    // "ring" in settings and set-up flows, "fill" on the main UI (popup menus)
    property string selectionStyle: "ring"
    // popup menu rows use the smaller menu row role
    property bool popup: false
    // a destructive action, e.g. "Delete page"
    property bool destructive: false

    // set when the row is part of a keypad focus chain: OK (Return) while focused activates it like a tap
    property bool keypadActivatable: false

    readonly property bool pressed: mouseArea.pressed
    // on the selection fill everything is drawn in the primary text colour (design system section 6)
    readonly property bool onFill: selection.shown && selectionStyle === "fill"
    readonly property color contentColor: pressed ? colors.bg
                                                  : destructive && !onFill ? colors.red : colors.textPrimary
    readonly property color secondaryColor: pressed ? colors.bg : onFill ? colors.textPrimary : colors.textSecondary

    signal clicked()

    Keys.onReturnPressed: {
        if (!keypadActivatable || !enabled) {
            event.accepted = false;
            return;
        }

        Haptic.play(Haptic.Click);
        menuRow.clicked();
        event.accepted = true;
    }

    opacity: enabled ? 1 : 0.4

    Components.Selectable {
        id: selection
        anchors { leftMargin: 8; rightMargin: 8 }
        selected: menuRow.selected
        style: menuRow.selectionStyle
    }

    Rectangle {
        anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
        radius: ui.cornerRadiusSmall
        color: colors.textPrimary
        visible: menuRow.pressed
    }

    Components.Icon {
        id: iconItem
        visible: menuRow.icon !== ""
        icon: menuRow.icon
        size: 60
        color: menuRow.contentColor
        anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
    }

    Text {
        id: label
        width: Math.min(implicitWidth, trailing.x - x - (badgeItem.visible ? badgeItem.width + 10 : 0) - 10)
        anchors { left: iconItem.visible ? iconItem.right : parent.left; leftMargin: iconItem.visible ? 10 : 20;
                  verticalCenter: parent.verticalCenter }
        elide: Text.ElideRight
        maximumLineCount: 1
        color: menuRow.contentColor
        font: menuRow.popup ? fonts.menuRow() : fonts.label()
    }

    Rectangle {
        id: badgeItem
        visible: menuRow.badge !== ""
        width: Math.max(height, badgeText.implicitWidth + 16)
        height: 32
        radius: height / 2
        color: menuRow.badgeAlert ? colors.red : colors.surfaceRaised
        anchors { left: label.right; leftMargin: 10; verticalCenter: parent.verticalCenter }

        Text {
            id: badgeText
            anchors.centerIn: parent
            text: menuRow.badge
            color: colors.textPrimary
            font: fonts.caption()
        }
    }

    // the end of the row: value, check mark and chevron, right-aligned at the gutter
    Row {
        id: trailing
        spacing: 10
        anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }

        Text {
            visible: menuRow.value !== ""
            width: Math.min(implicitWidth, menuRow.width / 2)
            anchors.verticalCenter: parent.verticalCenter
            text: menuRow.value
            elide: Text.ElideRight
            maximumLineCount: 1
            color: menuRow.secondaryColor
            font: fonts.help()
        }

        Components.Icon {
            visible: menuRow.checked
            icon: "uc:check"
            size: 60
            color: menuRow.contentColor
            anchors.verticalCenter: parent.verticalCenter
        }

        Components.Icon {
            visible: menuRow.chevron
            icon: "uc:chevron-right"
            size: 40
            color: menuRow.secondaryColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Components.HapticMouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: menuRow.clicked()
    }
}
