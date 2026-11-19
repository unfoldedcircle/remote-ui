// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The fullscreen web view of a web view entity (core-api entity_webview.md "Fullscreen web view"): opened by a tap on
// the entity's tile, a tap on a web view widget that is not interactive, or the `open` command (main.qml
// openWebView()). The page covers the whole screen; with the show_navigation option a bar at the bottom has Back,
// Forward, Reload and Close.
//
// Buttons (core-api entity_webview.md "Buttons"; the screen owns the input, docs/webview.md "Buttons of the fullscreen
// web view"): every button of the remote model that a mapping names (WebViewController.buttonIds) has the action of
// the web view's own buttons (entityObj.buttons, its user override over its option), else of the global setting
// (WebViewController.buttons, every button with its configured or default action):
// - `key`: the key goes to the page (WebView.sendKey()), pressed, repeated while held, released. By default the d-pad
//   sends the arrow keys, OK Enter and MENU ContextMenu;
// - `function`: a function of the web view (runFunction()). The scroll and page functions repeat while held, the others
//   act once per press. By default BACK closes, VOLUME zooms and CHANNEL scrolls a screen;
// - `pass`: the button keeps its function in the layer below: on top of an activity its button mapping, opened from a
//   profile page what the page header's activity or media player does with it (Page.qml headerButtonConfig(), looked
//   up at every press), else nothing; BACK without such a function closes the web view. The main screen's own d-pad
//   and CHANNEL (between its tiles and groups) are not passed on: the tiles are hidden behind the web view.
// A change of an action applies at once, also while the web view is open: keyConfig() is a binding.
// Fixed: HOME closes the web view and, when it was opened on top of another screen, that screen too; POWER, VOICE and
// every other button the mapping does not name keep the button mapping of the screen below.
// While the on-screen keyboard is up the d-pad and OK go to the text field of the page whatever their action: the
// WebView forwards them on the focus chain (WebViewPage.qml), as it does with the keyboard's own Enter. While a failed
// load shows its message, OK tries again. The page's dialogs and logins (WebViewRequests.qml) own the keys while they
// are open; the zoom functions do nothing while the on-screen keyboard is up either (a relayout would move the field
// the user types into).
//
// The web view itself is in WebViewPage.qml and WebViewRequests.qml, loaded only while ui.webViewAvailable: they
// import UC.WebView, which a build with CONFIG+=no-webview lacks.

import QtQuick 2.15

import WebView.Controller 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: webviewScreen

    // main.qml finds an open fullscreen web view by this
    readonly property bool isWebViewScreen: true
    // the screen this one was opened on top of (an activity screen), whose button mapping the other buttons keep
    property Item underlyingScreen: null
    // HOME was pressed: main.qml closes the screen below as well
    property bool closeToHome: false

    readonly property Item webView: pageLoader.item ? pageLoader.item.webView : null
    readonly property bool hasStartPage: entityObj && entityObj.url.toString() !== ""
    // the navigation bar: the show_navigation option, until a button with toggle_navigation shows or hides it for the
    // visit (navigationByButton true or false)
    property var navigationByButton: undefined
    readonly property bool navigationShown: navigationByButton !== undefined ? navigationByButton
                                                                              : !!(entityObj && entityObj.showNavigation)

    // Another web view while this one is open (the `open` command): it replaces the page, the screen stays.
    function showEntity(obj) {
        if (!obj || obj.id === webviewScreen.entityId) {
            return;
        }
        webviewScreen.entityId = obj.id;
        webviewScreen.entityObj = obj;
        webviewScreen.navigationByButton = undefined;
        WebViewController.fullscreenEntityId = obj.id;
    }

    function focusPage() {
        if (webviewScreen.webView) {
            webviewScreen.webView.forceActiveFocus();
        }
    }

    function goHome() {
        webviewScreen.closeToHome = true;
        webviewScreen.close();
    }

    function requestShown() {
        return requestsLoader.item !== null && requestsLoader.item.shown;
    }

    // the on-screen keyboard is up: the user types into a text field of the page
    function typing() {
        return typeof keyboard !== "undefined" && keyboard.active;
    }

    function closeScreen() {
        if (webviewScreen.state == "open") {
            webviewScreen.close();
        }
    }

    // zoom_in (1), zoom_out (-1), zoom_reset (0): one zoom level in or out (WebViewPage.qml) or back to the entity's
    // zoom, and the label with the level, also at the first or last level, where nothing changes
    function zoomPage(step) {
        if (!webviewScreen.webView || webviewScreen.typing()) {
            return;
        }
        if (step > 0) {
            pageLoader.item.zoomIn();
        } else if (step < 0) {
            pageLoader.item.zoomOut();
        } else {
            pageLoader.item.zoomReset();
        }
        zoomLabel.hold();
    }

    // A function of the web view (core-api WebviewButtonFunction); without a page only close, back_or_close and
    // toggle_navigation have something to do.
    function runFunction(name) {
        const view = webviewScreen.webView;
        switch (name) {
        case "zoom_in":
            webviewScreen.zoomPage(1);
            break;
        case "zoom_out":
            webviewScreen.zoomPage(-1);
            break;
        case "zoom_reset":
            webviewScreen.zoomPage(0);
            break;
        case "scroll_up":
            if (view) view.scrollUp();
            break;
        case "scroll_down":
            if (view) view.scrollDown();
            break;
        case "page_up":
            if (view) view.pageUp();
            break;
        case "page_down":
            if (view) view.pageDown();
            break;
        case "top":
            if (view) view.scrollToTop();
            break;
        case "bottom":
            if (view) view.scrollToBottom();
            break;
        case "back":
            if (view && view.canGoBack) view.goBack();
            break;
        case "forward":
            if (view && view.canGoForward) view.goForward();
            break;
        case "back_or_close":
            if (view && view.canGoBack) {
                view.goBack();
            } else {
                webviewScreen.closeScreen();
            }
            break;
        case "reload":
            if (view) view.reload();
            break;
        case "stop":
            if (view) view.stop();
            break;
        case "close":
            webviewScreen.closeScreen();
            break;
        case "toggle_navigation":
            webviewScreen.navigationByButton = !webviewScreen.navigationShown;
            break;
        case "focus_next":
            if (view) view.focusNext();
            break;
        case "focus_previous":
            if (view) view.focusPrevious();
            break;
        default:
            // none
            break;
        }
    }

    // A button with `pass`, opened from a profile page: the mapping of the page header's current activity or media
    // player, looked up at the press (the header changes while the web view is open). false without one, or for a
    // button it does not map.
    function mainScreenButton(key, type) {
        const main = containerMain.item;
        const page = main && main.currentPage ? main.currentPage : null;
        const config = page && page.headerButtonConfig ? page.headerButtonConfig() : null;
        if (config && config[key] && config[key][type]) {
            config[key][type]();
            return true;
        }
        return false;
    }

    // What the press of a mapped button did, by button, for its repeats and its release: {did, action}. did is "key"
    // (a key went down in the page), "function", "pass" (to the page header), "page" (the d-pad while the user types,
    // which the WebView forwards itself), "retry" or "nothing".
    property var pressedButtons: ({})

    function pressButton(button, action) {
        let did = "nothing";
        if (button.indexOf("DPAD_") === 0 && webviewScreen.typing()) {
            did = "page";
        } else if (button === "DPAD_MIDDLE" && pageLoader.item && pageLoader.item.failed) {
            pageLoader.item.retry();
            did = "retry";
        } else if (action.action === "pass") {
            if (!webviewScreen.mainScreenButton(button, "pressed") && button === "BACK") {
                webviewScreen.closeScreen();
            }
            did = "pass";
        } else if (webviewScreen.requestShown()) {
            // a dialog of the page fades in, and takes the keys then
        } else if (action.action === "key") {
            if (webviewScreen.webView) {
                webviewScreen.webView.sendKey(action.key, true);
                did = "key";
            }
        } else if (action.action === "function") {
            webviewScreen.runFunction(action["function"]);
            did = "function";
        }
        webviewScreen.pressedButtons[button] = { "did": did, "action": action };
    }

    function repeatButton(button) {
        const pressed = webviewScreen.pressedButtons[button];
        if (!pressed) {
            return;
        }
        if (pressed.did === "key") {
            if (webviewScreen.webView && !webviewScreen.requestShown()) {
                webviewScreen.webView.sendKey(pressed.action.key, true);
            }
        } else if (pressed.did === "function") {
            if (WebViewController.buttonRepeats(pressed.action) && !webviewScreen.requestShown()) {
                webviewScreen.runFunction(pressed.action["function"]);
            }
        } else if (pressed.did === "pass") {
            // a mapping without a repeat of its own repeats its press, as ButtonNavigation does
            if (!webviewScreen.mainScreenButton(button, "pressed_repeat")) {
                webviewScreen.mainScreenButton(button, "pressed");
            }
        }
    }

    function releaseButton(button) {
        const pressed = webviewScreen.pressedButtons[button];
        delete webviewScreen.pressedButtons[button];
        if (!pressed) {
            return;
        }
        if (pressed.did === "key") {
            if (webviewScreen.webView) {
                webviewScreen.webView.sendKey(pressed.action.key, false);
            }
        } else if (pressed.did === "pass") {
            webviewScreen.mainScreenButton(button, "released");
        }
    }

    // `pass` on top of a screen: that screen's mapping of the button as it is, with its own repeat and long press; the
    // d-pad not while the user types (its keys go to the text field then), OK not on a failed load (it tries again)
    function belowConfig(button, entry) {
        if (button.indexOf("DPAD_") !== 0) {
            return entry;
        }
        const guarded = {};
        for (const type in entry) {
            const handler = entry[type];
            guarded[type] = function() {
                if (webviewScreen.typing()) {
                    return;
                }
                if (button === "DPAD_MIDDLE" && pageLoader.item && pageLoader.item.failed) {
                    if (type === "pressed") {
                        pageLoader.item.retry();
                    }
                    return;
                }
                handler();
            };
        }
        return guarded;
    }

    // The key configuration of the screen: the buttons of the mapping with their actions, HOME, and the other buttons of
    // the screen below (POWER, VOICE, ...). A binding: a new action, of the web view or of the global setting, applies
    // at the next press.
    function keyConfig() {
        const config = {};
        const below = webviewScreen.underlyingScreen && webviewScreen.underlyingScreen.buttonNavigation
                ? webviewScreen.underlyingScreen.buttonNavigation.overrideConfig : null;
        const mapped = WebViewController.buttonIds;
        const own = webviewScreen.entityObj ? webviewScreen.entityObj.buttons : ({});
        const global = WebViewController.buttons;
        if (below) {
            for (const key in below) {
                if (key !== "HOME" && mapped.indexOf(key) < 0) {
                    config[key] = below[key];
                }
            }
        }
        // By index: on Qt 5.15.2 a garbage collection during a for-of loop over a C++ list (buttonIds is a QStringList)
        // corrupts the JavaScript heap; the loop then yields other values, or the UI crashes.
        for (let i = 0; i < mapped.length; ++i) {
            const button = mapped[i];
            const action = own[button] || global[button];
            if (!action) {
                continue;
            }
            if (action.action === "pass" && below) {
                // nothing below maps it: no entry, the screen's own default applies (BACK closes)
                if (below[button]) {
                    config[button] = webviewScreen.belowConfig(button, below[button]);
                }
                continue;
            }
            config[button] = {
                "pressed": function() { webviewScreen.pressButton(button, action); },
                "pressed_repeat": function() { webviewScreen.repeatButton(button); },
                "released": function() { webviewScreen.releaseButton(button); }
            };
        }
        config["HOME"] = {
            "pressed": function() { if (webviewScreen.state == "open") webviewScreen.goHome(); },
            "long_press": function() { if (webviewScreen.state == "open") webviewScreen.goHome(); }
        };
        return config;
    }

    overrideConfig: keyConfig()

    Component.onCompleted: {
        // the bar has its own close button, the page covers the whole screen
        iconClose.visible = false;
        WebViewController.fullscreenEntityId = webviewScreen.entityId;
        // BACK closes the screen at the press, without the long press of BaseDetail's default: with one, every mapped
        // BACK would act at its release only, and a held one close the web view after 800 ms. Only the default while
        // the entity is unavailable, else the button mapping's BACK.
        webviewScreen.buttonNavigation.extendDefaultConfig({
            "BACK": {
                "pressed": function() { webviewScreen.closeScreen(); }
            }
        });
    }

    onStateChanged: {
        if (webviewScreen.state == "open") {
            Qt.callLater(webviewScreen.focusPage);
        }
    }

    function releaseFullscreen() {
        if (WebViewController.fullscreenEntityId === webviewScreen.entityId) {
            WebViewController.fullscreenEntityId = "";
        }
    }

    onClosed: {
        webviewScreen.releaseFullscreen();
        WebViewController.viewClosed();
    }

    // dropped without closing (another screen replaced it, main.qml unloadSecondAndThirdContainer())
    Component.onDestruction: webviewScreen.releaseFullscreen()

    Loader {
        id: pageLoader
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: navigationBar.top }
        active: ui.webViewAvailable && webviewScreen.hasStartPage
        source: "qrc:/components/WebViewPage.qml"

        onLoaded: {
            pageLoader.item.entityObj = Qt.binding(function() { return webviewScreen.entityObj; });
            pageLoader.item.shown = Qt.binding(function() { return webviewScreen.state == "open"; });
            if (webviewScreen.state == "open") {
                Qt.callLater(webviewScreen.focusPage);
            }
        }

        Connections {
            target: pageLoader.item
            ignoreUnknownSignals: true

            function onCloseRequested() {
                //: Notification when the web page shown in the user interface closed itself (window.close())
                ui.createNotification(qsTr("The web page closed itself"), false);
                webviewScreen.close();
            }
        }
    }

    // the zoom level after VOLUME, at the top of the page
    Components.WebViewZoomLabel {
        id: zoomLabel
        anchors { top: pageLoader.top; topMargin: 20; horizontalCenter: pageLoader.horizontalCenter }
        webView: webviewScreen.webView
    }

    // without the web view or without a start page there is nothing to load
    Text {
        anchors { centerIn: pageLoader; margins: 20 }
        width: parent.width - 40
        visible: !pageLoader.active
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: colors.textSecondary
        font: fonts.prose()
        text: !ui.webViewAvailable
              //: Fullscreen web view of a web view entity, on a remote whose firmware cannot show web pages
              ? qsTr("This remote cannot show web pages")
              //: Fullscreen web view of a web view entity whose web address is missing or invalid
              : qsTr("The web page has no valid address")
    }

    Rectangle {
        id: navigationBar
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: webviewScreen.navigationShown ? 100 : 0
        visible: webviewScreen.navigationShown
        color: colors.bg

        Row {
            anchors { fill: parent; margins: 10 }
            spacing: 10

            Repeater {
                model: [
                    { "name": "back", "icon": "uc:arrow-left" },
                    { "name": "forward", "icon": "uc:arrow-right" },
                    { "name": "reload", "icon": "uc:arrow-rotate-right" },
                    { "name": "close", "icon": "uc:xmark" }
                ]

                Components.Button {
                    id: navigationButton
                    objectName: "navigation_" + modelData.name
                    width: (navigationBar.width - 20 - 3 * 10) / 4
                    height: 80
                    text: ""
                    variant: "secondary"
                    enabled: {
                        switch (modelData.name) {
                        case "back":
                            return webviewScreen.webView !== null && webviewScreen.webView.canGoBack;
                        case "forward":
                            return webviewScreen.webView !== null && webviewScreen.webView.canGoForward;
                        case "reload":
                            return webviewScreen.webView !== null;
                        default:
                            return true;
                        }
                    }
                    trigger: function() {
                        switch (modelData.name) {
                        case "back":
                            webviewScreen.webView.goBack();
                            break;
                        case "forward":
                            webviewScreen.webView.goForward();
                            break;
                        case "reload":
                            webviewScreen.webView.reload();
                            break;
                        default:
                            webviewScreen.close();
                            break;
                        }
                        // the d-pad stays with the page
                        if (modelData.name !== "close") {
                            webviewScreen.focusPage();
                        }
                    }

                    Components.Icon {
                        anchors.centerIn: parent
                        size: 60
                        // inverted on the pressed button, like a label (docs/design-system.md section 6)
                        color: navigationButton.pressed ? colors.bg : colors.textPrimary
                        icon: modelData.icon
                    }
                }
            }
        }
    }

    // the page's dialogs and logins over the whole screen
    Loader {
        id: requestsLoader
        anchors.fill: parent
        active: pageLoader.status === Loader.Ready
        source: "qrc:/components/WebViewRequests.qml"

        onLoaded: {
            requestsLoader.item.webView = Qt.binding(function() { return webviewScreen.webView; });
        }
    }
}
