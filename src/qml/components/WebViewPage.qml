// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The web page of a web view entity, for the fullscreen web view (entities/webview/deviceclass/Webview.qml) and the
// web view widget of an activity page (WebViewWidget.qml): a WebView with the entity's start page, zoom, background
// colour and site settings (credentials, cookies, TLS exception, user agent: WebViewSite), and its reload interval.
//
// Imports UC.WebView, which a binary built with CONFIG+=no-webview lacks: load it with a Loader, and only while
// ui.webViewAvailable. The page is loaded when this item is created and unloaded when it goes (core-api
// entity_webview.md "Lifecycle").
//
// - A change of the start page or of the site settings (an option update of the entity) loads the start page again,
//   so that new credentials or cookies apply; a change of zoom or background colour applies to the page as it is.
// - zoomIn() and zoomOut() (the zoom functions of the buttons in the fullscreen web view) step the zoom from the
//   option's level through the WebView item's levels, for as long as the page is shown: the entity's options are not
//   written. zoomReset(), a new zoom option or another entity starts from the option again.
// - The reload interval runs while the page is on screen and the display is on; it starts over when the page shows
//   again. Its reloads show no load indicator.
// - WebViewController.reloadRequested() (the `reload` command, or a cleared website data) reloads the page.
// - The load indicator (WebViewLoadIndicator.qml, docs/webview.md "Load indicator"): what the WebView says to show while
//   the page loads, and a failed load's message, over the page. OK (the fullscreen web view's buttons, retry()) and a
//   tap on the message try again.
// - Keys on the focus chain: a widget (keysToPage false) takes every key, the activity's button mapping has them
//   already. In the fullscreen web view (keysToPage) its button mapping sends the remote's d-pad and OK to the page
//   (Webview.qml), so the WebView does not forward them once more; while the on-screen keyboard is up it does, as the
//   keys of the text field the user types into, and so it does with the keyboard's own Enter. Decided at the press, so
//   that a press, its repeats and its release go the same way.

import QtQuick 2.15

import Power 1.0
import WebView.Controller 1.0

import UC.WebView 1.0

import "qrc:/components" as Components

Item {
    id: webViewPage

    // the web view entity (entity::Webview)
    property QtObject entityObj
    // the zoom of a web view widget in percent (25..400), 0: the entity's
    property int zoomOverride: 0
    // the page is on screen: the reload interval runs only then
    property bool shown: true
    // the fullscreen web view, whose button mapping sends the remote's keys to the page; a widget leaves every key to the
    // activity's mapping
    property bool keysToPage: true

    readonly property alias webView: webView
    readonly property url startUrl: entityObj ? entityObj.url : ""
    // the zoom of the options in percent: the widget's (25..400), else the entity's, with its user override
    readonly property int optionZoom: zoomOverride >= 25 && zoomOverride <= 400
                                      ? zoomOverride : (entityObj ? entityObj.zoom : 100)
    // the zoom the keys stepped to (1 = 100 %), 0 until a key zoomed: then the option's zoom applies
    property real keyZoom: 0

    // One zoom level in or out from the zoom shown, through the levels of the WebView item (50 to 300 %, nothing beyond
    // either end), for this page only.
    function zoomIn() {
        webView.zoomIn();
        webViewPage.keyZoom = webView.zoom;
    }

    function zoomOut() {
        webView.zoomOut();
        webViewPage.keyZoom = webView.zoom;
    }

    // back to the option's zoom
    function zoomReset() {
        webViewPage.keyZoom = 0;
    }

    // An options update that leaves the zoom as it is (another option changed, the fetch of the entity when its screen
    // opens) keeps the keys' zoom: a QML property reports a change only when its value changes.
    onOptionZoomChanged: webViewPage.keyZoom = 0
    onEntityObjChanged: webViewPage.keyZoom = 0

    // the page asked to close its window (window.close()) and has no history to go back to
    signal closeRequested()

    // the failure message shows: OK tries again
    readonly property bool failed: webView.loadIndicator === WebView.LoadIndicatorFailed

    // the failure message's OK or tap: the page loads again (WebKit reloads the address that failed)
    function retry() {
        webView.reload();
    }

    // the remote's d-pad and OK (InputController), which the fullscreen web view's button mapping sends to the page
    function isMappedKey(key) {
        return key === Qt.Key_Up || key === Qt.Key_Down || key === Qt.Key_Left || key === Qt.Key_Right
                || key === Qt.Key_Return;
    }

    // Whether the WebView forwards a d-pad key or OK to the page, by key, as decided at its press: only while the
    // on-screen keyboard is up (the text field's keys), and not on a failed load, whose OK the button mapping takes.
    property var forwardedKeys: ({})

    function forwardsMappedKey(event, pressed) {
        if (!(event.key in webViewPage.forwardedKeys) || (pressed && !event.isAutoRepeat)) {
            webViewPage.forwardedKeys[event.key] = typeof keyboard !== "undefined" && keyboard.active
                    && !webViewPage.failed;
        }
        const forward = webViewPage.forwardedKeys[event.key];
        if (!pressed && !event.isAutoRepeat) {
            delete webViewPage.forwardedKeys[event.key];
        }
        return forward;
    }

    // the start page last loaded: an options update that leaves it as it is (zoom, a fetch of the entity when its
    // screen opens) loads nothing
    property url loadedStartUrl: ""

    function loadStartPage() {
        if (webViewPage.startUrl.toString() !== "") {
            webViewPage.loadedStartUrl = webViewPage.startUrl;
            webView.url = webViewPage.startUrl;
        }
    }

    onStartUrlChanged: {
        if (webViewPage.startUrl.toString() !== webViewPage.loadedStartUrl.toString()) {
            Qt.callLater(webViewPage.loadStartPage);
        }
    }
    // Qt.callLater runs a function once per event loop pass, however often it is asked: one load at the start
    Component.onCompleted: Qt.callLater(webViewPage.loadStartPage)

    Connections {
        target: webViewPage.entityObj
        ignoreUnknownSignals: true

        // new credentials, cookies, certificate exception or user agent apply to a fresh load of the start page
        function onSiteChanged() {
            Qt.callLater(webViewPage.loadStartPage);
        }
    }

    Connections {
        target: WebViewController

        function onReloadRequested(entityId) {
            if (webViewPage.entityObj && (entityId === "" || entityId === webViewPage.entityObj.id)) {
                webView.reload();
            }
        }
    }

    WebView {
        id: webView
        anchors.fill: parent
        site: webViewPage.entityObj.site
        // the option, else white (core-api "Display")
        backgroundColor: webViewPage.entityObj ? webViewPage.entityObj.backgroundColor : colors.white
        zoom: webViewPage.keyZoom > 0 ? webViewPage.keyZoom : webViewPage.optionZoom / 100

        // a touch gives the item the focus: without keysToPage the keys stop here, before the page and the
        // activity screen (whose ButtonNavigation has them already, docs/key-navigation.md section 1). With it, the
        // d-pad and OK stop here unless the on-screen keyboard is up: the screen's button mapping sends them (an
        // accepted event is not forwarded; any other key goes on to the item, which forwards the page's keys).
        Keys.onPressed: {
            if (!webViewPage.keysToPage) {
                event.accepted = true;
            } else if (webViewPage.isMappedKey(event.key)) {
                event.accepted = !webViewPage.forwardsMappedKey(event, true);
            } else {
                event.accepted = false;
            }
        }
        Keys.onReleased: {
            if (!webViewPage.keysToPage) {
                event.accepted = true;
            } else if (webViewPage.isMappedKey(event.key)) {
                event.accepted = !webViewPage.forwardsMappedKey(event, false);
            } else {
                event.accepted = false;
            }
        }

        // what the page shows where it paints nothing yet (before its first frame): the web view's background, the
        // option or white, below the frames (z -1), not the screen behind
        Rectangle {
            objectName: "webViewBackground"
            anchors.fill: parent
            z: -1
            color: webView.backgroundColor
        }

        // window.close() of the page: back in the history, else the web view closes
        onCloseRequested: {
            if (webView.canGoBack) {
                webView.goBack();
            } else {
                webViewPage.closeRequested();
            }
        }
    }

    Components.WebViewLoadIndicator {
        anchors.fill: webView
        spinnerShown: webView.loadIndicator === WebView.LoadIndicatorSpinner
        barShown: webView.loadIndicator === WebView.LoadIndicatorBar
        failureShown: webView.loadIndicator === WebView.LoadIndicatorFailed
        pageFailed: webView.pageFailed
        pageColor: webView.backgroundColor
        progress: webView.loadProgress
        okHint: webViewPage.keysToPage

        onRetry: webViewPage.retry()
    }

    Timer {
        id: reloadTimer
        objectName: "reloadTimer"
        interval: webViewPage.entityObj ? webViewPage.entityObj.reloadInterval * 1000 : 0
        running: interval > 0 && webViewPage.shown && Power.windowShown
        repeat: true
        // over a page that is shown: no load indicator
        onTriggered: webView.reloadQuietly()
    }
}
