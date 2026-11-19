// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The zoom level of a WebView after VOLUME+/- and the magnification of a pinch (docs/webview.md, "Zoom with VOLUME"):
// shown at once, while the fingers move and the page renders it, and for about a second after the last key press
// (hold()) or pinch; then it fades out. A render that takes longer than spinnerDelay adds the app's spinner next to the
// level. Placed by its user over the web view, at the top: the web view test page and the fullscreen web view of a web
// view entity.
//
// The WebView is an Item here, without the UC.WebView import: the fullscreen web view's screen is part of every build.

import QtQuick 2.15

Rectangle {
    id: zoomLabel
    objectName: "zoomLabel"

    // the WebView whose zoom, magnification, pinch and render wait are shown
    property Item webView: null
    // The spinner next to the zoom level appears only when the page takes longer than this to render a zoom step
    // (webView.zoomRendering): a quick step does not flicker.
    property int spinnerDelay: 150

    // A key press zoomed, also at the first or last level, where nothing changes: the label tells the level that is
    // set for about a second.
    function hold() {
        zoomLabelHold.restart();
    }

    // a press or pinch within the last second, a pinch, or the page still renders the level
    readonly property bool shown: zoomLabelHold.running
                                  || (webView !== null && (webView.zoomRendering || webView.pinching))
    // the page renders the level for longer than spinnerDelay: the spinner shows
    property bool renderTakesLong: false

    width: zoomRow.implicitWidth + 40
    height: zoomRow.implicitHeight + 20
    radius: ui.cornerRadiusSmall
    color: colors.bg
    opacity: 0
    visible: opacity > 0

    onShownChanged: {
        if (shown) {
            zoomLabelFade.stop();
            opacity = 1;
        } else {
            zoomLabelFade.start();
        }
    }

    Connections {
        target: zoomLabel.webView
        ignoreUnknownSignals: true

        // the label stays a second after a pinch, like after a key press
        function onPinchingChanged() {
            if (!zoomLabel.webView.pinching) {
                zoomLabelHold.restart();
            }
        }

        function onZoomRenderingChanged() {
            if (zoomLabel.webView.zoomRendering) {
                zoomSpinnerTimer.restart();
            } else {
                zoomSpinnerTimer.stop();
                zoomLabel.renderTakesLong = false;
            }
        }
    }

    Timer {
        id: zoomSpinnerTimer
        interval: zoomLabel.spinnerDelay
        onTriggered: zoomLabel.renderTakesLong = true
    }

    Row {
        id: zoomRow
        anchors.centerIn: parent
        spacing: 12

        Text {
            id: zoomText
            objectName: "zoomText"
            anchors.verticalCenter: parent.verticalCenter
            color: colors.textPrimary
            // a value with its unit: the value role
            font: fonts.value()
            // "125 %", "2.0×" or both ("125 % · 2.0×"): the page zoom (VOLUME) and the magnification of a pinch are
            // separate (docs/webview.md, "Pinch zoom")
            text: {
                if (!zoomLabel.webView) {
                    return "";
                }
                const zoom = Math.round(zoomLabel.webView.zoom * 100) + " %";
                if (Math.abs(zoomLabel.webView.magnification - 1) < 0.05) {
                    return zoom;
                }
                const magnification = zoomLabel.webView.magnification.toFixed(1) + "×";
                return Math.abs(zoomLabel.webView.zoom - 1) < 0.005 ? magnification : zoom + " · " + magnification;
            }
        }

        // the app's command-in-progress spinner (StatusBar.qml, BaseTitle.qml)
        Image {
            id: zoomSpinner
            objectName: "zoomSpinner"
            anchors.verticalCenter: parent.verticalCenter
            width: 28; height: 28
            source: "qrc:/images/loader_small.png"
            fillMode: Image.PreserveAspectFit
            visible: zoomLabel.renderTakesLong

            RotationAnimation on rotation {
                running: zoomSpinner.visible
                loops: Animation.Infinite
                from: 0; to: 360
                duration: 1200
            }
        }
    }

    Timer {
        id: zoomLabelHold
        interval: 1000
    }

    NumberAnimation {
        id: zoomLabelFade
        target: zoomLabel; property: "opacity"; to: 0; duration: 300; easing.type: Easing.OutExpo
    }
}
