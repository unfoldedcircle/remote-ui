// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Item {
    id: imageLoader

    height: image2.implicitHeight
    clip: true

    Behavior on height {

        NumberAnimation { easing.type: Easing.OutBack; duration: 300 }
    }

    property string url: ""
    property string prevUrl: ""
    property bool aspectFit: false

    property alias image1: image1

    signal done()

    onUrlChanged: {
        if (url == "") {
            image1.source = "";
            image2.source = "";
        } else if (url != prevUrl) {
            image2.opacity = 0;
            image2.source = url;
            prevUrl = url;
        }
    }

    Image {
        id: image1
        width: parent.width
        height: imageLoader.aspectFit ? undefined : parent.height
        anchors.top: parent.top
        verticalAlignment: Image.AlignTop
        fillMode: imageLoader.aspectFit ? Image.PreserveAspectFit : Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        sourceSize.width: parent.width
        sourceSize.height: parent.height

        onStatusChanged: {
            if (image1.status == Image.Ready) {
                image2.opacity = 0;
                imageLoader.done();
            }

            if (image1.status == Image.Error) {
                image1.source = "";
                image1.source = imageLoader.url;
            }
        }
    }

    Image {
        id: image2
        width: parent.width
        height: imageLoader.aspectFit ? undefined : parent.height
        anchors.top: parent.top
        verticalAlignment: Image.AlignTop
        fillMode: imageLoader.aspectFit ? Image.PreserveAspectFit : Image.PreserveAspectCrop
        asynchronous: true
        opacity: 0
        cache: false
        sourceSize.width: parent.width
        sourceSize.height: parent.height

        Behavior on opacity {
            NumberAnimation { duration: 800; easing.type: Easing.OutExpo }
        }

        onStatusChanged: {
            if (image2.status == Image.Loading) {
                loader.opacity = 1;
            }

            if (image2.status == Image.Ready) {
                loader.opacity = 0;
                image2.opacity = 1;
            }

            if (image2.status == Image.Error) {
                image2.source = "";
                image2.source = imageLoader.url;
            }
        }

        onOpacityChanged: {
            if (image2.opacity == 1) {
                image1.source = url;
            }
        }
    }

    Rectangle {
        id: loader
        width: 20; height: 20
        radius: width/2
        color: colors.offwhite
        anchors.centerIn: parent
        transformOrigin: Item.Center
        opacity: 0

        SequentialAnimation {
            id: integrationLoadinganimation
            running: loader.opacity == 1
            loops: Animation.Infinite

            NumberAnimation { target: loader; properties: "width, height"; to: 2; easing.type: Easing.OutExpo; duration: 600  }
            PauseAnimation { duration: 300 }
            NumberAnimation { target: loader; properties: "width, height"; to: 20; easing.type: Easing.OutExpo; duration: 600  }
        }
    }
}
