// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

// The full-screen backgrounds of the pages of an activity or remote screen: the image the user set for a page, or the
// default background of the user interface, under a scrim of the strength the user set. Each background moves with its
// page while the pages move; neighbouring pages with the same image share one background that stays still. The runs of
// pages and every position come from Resources. A new configuration is built in the hidden strip and shown once the
// background of the current page is decoded.
Item {
    id: pageBackground

    // the pages of the user interface and its default background, as the core sends them
    property var pages
    property var defaultBackground

    // the page view: its offset, number of pages and current page
    property real offset
    property int count
    property int currentIndex

    readonly property real position: resource.pageBackgroundPosition(offset, count)
    readonly property var target: resource.getPageBackgroundRuns(pages ? pages : [], defaultBackground)

    // the strip that is shown
    property int front: 0

    function update() {
        const backStrip = strips.itemAt(1 - front);

        if (backStrip) {
            backStrip.layout = target;
            showWhenReady();
        }
    }

    // Swaps the strips once the hidden one holds the latest runs and the background of the current page is decoded.
    function showWhenReady() {
        const frontStrip = strips.itemAt(front);
        const backStrip = strips.itemAt(1 - front);

        if (frontStrip && backStrip && backStrip.layout === target && backStrip.ready) {
            front = 1 - front;
            frontStrip.layout = null;
        }
    }

    onTargetChanged: update()

    Repeater {
        id: strips
        model: 2

        onItemAdded: pageBackground.update()

        Item {
            id: strip

            // a result of getPageBackgroundRuns(), or null
            property var layout: null
            readonly property var runs: layout ? layout.runs : []
            readonly property int currentRun: {
                const run = layout ? layout.runOfPage[pageBackground.currentIndex] : undefined;
                return run === undefined ? 0 : run;
            }
            readonly property bool ready: {
                if (!layout || runItems.count !== runs.length) {
                    return false;
                }
                const current = runItems.itemAt(currentRun);
                return runs.length === 0 || (current !== null && current.ready);
            }

            width: pageBackground.width; height: pageBackground.height
            visible: index === pageBackground.front

            onReadyChanged: pageBackground.showWhenReady()

            Repeater {
                id: runItems
                model: strip.runs

                Item {
                    id: run

                    // copied once, so that the bindings evaluated per frame read plain properties
                    readonly property int first: modelData.first
                    readonly property int length: modelData.length
                    readonly property string source: modelData.source
                    readonly property string scale: modelData.scale
                    readonly property string align: modelData.align
                    readonly property var dims: modelData.dims
                    readonly property real dim: modelData.dim
                    readonly property bool blend: modelData.blend

                    readonly property real runOffset: resource.pageBackgroundRunOffset(first, length,
                                                                                       pageBackground.position,
                                                                                       pageBackground.count)
                    readonly property bool loaded: resource.pageBackgroundRunLoaded(index, strip.currentRun,
                                                                                    strip.runs.length)
                    readonly property bool ready: source === "" || image.status === Image.Ready
                                                  || image.status === Image.Error

                    x: runOffset * width
                    width: pageBackground.width; height: pageBackground.height
                    visible: runOffset > -1 && runOffset < 1

                    Image {
                        id: image
                        anchors.fill: parent
                        source: run.loaded ? run.source : ""
                        asynchronous: true
                        cache: true
                        fillMode: run.scale === "fit" ? Image.PreserveAspectFit
                                                      : run.scale === "none" ? Image.Pad : Image.PreserveAspectCrop
                        horizontalAlignment: Image.AlignHCenter
                        verticalAlignment: run.align === "top" ? Image.AlignTop
                                                               : run.align === "bottom" ? Image.AlignBottom
                                                                                        : Image.AlignVCenter
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: colors.bg
                        opacity: run.blend && run.visible ? resource.pageBackgroundRunDim(run.dims, run.first,
                                                                                          pageBackground.position,
                                                                                          pageBackground.count)
                                                          : run.dim
                        visible: run.source !== ""
                    }
                }
            }
        }
    }
}
