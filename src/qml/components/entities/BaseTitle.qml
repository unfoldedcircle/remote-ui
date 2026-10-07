// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components" as Components

Item {
    id: titleBase
    width: parent.width
    height: 80

    property alias icon: iconOpen.icon
    property alias suffix: iconOpen.suffix
    property alias title: titleOpen.text

    Components.Icon {
        id: iconOpen
        color: colors.offwhite
        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
        size: 70
    }

    Text {
        id: titleOpen
        // up to the status row, which grows with the icons it shows
        width: Math.min(parent.width - 200, statusCluster.x - x - 10)
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        // three lines fill the 80 px bar; a longer name is cut off with an ellipsis instead of spilling out
        maximumLineCount: 3
        elide: Text.ElideRight
        color: colors.offwhite
        opacity: iconOpen.opacity
        anchors { left: iconOpen.right; leftMargin: 10; verticalCenter: parent.verticalCenter; }
        font: fonts.primaryFont(24, "Medium")
        lineHeight: 0.8
    }

    // right-aligned status row: integration, wifi, battery, then the command-in-progress spinner; it
    // ends 60 px from the edge, clear of the close icon of the screen
    TitleStatus {
        id: statusCluster
        anchors { right: parent.right; rightMargin: 60; verticalCenter: parent.verticalCenter }
        integrationDisconnected: titleBase.parent ? titleBase.parent.integrationDisconnected === true : false
        commandInProgress: titleBase.parent && titleBase.parent.entityObj ? titleBase.parent.entityObj.commandInProgress : false
    }
}
