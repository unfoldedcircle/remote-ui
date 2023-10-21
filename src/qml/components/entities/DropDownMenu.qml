// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Rectangle {
    id: dropDownMenu
    width: parent.width
    height: parent.height - title.height
    y: ui.height
    clip: true
    color: colors.dark
    radius: ui.cornerRadiusSmall

    property bool opened: y != ui.height

    function open() {
        dropDownMenu.y = title.height;
    }
    function close() {
        dropDownMenu.y = ui.height;
    }

    Behavior on y {
        NumberAnimation { duration: 300; easing.type: Easing.OutExpo }
    }

    MouseArea {
        anchors.fill: parent
    }
}
