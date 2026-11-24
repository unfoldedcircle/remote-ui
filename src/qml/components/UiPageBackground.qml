// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

// The full-screen background of the current activity or remote page: the image the user set, under a scrim of the
// strength the user set. A new background is loaded into the hidden layer and cross-faded in once it is decoded.
Item {
    id: pageBackground

    // the `background` of the current page as the core sends it; undefined or null without one
    property var background

    readonly property var target: resource.getPageBackground(background)

    // the layer that is shown or fading in
    property int front: 0

    function isSame(a, b) {
        return a.source === b.source && a.scale === b.scale && a.align === b.align && a.dim === b.dim;
    }

    function update() {
        const frontLayer = layers.itemAt(front);
        const backLayer = layers.itemAt(1 - front);

        if (!frontLayer || !backLayer || isSame(target, frontLayer.values)) {
            return;
        }

        backLayer.values = target;
        showWhenReady();
    }

    // Swaps the layers once the hidden one holds the latest background, decoded or without an image.
    function showWhenReady() {
        const frontLayer = layers.itemAt(front);
        const backLayer = layers.itemAt(1 - front);

        if (frontLayer && backLayer && backLayer.ready && isSame(backLayer.values, target)
                && !isSame(frontLayer.values, target)) {
            front = 1 - front;
        }
    }

    onTargetChanged: update()

    Repeater {
        id: layers
        model: 2

        onItemAdded: pageBackground.update()

        Item {
            id: layer

            property var values: ({ source: "", scale: "fill", align: "center", dim: 0.4 })
            readonly property bool ready: values.source === "" || image.status === Image.Ready
                                          || image.status === Image.Error

            width: pageBackground.width; height: pageBackground.height
            opacity: index === pageBackground.front ? 1 : 0
            visible: opacity > 0

            Behavior on opacity {
                OpacityAnimator { duration: 200 }
            }

            onReadyChanged: pageBackground.showWhenReady()

            Image {
                id: image
                anchors.fill: parent
                source: layer.values.source
                asynchronous: true
                cache: true
                fillMode: layer.values.scale === "fit" ? Image.PreserveAspectFit
                                                       : layer.values.scale === "none" ? Image.Pad
                                                                                       : Image.PreserveAspectCrop
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: layer.values.align === "top" ? Image.AlignTop
                                                                : layer.values.align === "bottom" ? Image.AlignBottom
                                                                                                  : Image.AlignVCenter
            }

            Rectangle {
                anchors.fill: parent
                color: colors.bg
                opacity: layer.values.dim
                visible: layer.values.source !== ""
            }
        }
    }
}
