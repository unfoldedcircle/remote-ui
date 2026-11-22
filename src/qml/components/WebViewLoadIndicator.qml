// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// What the page of a web view entity shows while it loads (docs/webview.md, "Load indicator"), over the WebView of
// WebViewPage.qml: in the fullscreen web view and in the web view widget of an activity page. The WebView decides
// what to show (loadIndicator, pageFailed); this draws it at the page's size:
// - the spinner with the load progress, centred, in a card like the zoom label (WebViewZoomLabel.qml), black with the
//   light text, readable on any page and background colour; the percentage only where it fits below the spinner;
// - the thin bar at the top edge, the load progress in the light colour on a black track;
// - the failure: a short message in the card with the reason below it (the WebView's loadError), over the page's
//   background colour, which hides WebKit's error page and its raw error text until another page shows. In the
//   fullscreen web view (buttonsShown) the card has Try again and Details (the technical details in a bottom sheet,
//   only when there are any) and "Press OK to try again"; a tap beside them does nothing. The widget has no buttons:
//   the reason only where it fits, and a tap tries again (the widget that is not interactive opens the fullscreen web
//   view instead, above this).
// Only that cover and the card of the failure take touches; the spinner and the bar let every touch through to the
// page.
//
// Without the UC.WebView import, like the zoom label: WebViewPage.qml maps the WebView's values.

import QtQuick 2.15
import QtQuick.Controls 2.15

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
    // the failure has Try again and Details (the fullscreen web view), else a tap on it tries again (the widget)
    property bool buttonsShown: true
    // why the load failed (the WebView's loadError): "" (unknown), "other", "hostNotFound", "noNetwork",
    // "connectionRefused", "timeout", "certificate", "unsupportedAddress", "crashed", "outOfMemory"
    property string error: ""
    // the host of the failing address
    property string errorHost: ""
    // the technical details, untranslated (the WebView's loadErrorDetails); Details only when there are any
    property string errorDetails: ""

    // Try again, OK, or a tap on the failure of the widget
    signal retry()
    // the Details sheet closed: the page takes the keys again
    signal detailsClosed()

    // the failure went (a load the user waits for, a page shows): its details go with it
    onFailureShownChanged: {
        if (!loadIndicator.failureShown) {
            detailsPopup.close();
        }
    }

    //: Web view entity, on the whole screen or as a widget of an activity page: the web page did not load (no network,
    //: a wrong address, the server does not answer)
    readonly property string failedText: qsTr("The page could not be loaded")
    //: Web view entity on the whole screen, below "The page could not be loaded": OK loads the page again
    readonly property string hintText: qsTr("Press OK to try again")
    // the line below the message that says why, "" when the web view does not know (an older library, another reason)
    // or a reason that names the server has no host
    readonly property string reasonText: {
        const host = loadIndicator.errorHost;
        switch (loadIndicator.error) {
        case "hostNotFound":
            //: Web view entity, below "The page could not be loaded": the name of the web server could not be found
            //: (DNS). %1 is the server's name, such as www.example.com
            return host === "" ? ""
                               : qsTr("The server %1 wasn't found. Check the Remote's Wi-Fi connection and the address.")
                                 .arg(host);
        case "noNetwork":
            //: Web view entity, below "The page could not be loaded": the remote has no network connection (or no route
            //: to the server)
            return qsTr("The Remote isn't connected to a network.");
        case "connectionRefused":
            //: Web view entity, below "The page could not be loaded": the server answered that nothing listens on the
            //: port. %1 is the server's name or IP address, such as 192.168.1.20
            return host === "" ? ""
                               : qsTr("%1 refused the connection. Check that the server is running and the port is correct.")
                                 .arg(host);
        case "timeout":
            //: Web view entity, below "The page could not be loaded": the server did not answer. %1 is the server's
            //: name or IP address
            return host === "" ? "" : qsTr("%1 didn't respond in time.").arg(host);
        case "certificate":
            //: Web view entity, below "The page could not be loaded": the server's HTTPS certificate was not accepted.
            //: %1 is the server's name
            return host === "" ? ""
                               : qsTr("The certificate of %1 isn't trusted (expired, wrong name, or self-signed).")
                                 .arg(host);
        case "unsupportedAddress":
            //: Web view entity, below "The page could not be loaded": the web address uses another scheme, such as
            //: ftp:// or file://
            return qsTr("Only http:// and https:// addresses can be opened.");
        case "crashed":
            //: Web view entity, below "The page could not be loaded": the web engine's process of the page crashed
            //: (twice)
            return qsTr("The page stopped working.");
        case "outOfMemory":
            //: Web view entity, below "The page could not be loaded": the web page used more memory than the remote
            //: allows
            return qsTr("The page needs more memory than the Remote has.");
        default:
            return "";
        }
    }
    readonly property bool detailsAvailable: loadIndicator.buttonsShown && loadIndicator.errorDetails !== ""

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

    FontMetrics {
        id: helpMetrics
        font: fonts.help()
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

        // on the failure of the widget a tap tries again; with the buttons and while the page loads again a touch does
        // nothing
        Components.HapticMouseArea {
            anchors.fill: parent
            enabled: loadIndicator.failureShown && !loadIndicator.buttonsShown
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

        // the failure's card takes the touches on it, also over a page that is not covered (a refused address), and a
        // tap on the card of the widget tries again; the spinner's card lets them through
        MouseArea {
            anchors.fill: parent
            enabled: loadIndicator.failureShown
        }

        Components.HapticMouseArea {
            anchors.fill: parent
            enabled: loadIndicator.failureShown && !loadIndicator.buttonsShown
            onClicked: loadIndicator.retry()
        }

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
            // the room inside the gutter and the card's padding
            readonly property int maxWidth: loadIndicator.width - 2 * (loadIndicator.margin + loadIndicator.padding)
            readonly property int maxHeight: loadIndicator.height - 2 * (loadIndicator.margin + loadIndicator.padding)
            // the reason's lines below the message where they fit (the widget), at least one
            readonly property int reasonLines: Math.floor((maxHeight - failedText.height - spacing)
                                                          / helpMetrics.height)

            anchors.centerIn: parent
            // the message's width; the whole room with a reason or the buttons
            width: Math.max(1, Math.min(maxWidth, loadIndicator.reasonText !== "" || loadIndicator.buttonsShown
                                        ? maxWidth
                                        : Math.ceil(Math.max(failedMetrics.advanceWidth,
                                                             loadIndicator.okHint ? hintMetrics.advanceWidth : 0)) + 1))
            spacing: loadIndicator.spacing
            visible: loadIndicator.failureShown

            Text {
                id: failedText
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
                objectName: "loadFailedReason"
                width: parent.width
                visible: loadIndicator.reasonText !== "" && failureColumn.reasonLines >= 1
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                maximumLineCount: Math.max(1, failureColumn.reasonLines)
                elide: Text.ElideRight
                // the host is the page's, never markup
                textFormat: Text.PlainText
                color: colors.textSecondary
                font: fonts.help()
                text: loadIndicator.reasonText
            }

            Item {
                width: parent.width
                height: retryButton.height
                visible: loadIndicator.buttonsShown

                Components.Button {
                    id: retryButton
                    objectName: "loadFailedRetry"
                    anchors { left: parent.left; bottom: parent.bottom }
                    width: loadIndicator.detailsAvailable ? parent.width / 2 - 10 : parent.width
                    //: Web view entity on the whole screen, button below "The page could not be loaded": load the page
                    //: again
                    text: qsTr("Try again")
                    trigger: function() {
                        loadIndicator.retry();
                    }
                }

                Components.Button {
                    objectName: "loadFailedDetails"
                    anchors { right: parent.right; bottom: parent.bottom }
                    width: parent.width / 2 - 10
                    visible: loadIndicator.detailsAvailable
                    variant: "secondary"
                    //: Web view entity on the whole screen: the button below "The page could not be loaded" and the
                    //: title of the sheet it opens, with the technical error (the server's address, the error code)
                    text: qsTr("Details")
                    trigger: function() {
                        detailsPopup.open();
                    }
                }
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

    // the technical details of the failure, a bottom sheet like the Wi-Fi network's (WifiInfo.qml): BACK, HOME, Close
    // and a tap above the sheet close it
    Popup {
        id: detailsPopup
        objectName: "loadFailedDetailsPopup"
        width: parent.width; height: parent.height
        opacity: 0
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        // the popup takes the keyboard focus: OK presses Close
        focus: true

        // what it shows, kept while it fades out
        property string details: ""

        onAboutToShow: detailsPopup.details = loadIndicator.errorDetails
        onOpened: detailsButtonNavigation.takeControl()
        onClosed: {
            detailsButtonNavigation.releaseControl();
            loadIndicator.detailsClosed();
        }

        enter: Transition {
            PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        exit: Transition {
            PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        Components.ButtonNavigation {
            id: detailsButtonNavigation
            manageFocus: true
            initialFocusItem: detailsCloseButton
            defaultConfig: {
                "BACK": {
                    "pressed": function() {
                        detailsPopup.close();
                    }
                },
                "HOME": {
                    "pressed": function() {
                        detailsPopup.close();
                    }
                }
            }
        }

        background: Components.Dim {}

        // a tap above the sheet closes it; a tap on the sheet does nothing
        MouseArea {
            anchors.fill: parent
            onClicked: detailsPopup.close()
        }

        Item {
            objectName: "loadFailedDetailsSheet"
            width: parent.width
            height: detailsContent.height + 40
            anchors.bottom: parent.bottom

            Components.Sheet {
                anchors.fill: parent
            }

            MouseArea {
                anchors.fill: parent
            }

            Column {
                id: detailsContent
                spacing: 20
                width: parent.width - 40
                anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }

                Text {
                    width: parent.width
                    color: colors.textPrimary
                    font: fonts.title()
                    // the Details button's text, one translation (its translator comment)
                    text: qsTr("Details")
                }

                Components.Divider {
                    inset: 0
                }

                Item {
                    width: parent.width
                    height: detailsFlickable.height

                    Flickable {
                        id: detailsFlickable
                        width: parent.width
                        // the sheet leaves the top of the page visible
                        height: Math.min(contentHeight, Math.max(120, detailsPopup.height - 300))
                        contentWidth: width
                        contentHeight: detailsText.implicitHeight
                        interactive: contentHeight > height
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        Text {
                            id: detailsText
                            objectName: "loadFailedDetailsText"
                            // room for the scroll indicator
                            width: detailsFlickable.width - 20
                            // the address, the error text of the network library and its code: never markup
                            textFormat: Text.PlainText
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            color: colors.textPrimary
                            font: fonts.value()
                            text: detailsPopup.details
                        }
                    }

                    Components.ScrollIndicator {
                        parentObj: detailsFlickable
                        padding: 0
                        hideOverride: !detailsFlickable.interactive
                    }
                }

                Components.Button {
                    id: detailsCloseButton
                    objectName: "loadFailedDetailsClose"
                    width: parent.width
                    variant: "secondary"
                    //: Button of the sheet with the technical details of a web page that could not be loaded
                    text: qsTr("Close")
                    trigger: function() {
                        detailsPopup.close();
                    }
                }
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
