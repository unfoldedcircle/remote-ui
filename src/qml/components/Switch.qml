// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SWITCH COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - checked
 - _opacity
 - trigger
 - highlight
 - icon
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import Haptic 1.0

import "qrc:/components" as Components

Item {
    id: buttonContainer
    width: 90; height: 60

    property alias checked: button.checked
    property alias _opacity: button.opacity
    property bool highlight: activeFocus && ui.keyNavigationActive
    property var trigger
    // Shown inside the knob while checked. Defaults to the check mark used throughout the settings;
    // set to "" for a switch where a check mark would be misleading.
    property string icon: "uc:check"

    function activate() {
        Haptic.play(Haptic.Click);
        button.toggle();
        if (trigger) {
            trigger();
        }
    }

    // DPAD_MIDDLE maps to Key_Return. The inner QtQuick Switch never sees it: focus is held by this
    // container, and an unhandled key bubbles up the parent chain, not down to the child. Handle it
    // here so a switch reached with the d-pad can actually be toggled.
    Keys.onReturnPressed: {
        buttonContainer.activate();
        event.accepted = true;
    }

    Keys.onEnterPressed: {
        buttonContainer.activate();
        event.accepted = true;
    }

    Switch {
        id: button
        implicitHeight: buttonContainer.height; implicitWidth: buttonContainer.width

        onClicked: {
            buttonContainer.activate();
        }

        indicator: Rectangle {
            x: (button.checked ? buttonContainer.width / 6 : width / 4) + (button.visualPosition * (button.width - width - buttonContainer.width / 4)); y: (button.height - height) / 2
            width: buttonContainer.width / 2 - (button.checked ? 0 : buttonContainer.width / 8); height: width
            radius: buttonContainer.height / 2
            color: button.checked ? colors.offwhite : colors.light

            Behavior on x {
                enabled: !button.pressed
                SmoothedAnimation { velocity: 150 }
            }

            Components.Icon {
                icon: buttonContainer.icon
                color: colors.black
                size: buttonContainer.height / 2
                anchors.centerIn: parent
                visible: button.checked && buttonContainer.icon !== ""
            }
        }

        background: Rectangle {
            radius: buttonContainer.height / 2
            color: colors.medium
            border { width: 2; color: highlight ? colors.highlight : colors.transparent }
        }
    }
}
