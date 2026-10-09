// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 BUTTON COMPONENT

 The one button of the UI (docs/design-system.md sections 5 to 7): 80 px tall, the button type role, and
 one of three variants. Do not set `color` from the outside, pick a variant instead.

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width
 - variant: "primary" (default), "secondary" (Cancel, Skip and other alternatives) or "destructive"
 - text
 - highlight
 - trigger
**/

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: button
    width: title.implicitWidth + 40; height: 80
    radius: ui.cornerRadiusSmall
    // disabled: the whole control at 0.4, it stays reachable so the screen can say why it is off
    opacity: enabled ? 1 : 0.4

    // pressed inverts the button; a pressed destructive button only gets the stronger red
    color: {
        if (mouseArea.pressed) {
            return variant === "destructive" ? colors.redPressed : colors.textPrimary;
        }

        switch (variant) {
        case "secondary":
            return colors.surface;
        case "destructive":
            return colors.red;
        default:
            return colors.buttonPrimary;
        }
    }
    border { width: variant === "secondary" ? 2 : 0; color: colors.divider }

    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    signal triggered()

    property string variant: "primary"
    property alias text: title.text
    property bool highlight: activeFocus && ui.keyNavigationActive
    property var trigger

    function activate() {
        if (!button.enabled) {
            return;
        }

        Haptic.play(Haptic.Click);
        if (button.trigger) {
            button.trigger();
        }
        button.triggered();
    }

    // a control reached with the d-pad while the on-screen keyboard is up would sit under it
    onActiveFocusChanged: {
        if (activeFocus && typeof keyboard !== "undefined" && keyboard.active) {
            keyboard.hide();
        }
    }

    // DPAD_MIDDLE maps to Key_Return: activate the button when it holds the keyboard focus.
    Keys.onReturnPressed: {
        button.activate();
        event.accepted = true;
    }

    Keys.onEnterPressed: {
        button.activate();
        event.accepted = true;
    }

    Components.Selectable {
        selected: button.highlight
        radius: button.radius
    }

    Text {
        id: title
        width: button.width - 20
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
        color: mouseArea.pressed && button.variant !== "destructive" ? colors.bg
                                                                      : button.variant === "secondary" ? colors.textPrimary
                                                                                                       : colors.textOnButton
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: button
        font: fonts.button()
    }

    Components.HapticMouseArea {
        id: mouseArea
        anchors.fill: button
        onClicked: {
            if (button.trigger) {
                button.trigger();
            }
            button.triggered();
        }
    }
}
