// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Developer page of the embedded web view. Listed in the settings menu only when the firmware provides libucwebview
// (ui.webViewAvailable), see docs/webview.md. Two rows above the WebView:
//
//   [ address field ...................... ][ Load ]
//   [ <- ][ -> ][ Stop/Reload ]  status line (two lines)
//
// The page uses the QML focus chain (ADR 0007) and the d-pad moves as the rows look:
// - address field <-> Load (left/right; right leaves the field at the end of its text, up from Load reaches it too);
// - down from the field: the first enabled button of the second row; down from Load: Stop/Reload;
// - Back <-> Forward <-> Stop/Reload (left/right), up: the address field, down: the web view; a disabled Back or
//   Forward (no history in that direction) is skipped;
// - inside the web view the arrow keys and OK go to the page; a tap on a control or BACK (leaves the page) gets out.
// PREV and NEXT go back and forward in the page's history wherever the d-pad is, the web view included.

import QtQuick 2.15
import QtQuick.Layouts 1.15

import UC.WebView 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: webViewPage
    initialFocusItem: loadButton

    property string lastEvent: ""

    // the square icon buttons of the second row
    readonly property int toolButtonSize: 60

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

    // the user is typing into the address field: the keyboard is up on it
    function editingAddress() {
        return urlField.inputField.activeFocus && typeof keyboard !== "undefined" && keyboard.active;
    }

    function load() {
        const address = urlField.inputValue.trim();
        if (address === "") {
            return;
        }
        // done typing: show the page, like WifiPassword's Join. The field keeps the focus (InputField with
        // keyboardFollowsFocus); a tap on it or a d-pad walk back brings the keyboard again. Hidden before the
        // load starts, so that the field shows the completed address (onUrlChanged below).
        if (typeof keyboard !== "undefined" && urlField.inputField.activeFocus) {
            keyboard.hide();
        }
        webViewPage.lastEvent = "";
        webView.url = normalizedAddress(address);
    }

    // Back on the first history entry (Forward on the last) turns disabled while the d-pad is on it. Qt takes the
    // active focus from a disabled item and leaves its `focus` (qquickitem.cpp:6149-6152), and the page's
    // ButtonNavigation claims the focus back for the button it remembers, which a disabled item refuses
    // (qquickwindow.cpp:1114-1123): the d-pad would be dead until the next touch. Hand the focus to the other
    // history button, or to Stop/Reload. Deferred: the other button's `enabled` binding may not have seen the same
    // history change yet.
    function leaveDisabledHistoryButton() {
        if (!webViewPage.buttonNavigation.hasInputControl) {
            return;
        }
        for (const button of [backButton, forwardButton]) {
            if (!button.enabled && button.focus) {
                const other = button === backButton ? forwardButton : backButton;
                (other.enabled ? other : stopReloadButton).forceActiveFocus();
                return;
            }
        }
    }

    // PREV and NEXT of the remote: one step back or forward in the page's history, like the Back and Forward buttons.
    // They arrive through the page's button navigation (the input owner path, docs/key-navigation.md section 1), so
    // they work wherever the d-pad focus is, the web view included: the WebView item does not forward the remote's
    // media keys to the page (webviewinput.cpp) and leaves them unaccepted. Without history in that direction the key
    // does nothing. Ignored while the on-screen keyboard is up: the user is typing into the address field or into a
    // form of the page, which a navigation would take away under the keyboard (MediaBrowser.qml ignores its list keys
    // while the keyboard is up, too).
    function historyStep(forward) {
        if (typeof keyboard !== "undefined" && keyboard.active) {
            return;
        }
        if (forward && webView.canGoForward) {
            webView.goForward();
        } else if (!forward && webView.canGoBack) {
            webView.goBack();
        }
    }

    // One step per press: a held key does not repeat (pressed_repeat does nothing). Every step starts a navigation,
    // and the remote's auto-repeat (every 150 ms after 600 ms) would skip pages the user never saw. Extended, not
    // assigned: Settings.Page declares BACK and HOME (docs/key-navigation.md section 2).
    Component.onCompleted: {
        webViewPage.buttonNavigation.extendDefaultConfig({
            "PREV": {
                "pressed": function() {
                    webViewPage.historyStep(false);
                },
                "pressed_repeat": function() {}
            },
            "NEXT": {
                "pressed": function() {
                    webViewPage.historyStep(true);
                },
                "pressed_repeat": function() {}
            }
        });
    }

    // Like a browser's address bar the field shows the address of the page on screen (completed, redirected, after
    // Back or Forward), except while the user types into it. A refused address leaves the url, and the field, as
    // they are.
    Connections {
        target: webView

        function onUrlChanged() {
            if (!webViewPage.editingAddress() && webView.url.toString() !== "") {
                urlField.inputValue = webView.url.toString();
            }
        }
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
                // The keyboard comes up when the d-pad lands on the field, like on the Wi-Fi forms; DPAD_MIDDLE is
                // the field's Return and loads.
                moveInput: false
                keyboardFollowsFocus: true
                // right only at the end of the text, the caret moves first
                navRight: loadButton
                navDown: backButton.enabled ? backButton : (forwardButton.enabled ? forwardButton : stopReloadButton)
            }

            Components.Button {
                id: loadButton
                //: Button on the developer page that loads the entered web address in the embedded web view
                text: qsTr("Load")
                trigger: webViewPage.load
                // The field is to the left. Load had only `up` to it: left, where the field is, did nothing, so the
                // field could not be reached with the d-pad (found in a device test).
                KeyNavigation.left: urlField.inputField
                KeyNavigation.up: urlField.inputField
                KeyNavigation.down: stopReloadButton
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Components.Button {
                id: backButton
                objectName: "backButton"
                Layout.preferredWidth: webViewPage.toolButtonSize
                Layout.preferredHeight: webViewPage.toolButtonSize
                text: ""
                // dimmed and skipped by the d-pad (KeyNavigation passes over disabled items) without history
                enabled: webView.canGoBack
                opacity: enabled ? 1 : 0.3
                trigger: function() {
                    webView.goBack();
                }
                onEnabledChanged: if (!enabled) Qt.callLater(webViewPage.leaveDisabledHistoryButton)
                KeyNavigation.up: urlField.inputField
                KeyNavigation.right: forwardButton
                KeyNavigation.down: webView

                Components.Icon {
                    anchors.centerIn: parent
                    size: 60
                    color: colors.offwhite
                    icon: "uc:arrow-left"
                }
            }

            Components.Button {
                id: forwardButton
                objectName: "forwardButton"
                Layout.preferredWidth: webViewPage.toolButtonSize
                Layout.preferredHeight: webViewPage.toolButtonSize
                text: ""
                enabled: webView.canGoForward
                opacity: enabled ? 1 : 0.3
                trigger: function() {
                    webView.goForward();
                }
                onEnabledChanged: if (!enabled) Qt.callLater(webViewPage.leaveDisabledHistoryButton)
                KeyNavigation.up: urlField.inputField
                KeyNavigation.left: backButton
                KeyNavigation.right: stopReloadButton
                KeyNavigation.down: webView

                Components.Icon {
                    anchors.centerIn: parent
                    size: 60
                    color: colors.offwhite
                    icon: "uc:arrow-right"
                }
            }

            // One button for both: Stop while a page loads, Reload otherwise
            Components.Button {
                id: stopReloadButton
                objectName: "stopReloadButton"
                Layout.preferredWidth: webViewPage.toolButtonSize
                Layout.preferredHeight: webViewPage.toolButtonSize
                text: ""
                trigger: function() {
                    if (webView.loading) {
                        webView.stop();
                    } else {
                        webView.reload();
                    }
                }
                KeyNavigation.up: urlField.inputField
                KeyNavigation.left: forwardButton
                KeyNavigation.down: webView

                Components.Icon {
                    objectName: "stopReloadIcon"
                    anchors.centerIn: parent
                    size: 60
                    color: colors.offwhite
                    icon: webView.loading ? "uc:xmark" : "uc:arrow-rotate-right"
                }
            }

            Text {
                id: statusLine
                objectName: "statusLine"
                Layout.fillWidth: true
                Layout.preferredHeight: webViewPage.toolButtonSize
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                maximumLineCount: 2
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
        }

        WebView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true
            // only without the library: with it, the arrows go to the page
            KeyNavigation.up: stopReloadButton

            onCrashed: webViewPage.lastEvent = "crashed (" + reason + ")"
            onLoadFailed: webViewPage.lastEvent = "load failed: " + url
        }
    }
}
