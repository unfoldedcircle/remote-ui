// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 ICON COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - icon
 - color
 - size
**/

import QtQuick 2.15

Item {
    id: iconBase
    width: size
    height: size

    property string icon
    property string suffix
    property string _icon: resource.getIcon(icon, suffix.toLowerCase())
    property int size: 80
    property alias color: iconText.color

    Image {
        id: image
        fillMode: Image.PreserveAspectFit
        antialiasing: true
        asynchronous: true
        cache: false
        width: parent.width
        height: parent.height
        sourceSize.width: width
        sourceSize.height: height
        source: _icon.includes("data:") || _icon.includes("file:") ? _icon : ""
        visible: image.source != ""
        anchors.centerIn: parent
    }

    Text {
        id: iconText
        text: _icon
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
        color: colors.offwhite
        font { family: "icons"; pixelSize: size }
        visible: image.source == ""
    }
}
