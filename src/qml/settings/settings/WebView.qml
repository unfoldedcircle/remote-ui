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
// Wherever the d-pad is, the web view included, PREV and NEXT go back and forward in the page's history, VOLUME +/-
// zoom the page in and out, CHANNEL +/- scroll it a page up and down (not while the on-screen keyboard is up).
// Two fingers pinch-zoom the page (libucwebview 0.6.0): the zoom label shows the magnification next to the zoom.
// The page's alert(), confirm(), prompt(), "leave this page?" question and HTTP logins open a dialog over the page
// (libucwebview 0.7.0, components/WebViewDialog.qml and WebViewLogin.qml), one at a time, script dialogs first: the page
// waits for them. While one is open it owns the keys; PREV/NEXT, VOLUME and CHANNEL do nothing. Downloads, documents the
// web view cannot show (PDF) and refused new windows are told with a notification; window.close() of the page goes
// back in the history, or leaves this page like BACK when there is none.

import QtQuick 2.15
import QtQuick.Layouts 1.15

import UC.WebView 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: webViewPage
    initialFocusItem: loadButton

    property string lastEvent: ""

    // the square icon buttons of the second row: the smallest hit target of the design system, an icon box of a row
    readonly property int toolButtonSize: 60

    // The spinner next to the zoom level appears only when the page takes longer than this to render a zoom step
    // (webView.zoomRendering): a quick step does not flicker.
    readonly property int zoomSpinnerDelay: 150
    property bool zoomSpinnerShown: false

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

    // the on-screen keyboard is up, on the address field or on an input of the page: the user is typing
    function keyboardUp() {
        return typeof keyboard !== "undefined" && keyboard.active;
    }

    // a dialog or login of the web page is open, or still fading: the keys are its own
    function requestShown() {
        return webViewDialog.visible || webViewLogin.visible;
    }

    // the user is typing into the address field: the keyboard is up on it
    function editingAddress() {
        return urlField.inputField.activeFocus && webViewPage.keyboardUp();
    }

    function load() {
        const address = urlField.inputValue.trim();
        if (address === "") {
            return;
        }
        // done typing: show the page, like WifiPassword's Join. The field's frame keeps the focus (InputField with
        // keyboardFollowsFocus); a tap on the field or a d-pad walk back brings the keyboard again. Hidden before the
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
        if (webViewPage.keyboardUp() || webViewPage.requestShown()) {
            return;
        }
        if (forward && webView.canGoForward) {
            webView.goForward();
        } else if (!forward && webView.canGoBack) {
            webView.goBack();
        }
    }

    // VOLUME+ and VOLUME-: one zoom level in or out (the levels are the WebView item's, 50 to 300 %), shown over the
    // page while it renders the new level and for a moment after. WebKit keeps the zoom for the view across
    // navigations, and the view lives as long as the page: every visit of the page starts at 100 %. The page has the
    // keys to itself: a settings page does nothing with them otherwise, the volume of an activity is the main
    // screen's (components/Page.qml). Like PREV and NEXT they do nothing while the keyboard is up: a relayout would
    // move the field the user types into. Presses while the page renders a step move the level at once; the item
    // hands only the latest one to WebKit when the render is done (docs/webview.md).
    function zoomStep(up) {
        if (webViewPage.keyboardUp() || webViewPage.requestShown()) {
            return;
        }
        if (up) {
            webView.zoomIn();
        } else {
            webView.zoomOut();
        }
        // also at the first or last level: the label tells that the zoom ends there
        zoomLabelHold.restart();
    }

    // CHANNEL+ and CHANNEL-: Page_Up and Page_Down for the page, which WebKit scrolls by a page like a browser does
    // (in a text area of the page that has the focus the key moves its caret a page instead, docs/webview.md). Not
    // while the keyboard is up, like the other keys.
    function scrollStep(down) {
        if (webViewPage.keyboardUp() || webViewPage.requestShown()) {
            return;
        }
        if (down) {
            webView.pageDown();
        } else {
            webView.pageUp();
        }
    }

    // ---- the web page's requests (libucwebview 0.7.0, docs/webview.md "Dialogs and logins") ----------------------

    // Requests waiting for their dialog, oldest first. Script dialogs go first: the page is blocked until the answer.
    // WebKit has one script dialog of a page at a time, several logins can wait (a page with several protected parts).
    property var dialogQueue: []
    property var loginQueue: []

    function queueRequest(queue, request) {
        // the page answers it: the item does not answer it with the default
        request.handled = true;
        queue.push(request);
        // a request that ends while it waits (a navigation, the page went on) leaves the queue
        const drop = function() {
            webViewPage.dropRequest(request);
        };
        request.finished.connect(drop);
        request.cancelled.connect(drop);
        // not from inside the library's callback: the dialog takes the focus from the web view
        Qt.callLater(webViewPage.showNextRequest);
    }

    function dropRequest(request) {
        webViewPage.dialogQueue = webViewPage.dialogQueue.filter(function(r) { return r !== request; });
        webViewPage.loginQueue = webViewPage.loginQueue.filter(function(r) { return r !== request; });
    }

    function showNextRequest() {
        if (webViewPage.requestShown()) {
            return;
        }
        if (webViewPage.dialogQueue.length > 0) {
            webViewDialog.show(webViewPage.dialogQueue.shift());
        } else if (webViewPage.loginQueue.length > 0) {
            webViewLogin.show(webViewPage.loginQueue.shift());
        }
    }

    // the PDF types of libucwebview (uc_pdf_policy.c), with or without parameters
    function isPdf(mimeType) {
        return /^\s*(application\/pdf|application\/x-pdf|text\/pdf)\s*(;|$)/i.test(mimeType);
    }

    // PREV/NEXT and VOLUME: one step per press, a held key does not repeat (pressed_repeat does nothing). Every
    // history step starts a navigation and every zoom step a relayout, and the remote's auto-repeat (every 150 ms
    // after 600 ms) would skip pages and levels the user never saw. CHANNEL repeats: holding it scrolls on. Extended,
    // not assigned: Settings.Page declares BACK and HOME (docs/key-navigation.md section 2).
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
            },
            "VOLUME_UP": {
                "pressed": function() {
                    webViewPage.zoomStep(true);
                },
                "pressed_repeat": function() {}
            },
            "VOLUME_DOWN": {
                "pressed": function() {
                    webViewPage.zoomStep(false);
                },
                "pressed_repeat": function() {}
            },
            "CHANNEL_UP": {
                "pressed": function() {
                    webViewPage.scrollStep(false);
                },
                "pressed_repeat": function() {
                    webViewPage.scrollStep(false);
                }
            },
            "CHANNEL_DOWN": {
                "pressed": function() {
                    webViewPage.scrollStep(true);
                },
                "pressed_repeat": function() {
                    webViewPage.scrollStep(true);
                }
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

        // the label stays a second after a pinch, like after a key press
        function onPinchingChanged() {
            if (!webView.pinching) {
                zoomLabelHold.restart();
            }
        }

        function onZoomRenderingChanged() {
            if (webView.zoomRendering) {
                zoomSpinnerTimer.restart();
            } else {
                zoomSpinnerTimer.stop();
                webViewPage.zoomSpinnerShown = false;
            }
        }

        function onDialogRequested(request) {
            webViewPage.queueRequest(webViewPage.dialogQueue, request);
        }

        function onAuthenticationRequested(request) {
            webViewPage.queueRequest(webViewPage.loginQueue, request);
        }

        // nothing was saved and the page stays; the item reports no load failure for it
        function onResponseRefused(url, mimeType, download) {
            let message;
            if (download) {
                //: Notification when a web page starts a download, which the remote cannot save
                message = qsTr("Downloads are not supported");
            } else if (webViewPage.isPdf(mimeType)) {
                //: Notification when a link of a web page opens a PDF document, which the remote cannot show
                message = qsTr("PDF documents cannot be shown here");
            } else {
                //: Notification when a link of a web page opens a file the remote cannot show (not a web page or image)
                message = qsTr("This document cannot be shown here");
            }
            ui.createNotification(message, true);
            webViewPage.lastEvent = (download ? "download refused: " : "not shown: ") + url;
        }

        // window.open() and target=_blank: http and https load into this view without a word, like a link
        function onNewWindow(url, loaded) {
            if (!loaded) {
                //: Notification when a web page tries to open a new window with an address the remote cannot show
                ui.createNotification(qsTr("The page tried to open a link that cannot be shown"), true);
                webViewPage.lastEvent = "new window refused: " + url;
            }
        }

        // window.close() of the page: back in the history if there is one, else away from this page like BACK.
        // WebKit allows it only on a page with at most one history entry, so it is nearly always
        // the latter.
        function onCloseRequested() {
            if (webView.canGoBack) {
                webView.goBack();
                return;
            }
            //: Notification when the web page shown in the user interface closed itself (window.close())
            ui.createNotification(qsTr("The web page closed itself"), false);
            profileRoot.goBack();
            webViewPage.buttonNavigation.restoreDefaultConfig();
        }
    }

    Timer {
        id: zoomSpinnerTimer
        interval: webViewPage.zoomSpinnerDelay
        onTriggered: webViewPage.zoomSpinnerShown = true
    }

    ColumnLayout {
        anchors { top: topNavigation.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        // the 20 px gutter of every page (docs/design-system.md section 5)
        anchors.leftMargin: 20
        anchors.rightMargin: 20
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
                // dimmed (Components.Button) and skipped by the d-pad (KeyNavigation passes over disabled items)
                // without history
                enabled: webView.canGoBack
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
                    // inverted on the pressed button, like a label (docs/design-system.md section 6)
                    color: backButton.pressed ? colors.bg : colors.textPrimary
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
                    color: forwardButton.pressed ? colors.bg : colors.textPrimary
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
                    color: stopReloadButton.pressed ? colors.bg : colors.textPrimary
                    icon: webView.loading ? "uc:xmark" : "uc:arrow-rotate-right"
                }
            }

            Text {
                id: statusLine
                objectName: "statusLine"
                Layout.fillWidth: true
                // two lines of the caption role, the smallest text there is
                Layout.preferredHeight: Math.max(webViewPage.toolButtonSize, Math.ceil(2 * statusMetrics.height))
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                color: colors.textSecondary
                font: fonts.caption()

                FontMetrics {
                    id: statusMetrics
                    font: statusLine.font
                }
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

            // the d-pad is in the web view: the ring of the selected control (docs/design-system.md section 6)
            Components.Selectable {
                selected: webView.activeFocus
            }

            // The zoom level after VOLUME+/- and the magnification of a pinch, at the top of the page: at once, while
            // the fingers move and the page renders it, and for about a second after the last press or pinch; then it
            // fades out. A render that takes longer than zoomSpinnerDelay adds a spinner next to the level.
            Rectangle {
                id: zoomLabel
                objectName: "zoomLabel"
                // a press or pinch within the last second, a pinch, or the page still renders the level
                readonly property bool shown: zoomLabelHold.running || webView.zoomRendering || webView.pinching
                anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }
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
                        // "125 %", "2.0×" or both ("125 % · 2.0×"): the page zoom (VOLUME) and the magnification of
                        // a pinch are separate (docs/webview.md, "Pinch zoom")
                        text: {
                            const zoom = Math.round(webView.zoom * 100) + " %";
                            if (Math.abs(webView.magnification - 1) < 0.05) {
                                return zoom;
                            }
                            const magnification = webView.magnification.toFixed(1) + "\u00d7";
                            return Math.abs(webView.zoom - 1) < 0.005 ? magnification : zoom + " · " + magnification;
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
                        visible: webViewPage.zoomSpinnerShown

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
        }
    }

    // the web page's dialogs and logins, declared after the page's controls: they open over all of them
    Components.WebViewDialog {
        id: webViewDialog
        objectName: "webViewDialog"
        onClosed: Qt.callLater(webViewPage.showNextRequest)
    }

    Components.WebViewLogin {
        id: webViewLogin
        objectName: "webViewLogin"
        onClosed: Qt.callLater(webViewPage.showNextRequest)
    }
}
