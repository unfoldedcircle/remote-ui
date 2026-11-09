// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Developer page of the embedded web view: an address field, a Load button, the WebView and a status line.
// Listed in the settings menu only when the firmware provides libucwebview (ui.webViewAvailable), see
// docs/webview.md. The page uses the QML focus chain (ADR 0007): the d-pad moves between the field, the button
// and the web view; inside the web view the arrow keys and Enter go to the page.

import QtQuick 2.15
import QtQuick.Layouts 1.15

import UC.WebView 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: webViewPage
    initialFocusItem: loadButton

    property string lastEvent: ""

    function load() {
        const address = urlField.inputValue.trim();
        if (address === "") {
            return;
        }
        webViewPage.lastEvent = "";
        webView.url = address.indexOf("://") === -1 ? "http://" + address : address;
    }

    ColumnLayout {
        anchors { top: topNavigation.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Components.InputField {
                id: urlField
                Layout.fillWidth: true
                inputValue: "http://localhost/"
                //: Placeholder of the address field on the developer page that shows a web page inside the user interface
                inputField.placeholderText: qsTr("Web address")
                inputField.inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                inputField.onAccepted: webViewPage.load()
                navDown: loadButton
            }

            Components.Button {
                id: loadButton
                //: Button on the developer page that loads the entered web address in the embedded web view
                text: qsTr("Load")
                trigger: webViewPage.load
                KeyNavigation.up: urlField.inputField
                KeyNavigation.down: webView
            }
        }

        Text {
            id: statusLine
            Layout.fillWidth: true
            elide: Text.ElideRight
            color: colors.light
            font: fonts.caption()
            text: {
                if (!webView.available) {
                    //: Status line of the developer page when the firmware has no web view library
                    return qsTr("Web view unavailable");
                }
                let status = webView.loading ? "loading " + webView.loadProgress + "%" : webView.title;
                status += " | frames " + webView.framesReceived + "/" + webView.framesReleased;
                if (webViewPage.lastEvent !== "") {
                    status += " | " + webViewPage.lastEvent;
                }
                return status;
            }
        }

        WebView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true
            KeyNavigation.up: loadButton

            onCrashed: webViewPage.lastEvent = "crashed (" + reason + ")"
            onLoadFailed: webViewPage.lastEvent = "load failed: " + url
        }
    }
}
