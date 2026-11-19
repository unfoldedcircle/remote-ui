// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Test host for the fullscreen web view of a web view entity: the screen as main.qml loads it
// (qrc:/components/entities/webview/deviceclass/Webview.qml, a BaseDetail), opened on its own, as from a profile page
// tile, or on top of an activity screen whose button mapping it inherits (main.qml openWebView()), and the web view
// widget of an activity page. The ids containerMain and keyboard and the function openWebView() are resolved by the
// components through the QML context chain, exactly as in main.qml.

import QtQuick 2.15

import "qrc:/components" as Components

Item {
    id: host
    width: 480; height: 850

    // main.qml's containerMain: BaseDetail hides and shows its item when a screen opens and closes
    QtObject {
        id: containerMain
        property Item item: mainScreen
    }

    // MainContainer.qml as far as the fullscreen web view uses it: the current page, whose header has a running
    // activity or a playing media player (Page.qml headerButtonConfig(); null with headerHasActivity false)
    Item {
        id: mainScreen
        property Item currentPage: headerPage
        states: [State { name: "hidden" }, State { name: "visible" }]

        Item {
            id: headerPage
            function headerButtonConfig() {
                if (!host.headerHasActivity) {
                    return null;
                }
                // like Page.qml's header: no CHANNEL, unless a test asks for it (headerMapsChannel)
                const config = {
                    "VOLUME_UP": { "pressed": function() { host.countMain("VOLUME_UP"); } },
                    "VOLUME_DOWN": { "pressed": function() { host.countMain("VOLUME_DOWN"); } },
                    "MUTE": { "pressed": function() { host.countMain("MUTE"); } },
                    "PLAY": { "pressed": function() { host.countMain("PLAY"); } }
                };
                if (host.headerMapsChannel) {
                    config["CHANNEL_UP"] = { "pressed": function() { host.countMain("CHANNEL_UP"); } };
                    config["CHANNEL_DOWN"] = { "pressed": function() { host.countMain("CHANNEL_DOWN"); } };
                }
                return config;
            }
        }
    }

    property bool headerHasActivity: true
    property bool headerMapsChannel: false
    property var mainPresses: ({})

    function countMain(key) {
        const next = Object.assign({}, host.mainPresses);
        next[key] = (next[key] || 0) + 1;
        host.mainPresses = next;
    }

    // how often the main screen's header mapping of a key ran
    function mainScreenPresses(key) {
        return host.mainPresses[key] || 0;
    }

    // main.qml's on-screen keyboard as far as the screen and its components read it
    QtObject {
        id: keyboard
        objectName: "keyboard"
        property bool active: false
        signal opened()
        signal closed()
        function show() {}
        function hide() {}
    }

    // An activity screen as Activity.qml maps its buttons (overrideConfig of its ButtonNavigation): every press is
    // counted by its key name.
    Item {
        id: activityScreen
        objectName: "activityScreen"
        anchors.fill: parent

        property var presses: ({})
        property alias buttonNavigation: activityNavigation

        function count(key) {
            const next = Object.assign({}, activityScreen.presses);
            next[key] = (next[key] || 0) + 1;
            activityScreen.presses = next;
        }

        Components.ButtonNavigation {
            id: activityNavigation
            overrideConfig: {
                "VOLUME_UP": { "pressed": function() { activityScreen.count("VOLUME_UP"); } },
                "VOLUME_DOWN": { "pressed": function() { activityScreen.count("VOLUME_DOWN"); } },
                "MUTE": { "pressed": function() { activityScreen.count("MUTE"); } },
                "CHANNEL_UP": { "pressed": function() { activityScreen.count("CHANNEL_UP"); } },
                "CHANNEL_DOWN": { "pressed": function() { activityScreen.count("CHANNEL_DOWN"); } },
                "DPAD_UP": { "pressed": function() { activityScreen.count("DPAD_UP"); } },
                "BACK": { "pressed": function() { activityScreen.count("BACK"); } },
                "PLAY": { "pressed": function() { activityScreen.count("PLAY"); } },
                "RED": { "pressed": function() { activityScreen.count("RED"); } },
                "MENU": { "pressed": function() { activityScreen.count("MENU"); } },
                "POWER": { "pressed": function() { activityScreen.count("POWER"); } }
            }
        }
    }

    Loader {
        id: screenLoader
        objectName: "screenLoader"
        anchors.fill: parent

        onLoaded: screenLoader.item.open(true)
    }

    // opens the fullscreen web view of the entity, on top of the activity screen when onActivity is set
    function openScreen(entityObj, onActivity) {
        const properties = { "entityId": entityObj.id, "entityObj": entityObj };
        if (onActivity) {
            properties["underlyingScreen"] = activityScreen;
        }
        screenLoader.setSource("qrc:/components/entities/webview/deviceclass/Webview.qml", properties);
        return screenLoader.item;
    }

    function closeScreen() {
        screenLoader.source = "";
    }

    // The web view widget of an activity page (WebViewWidget.qml) at the size of its fields in the page grid.
    Loader {
        id: widgetLoader
        objectName: "widgetLoader"
        x: 60; y: 100
    }

    function openWidget(entityId, interactive, width, height) {
        widgetLoader.setSource("qrc:/components/WebViewWidget.qml",
                               { "entityId": entityId, "interactive": interactive, "width": width, "height": height });
        return widgetLoader.item;
    }

    // main.qml's openWebView(): a tap on a widget that is not interactive opens the fullscreen web view
    property int fullscreenOpens: 0
    function openWebView(entityId) {
        host.fullscreenOpens++;
    }

    // how often the activity's mapping of a key ran
    function activityPresses(key) {
        return activityScreen.presses[key] || 0;
    }
}
