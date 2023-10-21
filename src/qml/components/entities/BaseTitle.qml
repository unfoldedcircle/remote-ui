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
        width: parent.width - 200
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        elide: Text.ElideRight
        color: colors.offwhite
        opacity: iconOpen.opacity
        anchors { left: iconOpen.right; leftMargin: 10; verticalCenter: parent.verticalCenter; }
        font: fonts.primaryFont(24, "Medium")
        lineHeight: 0.8
    }
}
