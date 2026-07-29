// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Item {
    id: imageLoader

    height: image.implicitHeight
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
    property bool failed: false
    property int retryCount: 0
    property int maxRetries: 2
    property int retryDelay: 1000

    property alias image1: image
    readonly property bool isDataUrl: url.indexOf("data:") === 0
    readonly property bool isProviderUrl: url.indexOf("image://media-art/") === 0

    signal done()

    onUrlChanged: {
        loadingDelay.stop();
        retryTimer.stop();
        loader.opacity = 0;
        failed = false;
        retryCount = 0;

        if (url == prevUrl) {
            return;
        }

        if (url == "") {
            image.opacity = 0;
            image.source = "";
            prevUrl = "";
            return;
        }

        image.opacity = 0;
        image.source = url;
        prevUrl = url;
    }

    Image {
        id: image
        width: parent.width
        height: parent.height
        anchors.top: parent.top
        verticalAlignment: alignCentered ? Image.AlignVCenter : Image.AlignTop
        fillMode: imageLoader.aspectFit ? Image.PreserveAspectFit : Image.PreserveAspectCrop
        asynchronous: true
        cache: !imageLoader.isDataUrl && !imageLoader.isProviderUrl
        opacity: 0
        sourceSize.width: parent.width
        sourceSize.height: parent.height

        Behavior on opacity {
            NumberAnimation { duration: 800; easing.type: Easing.OutExpo }
        }

        onStatusChanged: {
            if (image.status == Image.Loading) {
                loadingDelay.restart();
            }

            if (image.status === Image.Ready || image.status === Image.Error || image.status === Image.Null) {
                loadingDelay.stop();
                loader.opacity = 0;
            }

            if (image.status == Image.Ready) {
                failed = false;
                image.opacity = 1;

                if (imageLoader.shrinkHeight && !imageLoader.aspectFit) {
                    imageLoader.height = image.paintedHeight;
                }

                imageLoader.done();
            }

            if (image.status == Image.Error && image.source != "") {
                if (retryCount < maxRetries) {
                    retryCount += 1;
                    image.source = "";
                    retryTimer.restart();
                } else {
                    image.opacity = 0;
                    image.source = "";
                    prevUrl = "";
                    failed = true;
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

    Timer {
        id: retryTimer
        interval: retryDelay
        repeat: false
        onTriggered: {
            if (url != "") {
                image.source = url;
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
