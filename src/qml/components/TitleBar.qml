// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

// The title bar of pages, sheets and dialogs (docs/design-system.md sections 5 and 7): 80 px tall, an 80 x 80
// back target at the left or close target at the right edge, the title in the title role. The title is centred
// on the bar while it fits between the targets (the gutters, without a target), otherwise it starts after the
// target and elides.
// It only renders and reports taps: BACK and HOME stay with the screen's own navigation.
Item {
    id: titleBar
    width: parent ? parent.width : 0
    height: 80
    implicitHeight: 80

    Layout.fillWidth: true

    property alias text: titleText.text
    // "back": arrow at the left edge, "close": cross at the right edge, "": no target (onboarding)
    property string action: "back"
    // called when the target is tapped
    property var goBack
    // a screen whose end action is not "close" (the page selector's edit mode) sets its own icon and colour
    property string icon: action === "close" ? "uc:xmark" : "uc:arrow-left"
    property int iconSize: targetSize
    property color iconColor: colors.textPrimary

    signal actionTriggered()

    readonly property int targetSize: 80
    // the space kept free at each side of a centred title
    readonly property int sideSpace: action === "" ? 20 : targetSize
    readonly property bool centred: titleText.implicitWidth <= width - 2 * sideSpace

    // the bar covers content that scrolls under it
    Rectangle {
        anchors.fill: parent
        color: colors.bg
    }

    Item {
        id: actionTarget
        visible: titleBar.action !== ""
        width: titleBar.targetSize
        height: titleBar.targetSize
        x: titleBar.action === "close" ? titleBar.width - width : 0
        anchors.verticalCenter: parent.verticalCenter

        Components.Icon {
            anchors.centerIn: parent
            icon: titleBar.icon
            size: titleBar.iconSize
            color: titleBar.iconColor
        }

        Components.HapticMouseArea {
            anchors.fill: parent
            enabled: actionTarget.visible
            onClicked: {
                if (titleBar.goBack) {
                    titleBar.goBack();
                }
                titleBar.actionTriggered();
            }
        }
    }

    Text {
        id: titleText
        x: titleBar.centred ? titleBar.sideSpace : (titleBar.action === "back" ? titleBar.targetSize : 20)
        width: titleBar.centred ? titleBar.width - 2 * titleBar.sideSpace
                                : titleBar.width - (titleBar.action === "" ? 40 : titleBar.targetSize + 20)
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: titleBar.centred ? Text.AlignHCenter : Text.AlignLeft
        elide: Text.ElideRight
        maximumLineCount: 1
        color: colors.textPrimary
        font: fonts.title()
    }
}
