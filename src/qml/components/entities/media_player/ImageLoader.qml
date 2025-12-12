// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Item {
    id: imageLoader

    height: image2.implicitHeight
    clip: true

    Behavior on height {
        enabled: imageLoader.shrinkHeight
        NumberAnimation { easing.type: Easing.OutBack; duration: 300 }
    }

    property string url: ""
    property string prevUrl: ""
    property bool aspectFit: false
    property bool shrinkHeight: false
    property bool alignCentered: false

    property alias image1: image1

    signal done()

    onUrlChanged: {
        loadingDelay.stop();
        loader.opacity = 0;

        if (url == prevUrl) {
            return;
        }

        if (url == "") {
            image1.source = "";
            image2.source = "";
            prevUrl = "";
            return;
        }

        image2.opacity = 0;
        image2.source = url;
        prevUrl = url;
    }

    Image {
        id: image1
        width: parent.width
        height: parent.height
        anchors.top: parent.top
        verticalAlignment: alignCentered ? Image.AlignVCenter : Image.AlignTop
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
                console.error("Failed to load image into image 1");
            }
        }
    }

    Image {
        id: image2
        width: parent.width
        height: parent.height
        anchors.top: parent.top
        verticalAlignment: alignCentered ? Image.AlignVCenter : Image.AlignTop
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
                loadingDelay.restart();
                console.debug("Loading image");
            }

            if (image2.status === Image.Ready || image2.status === Image.Error || image2.status === Image.Null) {
                loadingDelay.stop();
                loader.opacity = 0;
            }

            if (image2.status == Image.Ready) {
                image2.opacity = 1;

                if (imageLoader.shrinkHeight && !imageLoader.aspectFit) {
                    imageLoader.height = image2.paintedHeight;
                }

                console.debug("Image loaded");
            }

            if (image2.status == Image.Error && image2.source != "") {
                image2.source = "";
                prevUrl = "";
                console.error("Failed to load image into image 2");
            }
        }

        onOpacityChanged: {
            if (image2.opacity == 1) {
                if (image1.source != url) {
                    image1.source = url;
                }
            }
        }
    }

    Timer {
        id: loadingDelay
        interval: 500
        repeat: false
        onTriggered: loader.opacity = 1
    }

    Rectangle {
        id: loader
        width: 20; height: 20
        radius: width/2
        color: colors.offwhite
        anchors.centerIn: parent
        transformOrigin: Item.Center
        opacity: 0
        visible: opacity > 0

        onOpacityChanged: {
            if (loader.opacity == 1)
                integrationLoadinganimation.start();
            else
                integrationLoadinganimation.stop();
        }

        SequentialAnimation {
            id: integrationLoadinganimation
            running: false
            loops: Animation.Infinite

            NumberAnimation { target: loader; properties: "width, height"; to: 2; easing.type: Easing.OutExpo; duration: 600  }
            PauseAnimation { duration: 300 }
            NumberAnimation { target: loader; properties: "width, height"; to: 20; easing.type: Easing.OutExpo; duration: 600  }
        }
    }
}
