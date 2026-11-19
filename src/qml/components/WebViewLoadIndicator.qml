// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// What the page of a web view entity shows while it loads (docs/webview.md, "Load indicator"), over the WebView of
// WebViewPage.qml: in the fullscreen web view and in the web view widget of an activity page. The WebView decides
// what to show (loadIndicator, pageFailed); this draws it at the page's size:
// - the spinner with the load progress, centred, in a card like the zoom label (WebViewZoomLabel.qml), black with the
//   light text, readable on any page and background colour; the percentage only where it fits below the spinner;
// - the thin bar at the top edge, the load progress in the light colour on a black track;
// - the failure: a short message in the card, "Press OK to try again" where the remote's keys reach the page, over the
//   page's background colour, which hides WebKit's error page and its raw error text until another page shows. A tap
//   on it tries again (the widget that is not interactive opens the fullscreen web view instead, above this).
// Only that cover takes touches; the spinner and the bar let every touch through to the page.
//
// Without the UC.WebView import, like the zoom label: WebViewPage.qml maps the WebView's values.

import QtQuick 2.15

import "qrc:/components" as Components

Item {
    id: loadIndicator
    objectName: "loadIndicator"

    // the WebView's loadIndicator: LoadIndicatorSpinner, LoadIndicatorBar, LoadIndicatorFailed
    property bool spinnerShown: false
    property bool barShown: false
    property bool failureShown: false
    // the WebView's pageFailed: it shows a failed load's error page, or nothing after a crash
    property bool pageFailed: false
    // the page's background colour, below the cover
    property color pageColor: colors.white
    // the load progress in percent
    property int progress: 0
    // the remote's OK reaches the page (the fullscreen web view): the failure says to press it
    property bool okHint: true

    // a tap on the failure
    signal retry()

    //: Web view entity, on the whole screen or as a widget of an activity page: the web page did not load (no network,
    //: a wrong address, the server does not answer)
    readonly property string failedText: qsTr("The page could not be loaded")
    //: Web view entity on the whole screen, below "The page could not be loaded": OK loads the page again
    readonly property string hintText: qsTr("Press OK to try again")

    readonly property int margin: 20
    readonly property int padding: 20
    readonly property int spacing: 12
    readonly property int barHeight: 6
    // the spinner shrinks with the page down to a 3 x 3 widget; the percentage goes where it does not fit below it
    readonly property int spinnerSize: Math.round(Math.max(40, Math.min(72, (Math.min(width, height)
                                                                           - 2 * (margin + padding)) / 2)))
    readonly property bool progressFits: Math.max(spinnerSize, widestProgress.advanceWidth) + 2 * padding
                                         <= width - 2 * margin
                                         && spinnerSize + spacing + valueMetrics.height + 2 * padding
                                         <= height - 2 * margin

    TextMetrics {
        id: widestProgress
        font: fonts.value()
        text: "100 %"
    }

    FontMetrics {
        id: valueMetrics
        font: fonts.value()
    }

    FontMetrics {
        id: proseMetrics
        font: fonts.prose()
    }

    // the message on one line where it fits, else wrapped in the widest card the page has room for (the gutter around)
    TextMetrics {
        id: failedMetrics
        font: fonts.prose()
        text: loadIndicator.failedText
    }

    TextMetrics {
        id: hintMetrics
        font: fonts.help()
        text: loadIndicator.hintText
    }

    // WebKit's error page or a blank view after a crash: the page's background over it
    Rectangle {
        objectName: "loadFailedCover"
        anchors.fill: parent
        visible: loadIndicator.pageFailed
        color: loadIndicator.pageColor

        // a touch never reaches the error page
        MouseArea {
            anchors.fill: parent
        }

        // on the failure a tap tries again; while the page loads again a touch does nothing
        Components.HapticMouseArea {
            anchors.fill: parent
            enabled: loadIndicator.failureShown
            onClicked: loadIndicator.retry()
        }
    }

    Rectangle {
        id: card
        objectName: "loadIndicatorCard"
        anchors.centerIn: parent
        width: (loadIndicator.failureShown ? failureColumn.width : spinnerColumn.width) + 2 * loadIndicator.padding
        height: (loadIndicator.failureShown ? failureColumn.height : spinnerColumn.height) + 2 * loadIndicator.padding
        visible: loadIndicator.spinnerShown || loadIndicator.failureShown
        radius: ui.cornerRadiusSmall
        color: colors.bg

        Column {
            id: spinnerColumn
            anchors.centerIn: parent
            spacing: loadIndicator.spacing
            visible: !loadIndicator.failureShown

            // a three-quarter ring that fades towards its tail, like the app's small spinner (loader_small.png),
            // drawn at its size
            Canvas {
                id: spinner
                objectName: "loadSpinner"
                anchors.horizontalCenter: parent.horizontalCenter
                width: loadIndicator.spinnerSize
                height: width

                onWidthChanged: requestPaint()
                onPaint: {
                    const ctx = getContext("2d");
                    ctx.reset();
                    const line = Math.max(3, Math.round(width / 10));
                    const centre = width / 2;
                    // bright at the head, which leads the clockwise turn, transparent at the tail
                    const gradient = ctx.createConicalGradient(centre, centre, 0);
                    gradient.addColorStop(0, colors.textPrimary);
                    gradient.addColorStop(0.75, Qt.rgba(colors.textPrimary.r, colors.textPrimary.g,
                                                        colors.textPrimary.b, 0));
                    gradient.addColorStop(1, colors.textPrimary);
                    ctx.lineWidth = line;
                    ctx.lineCap = "round";
                    ctx.strokeStyle = gradient;
                    ctx.beginPath();
                    ctx.arc(centre, centre, centre - line / 2, 0, -1.5 * Math.PI, true);
                    ctx.stroke();
                }

                RotationAnimation on rotation {
                    running: spinner.visible
                    loops: Animation.Infinite
                    from: 0; to: 360
                    duration: 1200
                }
            }

            Text {
                objectName: "loadProgressText"
                anchors.horizontalCenter: parent.horizontalCenter
                visible: loadIndicator.progressFits
                color: colors.textPrimary
                // a value with its unit, like the zoom level
                font: fonts.value()
                text: loadIndicator.progress + " %"
            }
        }

        Column {
            id: failureColumn
            anchors.centerIn: parent
            width: Math.max(1, Math.min(loadIndicator.width - 2 * (loadIndicator.margin + loadIndicator.padding),
                                        Math.ceil(Math.max(failedMetrics.advanceWidth,
                                                           loadIndicator.okHint ? hintMetrics.advanceWidth : 0)) + 1))
            spacing: loadIndicator.spacing
            visible: loadIndicator.failureShown

            Text {
                objectName: "loadFailedText"
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                // as many lines as the page has room for, in a small widget
                maximumLineCount: Math.max(1, Math.floor((loadIndicator.height - 2 * (loadIndicator.margin
                                                                                   + loadIndicator.padding))
                                                         / (proseMetrics.height * fonts.proseLineHeight)))
                elide: Text.ElideRight
                color: colors.textPrimary
                font: fonts.prose()
                lineHeight: fonts.proseLineHeight
                text: loadIndicator.failedText
            }

            Text {
                objectName: "loadFailedHint"
                width: parent.width
                visible: loadIndicator.okHint
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: colors.textSecondary
                font: fonts.help()
                text: loadIndicator.hintText
            }
        }
    }

    Rectangle {
        objectName: "loadProgressBar"
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: loadIndicator.barHeight
        visible: loadIndicator.barShown
        color: colors.bg

        Rectangle {
            objectName: "loadProgressFill"
            anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
            width: parent.width * Math.max(0, Math.min(100, loadIndicator.progress)) / 100
            color: colors.textPrimary
        }
    }
}
