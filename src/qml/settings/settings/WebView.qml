// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Developer page of the embedded web view: an address field, a Load button, a Stop/Reload button, the WebView and a
// status line. Listed in the settings menu only when the firmware provides libucwebview (ui.webViewAvailable), see
// docs/webview.md. The page uses the QML focus chain (ADR 0007): the d-pad moves field -> Load -> Stop/Reload ->
// web view (down) and back (up); inside the web view the arrow keys and Enter go to the page.

import QtQuick 2.15
import QtQuick.Layouts 1.15

import UC.WebView 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: webViewPage
    initialFocusItem: loadButton

    property string lastEvent: ""

    // What the address field means: a URL with a scheme the web view knows is loaded as it is (the item refuses
    // file: unless UC_WEBVIEW_ALLOW_FILE_URLS=1 is set, and every scheme outside http, https, file, about and data,
    // with loadFailed), an absolute path becomes a file:// URL, anything else a host for http:// ("localhost:8080").
    function normalizedAddress(address) {
        if (/^(https?|file|about|data):/i.test(address)) {
            return address;
        }
        if (address.charAt(0) === "/") {
            return "file://" + address;
        }
        return address.indexOf("://") === -1 ? "http://" + address : address;
    }

    function load() {
        const address = urlField.inputValue.trim();
        if (address === "") {
            return;
        }
        webViewPage.lastEvent = "";
        webView.url = normalizedAddress(address);
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
                // Edit in place with the keyboard below, like every other form (WifiPassword.qml). The default
                // moveInput opens the full-screen input overlay of main.qml, which takes the input control; the
                // page's ButtonNavigation (manageFocus) then parks the focus, the virtual keyboard closes without a
                // focused text item, the overlay hides and hands the control back, the ButtonNavigation refocuses
                // the field, which opens the overlay again: an endless show/hide loop, with no web page involved.
                moveInput: false
                keyboardFollowsFocus: true
                navDown: loadButton
            }

            Components.Button {
                id: loadButton
                //: Button on the developer page that loads the entered web address in the embedded web view
                text: qsTr("Load")
                trigger: webViewPage.load
                KeyNavigation.up: urlField.inputField
                KeyNavigation.down: stopReloadButton
                KeyNavigation.right: stopReloadButton
            }

            // One button for both, the row has no room for two: Stop while a page loads, Reload otherwise. Square,
            // as high as Load.
            Components.Button {
                id: stopReloadButton
                objectName: "stopReloadButton"
                width: height
                text: ""
                trigger: function() {
                    if (webView.loading) {
                        webView.stop();
                    } else {
                        webView.reload();
                    }
                }
                KeyNavigation.up: loadButton
                KeyNavigation.left: loadButton
                KeyNavigation.down: webView

                Components.Icon {
                    objectName: "stopReloadIcon"
                    anchors.centerIn: parent
                    size: 60
                    color: colors.offwhite
                    icon: webView.loading ? "uc:xmark" : "uc:arrow-rotate-right"
                }
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
            KeyNavigation.up: stopReloadButton

            onCrashed: webViewPage.lastEvent = "crashed (" + reason + ")"
            onLoadFailed: webViewPage.lastEvent = "load failed: " + url
        }
    }
}
