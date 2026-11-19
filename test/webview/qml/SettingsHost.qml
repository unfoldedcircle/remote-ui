// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Test host for Settings > Web view: the page as the settings menu loads it (qrc:/settings/settings/WebView.qml, with
// its Settings.Page button navigation), the web view test page it opens, and the ids profileRoot and keyboard of
// main.qml as far as the pages use them, resolved through the QML context chain.

import QtQuick 2.15

import "qrc:/settings/settings" as SettingsPages

Item {
    id: host
    width: 480; height: 850

    // main.qml's profileRoot: Settings.Page and the title bar call goBack()/goHome()
    QtObject {
        id: profileRoot
        objectName: "profileRoot"
        property int backCalls: 0
        function goBack() { backCalls++; }
        function goHome() {}
    }

    // main.qml's on-screen keyboard as far as the pages and their components read it
    QtObject {
        id: keyboard
        objectName: "keyboard"
        property bool active: false
        signal opened()
        signal closed()
        function show() {}
        function hide() {}
    }

    // main.qml's loading screen, which a popup list stops when it shows
    QtObject {
        id: loading
        function start() {}
        function stop() {}
    }

    // the settings overlay's background, which the settings pages draw on (Profile.qml)
    Rectangle {
        anchors.fill: parent
        color: colors.bg
    }

    SettingsPages.WebView {
        id: page
        objectName: "page"
        // the title of the menu entry, which Settings.qml passes
        topNavigationText: "Web view"
    }
}
