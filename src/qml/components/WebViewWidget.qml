// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The web view widget of an activity page (core-api ui-widgets.md "Web view"): the web page of a web view entity in
// the page grid, at least 3 x 3 fields, at most one per activity (the core checks both).
//
// - The page is loaded while the activity page with the widget is the current one and unloaded when it is left.
//   While a fullscreen web view is open the page is unloaded too and loaded again afterwards: the UI shows one web
//   page at a time.
// - interactive (default): touch goes to the page, a swipe on the widget scrolls the page, not the activity pages.
//   Not interactive: the page is only shown, a tap opens the web view fullscreen.
// - The remote's buttons keep the activity's button mapping: the widget takes no key, also while the page has the
//   focus after a touch.
// - zoom: the widget's page zoom in percent, else the effective zoom of the entity.
// - Without the web view, for an unavailable or deleted entity, or without a start page: an empty placeholder.

import QtQuick 2.15

import Entity.Controller 1.0
import WebView.Controller 1.0

import "qrc:/components" as Components

Item {
    id: webViewWidget
    width: 240
    height: 240

    property string entityId
    property QtObject entityObj: null
    property bool interactive: true
    // the widget's page zoom in percent, 0: the entity's
    property int zoom: 0
    // the activity page with the widget is the current one
    property bool pageShown: true

    readonly property bool pageAvailable: ui.webViewAvailable && entityObj !== null && entityObj.enabled
                                          && entityObj.url.toString() !== ""

    function ensureEntityLoaded() {
        webViewWidget.entityObj = EntityController.get(webViewWidget.entityId);
    }

    Component.onCompleted: ensureEntityLoaded()
    onEntityIdChanged: ensureEntityLoaded()

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onEntityLoaded(success, loadedId) {
            if (success && loadedId === webViewWidget.entityId) {
                webViewWidget.ensureEntityLoaded();
            }
        }
    }

    // the empty placeholder, also behind the page while it loads
    Rectangle {
        anchors.fill: parent
        radius: ui.cornerRadiusSmall
        color: colors.surface

        Components.Icon {
            anchors.centerIn: parent
            size: 80
            color: colors.textDisabled
            icon: webViewWidget.entityObj ? webViewWidget.entityObj.icon : "uc:globe"
            visible: !pageLoader.active
        }
    }

    Loader {
        id: pageLoader
        anchors.fill: parent
        active: webViewWidget.pageAvailable && webViewWidget.pageShown && WebViewController.fullscreenEntityId === ""
        source: "qrc:/components/WebViewPage.qml"

        onLoaded: {
            pageLoader.item.zoomOverride = Qt.binding(function() { return webViewWidget.zoom; });
            pageLoader.item.keysToPage = false;
            pageLoader.item.entityObj = Qt.binding(function() { return webViewWidget.entityObj; });
        }

        onActiveChanged: {
            // the activity page was left: the website data setting applies (a fullscreen web view applies it itself)
            if (!active && WebViewController.fullscreenEntityId === "") {
                WebViewController.viewClosed();
            }
        }
    }

    // not interactive: the page is only shown, a tap opens it fullscreen
    Components.HapticMouseArea {
        anchors.fill: parent
        enabled: !webViewWidget.interactive && webViewWidget.entityObj !== null
        visible: enabled
        onClicked: openWebView(webViewWidget.entityId)
    }
}
