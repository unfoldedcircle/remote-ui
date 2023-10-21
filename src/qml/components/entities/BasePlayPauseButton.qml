// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 PLAY/PAUSE BUTTON COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - checked
 - trigger
**/

import QtQuick 2.15
import Haptic 1.0

import "qrc:/components" as Components

Components.HapticMouseArea {
    id: mouseArea

    property bool checked: false
    property var trigger

    onClicked: {
        mouseArea.trigger();
    }

    Components.Icon {
        id: icon
        color: colors.offwhite
        icon: checked ? "uc:play" : "uc:pause"
        anchors.centerIn: parent
        size: 40
        transformOrigin: Item.Center
    }
}
