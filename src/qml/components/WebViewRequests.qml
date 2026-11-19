// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The page's own requests for the fullscreen web view of a web view entity (libucwebview 0.7.0, docs/webview.md
// "Dialogs and logins"): alert(), confirm(), prompt(), the leave-page question and HTTP logins open as the dialogs of
// WebViewDialog.qml and WebViewLogin.qml, one at a time, script dialogs first (the page waits for them). A login the
// entity's configured credentials answer never gets here (the WebView item answers it). Downloads, documents the web
// view cannot show (PDF) and refused new windows are told with a notification.
//
// Fills the screen, so that the dialogs cover it. Loaded with a Loader next to WebViewPage.qml (UC.WebView).

import QtQuick 2.15

import "qrc:/components" as Components

Item {
    id: webViewRequests

    // the WebView item whose requests are shown
    property Item webView: null
    // a dialog or login is open, or still fading: the keys are its own
    readonly property bool shown: webViewDialog.visible || webViewLogin.visible

    // requests waiting for their dialog, oldest first
    property var dialogQueue: []
    property var loginQueue: []

    function queueRequest(queue, request) {
        // this screen answers it: the item does not answer it with the default
        request.handled = true;
        queue.push(request);
        // a request that ends while it waits (a navigation, the page went on) leaves the queue
        const drop = function() {
            webViewRequests.dropRequest(request);
        };
        request.finished.connect(drop);
        request.cancelled.connect(drop);
        // not from inside the library's callback: the dialog takes the focus from the web view
        Qt.callLater(webViewRequests.showNextRequest);
    }

    function dropRequest(request) {
        webViewRequests.dialogQueue = webViewRequests.dialogQueue.filter(function(r) { return r !== request; });
        webViewRequests.loginQueue = webViewRequests.loginQueue.filter(function(r) { return r !== request; });
    }

    function showNextRequest() {
        if (webViewRequests.shown) {
            return;
        }
        if (webViewRequests.dialogQueue.length > 0) {
            webViewDialog.show(webViewRequests.dialogQueue.shift());
        } else if (webViewRequests.loginQueue.length > 0) {
            webViewLogin.show(webViewRequests.loginQueue.shift());
        } else if (webViewRequests.webView) {
            // the d-pad goes back to the page
            webViewRequests.webView.forceActiveFocus();
        }
    }

    // the PDF types of libucwebview (uc_pdf_policy.c), with or without parameters
    function isPdf(mimeType) {
        return /^\s*(application\/pdf|application\/x-pdf|text\/pdf)\s*(;|$)/i.test(mimeType);
    }

    Connections {
        target: webViewRequests.webView
        ignoreUnknownSignals: true

        function onDialogRequested(request) {
            webViewRequests.queueRequest(webViewRequests.dialogQueue, request);
        }

        function onAuthenticationRequested(request) {
            webViewRequests.queueRequest(webViewRequests.loginQueue, request);
        }

        // nothing was saved and the page stays; the item reports no load failure for it
        function onResponseRefused(url, mimeType, download) {
            if (download) {
                //: Notification when a web page starts a download, which the remote cannot save
                ui.createNotification(qsTr("Downloads are not supported"), true);
            } else if (webViewRequests.isPdf(mimeType)) {
                //: Notification when a link of a web page opens a PDF document, which the remote cannot show
                ui.createNotification(qsTr("PDF documents cannot be shown here"), true);
            } else {
                //: Notification when a link of a web page opens a file the remote cannot show (not a web page or image)
                ui.createNotification(qsTr("This document cannot be shown here"), true);
            }
        }

        // window.open() and target=_blank: http and https load into this view without a word, like a link
        function onNewWindow(url, loaded) {
            if (!loaded) {
                //: Notification when a web page tries to open a new window with an address the remote cannot show
                ui.createNotification(qsTr("The page tried to open a link that cannot be shown"), true);
            }
        }
    }

    Components.WebViewDialog {
        id: webViewDialog
        onClosed: Qt.callLater(webViewRequests.showNextRequest)
    }

    Components.WebViewLogin {
        id: webViewLogin
        onClosed: Qt.callLater(webViewRequests.showNextRequest)
    }
}
