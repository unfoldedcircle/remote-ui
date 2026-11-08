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
    property string _icon: icon ? resource.getIcon(icon, suffix.toLowerCase()) : ""
    // an SVG icon file: the largest size with its own aspect ratio inside the box, invalid for any other icon
    readonly property size _svgSize: _icon.indexOf("file:") === 0 ? resource.getIconSize(icon, width, height)
                                                                   : Qt.size(-1, -1)
    readonly property bool _hasSvgSize: _svgSize.width > 0 && _svgSize.height > 0
    property int size: 5
    property bool useLayer: false
    property alias color: iconText.color

    // An image icon that exists but cannot be decoded shows the placeholder glyph instead.
    readonly property bool imageFailed: image.status === Image.Error

    layer.enabled: useLayer
    layer.smooth: useLayer

    Image {
        id: image
        fillMode: Image.PreserveAspectFit
        antialiasing: true
        asynchronous: true
        cache: source.toString().indexOf("data:") !== 0
        width: iconBase._hasSvgSize ? iconBase._svgSize.width : parent.width
        height: iconBase._hasSvgSize ? iconBase._svgSize.height : parent.height
        // decoded at the size it is drawn: an SVG is rasterized at this size, never at the size it declares and
        // never at a larger size that covers the box
        sourceSize.width: width
        sourceSize.height: height
        source: _icon.includes("data:") || _icon.includes("file:") ? _icon : ""
        visible: image.source != "" && !iconBase.imageFailed
        anchors.centerIn: parent

        onStatusChanged: {
            if (status === Image.Error) {
                console.warn("Cannot load icon:", iconBase.icon);
            }
        }
    }

    Text {
        id: iconText
        text: iconBase.imageFailed ? resource.getIconPlaceholder() : _icon
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
        color: colors.offwhite
        font { family: fonts.iconFamily; pixelSize: Math.round(size / 2); }
        visible: image.source == "" || iconBase.imageFailed
        renderType: Text.NativeRendering
    }
}
