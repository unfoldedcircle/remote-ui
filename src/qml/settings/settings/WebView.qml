// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Settings > Web view: the global web view settings of the core (core-api entity_webview.md "Global web view
// settings"), which apply to every web view, the buttons that clear the cache and the website data of all web views,
// and the web view test page. Listed in the settings menu only when the firmware provides libucwebview
// (ui.webViewAvailable), see docs/webview.md.
//
// - Colour scheme and website data are picker rows (PopupList), invalid certificates a switch. A change goes to the
//   core (WebViewController, set_webview_cfg, admin scope) and the row shows the core's answer; a refused change is
//   told with a notification and the row keeps the setting as it is. A change made elsewhere (the web configurator)
//   shows at once (configuration_change).
// - Buttons opens the sub page of the remote's buttons in a full-screen web page (WebViewButtons.qml) over this page,
//   like the test page.
// - Clear cache and Clear website data clear the data of all web views, like the clear_cache and clear_data commands;
//   Clear website data asks first, because it signs the user out of pages with a cookie login. A notification tells
//   the outcome. Without a library that can clear (older than 0.8.0) both buttons are disabled and the help text says
//   why.
// - Web view test opens the test page (WebViewTest.qml) over this page, on the whole screen; its BACK closes it. The
//   test page and the buttons page are opaque and take the touches; this page's content and title bar are hidden while
//   one of them is open.
//
// The page uses the QML focus chain (ADR 0007): the d-pad walks the rows and buttons from top to bottom.

import QtQuick 2.15
import QtQuick.Layouts 1.15

import WebView.Controller 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: webViewSettings
    topNavigation.z: 400
    // hidden under the test page or the buttons page: nothing of this page shows through or takes input while one is
    // open
    topNavigation.visible: !webViewSettings.subPageOpen
    scrollTarget: flickable
    initialFocusItem: colorSchemeRow

    // the library can clear website data (library 0.8.0); the page exists only while the library is loaded
    readonly property bool clearSupported: WebViewController.canClearWebsiteData()
    // the test page or the buttons page is open over this page
    readonly property bool subPageOpen: subPageLoader.item !== null
    // the row that opened it, which takes the focus back when it closes
    property Item subPageRow: null
    // a clear runs: the buttons do nothing until its end. They stay enabled: a button that turns disabled under the
    // d-pad loses the focus, and the d-pad would be dead until the next touch.
    property bool clearing: false

    // The one handler for a selection in the popup list, set by the row that opens it and cleared when the popup closes
    // (Localisation.qml): a popup closed with BACK must not leave a handler for the next one.
    property var popupSelectionHandler: null

    function colorSchemeName(value) {
        if (value === "light") {
            //: Web view settings: the web pages are asked to use their light colour scheme
            return qsTr("Light");
        }
        //: Web view settings: the web pages are asked to use their dark colour scheme
        return qsTr("Dark");
    }

    function websiteDataName(value) {
        switch (value) {
        case "clear_cache_on_close":
            //: Web view settings: the cached files of the web pages are deleted whenever a web page is closed (a setting, not the button that clears them now)
            return qsTr("Clear cache", "website data on close");
        case "clear_all_on_close":
            //: Web view settings: all website data (cookies, cache, storage) is deleted whenever a web page is closed (a setting)
            return qsTr("Clear all", "website data on close");
        default:
            //: Web view settings: the website data (cookies, cache, storage) of the web pages is kept when a web page is closed (a setting)
            return qsTr("Keep", "website data on close");
        }
    }

    function openList(title, rows, currentValue, handler) {
        listModel.clear();
        let current = 0;
        for (let i = 0; i < rows.length; i++) {
            listModel.append(rows[i]);
            if (rows[i].value === currentValue) {
                current = i;
            }
        }
        webViewSettings.popupSelectionHandler = handler;
        popupListLoader.setSource("qrc:/components/PopupList.qml", { title: title, listModel: listModel,
                                      showSearch: false, initialSelected: current, currentValue: currentValue });
    }

    function openColorSchemeList() {
        //: Title of the list to choose the colour scheme of the web pages (Settings > Web view)
        webViewSettings.openList(qsTr("Select color scheme"), [
            { "name": webViewSettings.colorSchemeName("dark"), "value": "dark", "secondary": "" },
            { "name": webViewSettings.colorSchemeName("light"), "value": "light", "secondary": "" }
        ], WebViewController.colorScheme, function(value) {
            WebViewController.colorScheme = value;
        });
    }

    function openWebsiteDataList() {
        //: Title of the list to choose what happens with the website data when a web page is closed (Settings > Web view)
        webViewSettings.openList(qsTr("Website data on close"), [
            //: Web view settings: second line of "Keep" in the list of what happens with the website data when a web page closes; keep it short
            { "name": webViewSettings.websiteDataName("keep"), "value": "keep", "secondary": qsTr("Stay signed in to pages") },
            //: Web view settings: second line of "Clear cache" in the list of what happens with the website data when a web page closes; keep it short
            { "name": webViewSettings.websiteDataName("clear_cache_on_close"), "value": "clear_cache_on_close", "secondary": qsTr("Delete cached files") },
            //: Web view settings: second line of "Clear all" in the list of what happens with the website data when a web page closes: cookies and stored data are deleted too; keep it short
            { "name": webViewSettings.websiteDataName("clear_all_on_close"), "value": "clear_all_on_close", "secondary": qsTr("Signs you out of pages") }
        ], WebViewController.websiteData, function(value) {
            WebViewController.websiteData = value;
        });
    }

    function openSubPage(row, source) {
        webViewSettings.subPageRow = row;
        subPageLoader.setSource(source, { topNavigationText: Qt.binding(function() { return row.text; }) });
    }

    function clear(all) {
        if (webViewSettings.clearing) {
            return;
        }
        webViewSettings.clearing = true;
        WebViewController.clearData(all);
    }

    function confirmClearWebsiteData() {
        if (webViewSettings.clearing) {
            return;
        }
        ui.createActionableWarningNotification(
                    //: Title of the question before all website data of the web pages is deleted (Settings > Web view)
                    qsTr("Clear website data"),
                    //: Question before all website data of the web pages is deleted (Settings > Web view)
                    qsTr("This deletes the cookies, the cache and the stored data of all web pages. Pages you signed in to ask you to sign in again."),
                    "uc:triangle-exclamation",
                    function() {
                        webViewSettings.clear(true);
                    },
                    //: Button of the question before all website data of the web pages is deleted
                    qsTr("Clear"));
    }

    Connections {
        target: WebViewController

        function onDataCleared(all, result) {
            webViewSettings.clearing = false;
            if (result === WebViewController.Cleared) {
                ui.createNotification(all
                                      //: Notification: all website data of the web pages was deleted (Settings > Web view)
                                      ? qsTr("Website data cleared")
                                      //: Notification: the cache of the web pages was deleted (Settings > Web view)
                                      : qsTr("Cache cleared"), false);
            } else if (result === WebViewController.Unsupported) {
                //: Notification: the firmware's web view cannot delete website data
                ui.createNotification(qsTr("Clearing website data is not supported by this firmware"), true);
            } else {
                ui.createNotification(all
                                      //: Notification: deleting all website data of the web pages failed (Settings > Web view)
                                      ? qsTr("The website data could not be cleared")
                                      //: Notification: deleting the cache of the web pages failed (Settings > Web view)
                                      : qsTr("The cache could not be cleared"), true);
            }
        }
    }

    ListModel {
        id: listModel
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        visible: !webViewSettings.subPageOpen
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        boundsBehavior: Flickable.StopAtBounds

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 0
            width: parent.width

            /** COLOR SCHEME **/
            Components.MenuRow {
                id: colorSchemeRow
                objectName: "colorSchemeRow"
                //: Web view settings: the colour scheme (light or dark) the web pages are asked to use
                text: qsTr("Color scheme")
                //: Web view settings: second line of "Color scheme": only web pages with a light and a dark design follow the setting; keep it short (one line)
                description: qsTr("For pages that support it")
                value: webViewSettings.colorSchemeName(WebViewController.colorScheme)
                chevron: true
                selected: activeFocus
                keypadActivatable: true
                onClicked: webViewSettings.openColorSchemeList()

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.down: websiteDataRow
            }

            Components.Divider {}

            /** WEBSITE DATA ON CLOSE **/
            Components.MenuRow {
                id: websiteDataRow
                objectName: "websiteDataRow"
                //: Web view settings: what happens with the website data (cookies, cache, storage) when a web page is closed; the row shows Keep, Clear cache or Clear all
                text: qsTr("Website data on close")
                //: Web view settings: second line of "Website data on close": what is deleted when a web page closes (nothing, the cache or all website data); keep it short (one line)
                description: qsTr("What closing a page deletes")
                value: webViewSettings.websiteDataName(WebViewController.websiteData)
                chevron: true
                selected: activeFocus
                keypadActivatable: true
                onClicked: webViewSettings.openWebsiteDataList()

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: colorSchemeRow
                KeyNavigation.down: buttonsRow
            }

            Components.Divider {}

            /** BUTTONS **/
            Components.MenuRow {
                id: buttonsRow
                objectName: "buttonsRow"
                //: Web view settings: opens the page of what the remote's buttons do in a full-screen web page
                text: qsTr("Buttons")
                //: Web view settings: second line of "Buttons"; keep it short (one line)
                description: qsTr("In a full-screen web page")
                chevron: true
                selected: activeFocus
                keypadActivatable: true
                onClicked: webViewSettings.openSubPage(buttonsRow, "qrc:/settings/settings/WebViewButtons.qml")

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: websiteDataRow
                KeyNavigation.down: certificatesSwitch
            }

            Components.Divider {}

            /** INVALID CERTIFICATES **/
            Components.SettingRow {
                //: Web view settings: switch that lets every web page load with a self-signed, expired or otherwise invalid TLS certificate
                title: qsTr("Allow invalid certificates")
                //: Web view settings: help text of "Allow invalid certificates"
                help: qsTr("Accept self-signed, expired and other invalid certificates on all web pages. Their connections are then not protected against attackers.")

                Components.Switch {
                    id: certificatesSwitch
                    objectName: "certificatesSwitch"
                    icon: "uc:check"
                    checked: WebViewController.allowInvalidCertificates
                    trigger: function() {
                        WebViewController.allowInvalidCertificates = !WebViewController.allowInvalidCertificates;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.up: buttonsRow
                    KeyNavigation.down: clearCacheButton
                }
            }

            /** CLEAR **/
            Components.SettingRow {
                //: Web view settings: section with the buttons that delete the cache and the website data of all web pages
                title: qsTr("Cache and website data")
                help: webViewSettings.clearSupported
                      //: Web view settings: help text of the buttons that delete the cache and the website data
                      ? qsTr("All web pages share their cache, cookies and stored data.")
                      //: Web view settings: the firmware's web view cannot delete the cache and the website data
                      : qsTr("Clearing needs a newer firmware.")
                controlBelow: true

                Column {
                    width: parent.width
                    spacing: 20

                    Components.Button {
                        id: clearCacheButton
                        objectName: "clearCacheButton"
                        width: parent.width
                        variant: "secondary"
                        //: Web view settings: button that deletes the cached files of all web pages
                        text: qsTr("Clear cache")
                        enabled: webViewSettings.clearSupported
                        trigger: function() {
                            webViewSettings.clear(false);
                        }

                        /** KEYBOARD NAVIGATION **/
                        KeyNavigation.up: certificatesSwitch
                        KeyNavigation.down: clearDataButton
                    }

                    Components.Button {
                        id: clearDataButton
                        objectName: "clearDataButton"
                        width: parent.width
                        variant: "destructive"
                        //: Web view settings: button that deletes the cookies, cache and stored data of all web pages
                        text: qsTr("Clear website data")
                        enabled: webViewSettings.clearSupported
                        trigger: webViewSettings.confirmClearWebsiteData

                        /** KEYBOARD NAVIGATION **/
                        KeyNavigation.up: clearCacheButton
                        KeyNavigation.down: testPageRow
                    }
                }
            }

            /** TEST PAGE **/
            Components.MenuRow {
                id: testPageRow
                objectName: "testPageRow"
                Layout.bottomMargin: 20
                //: Web view settings: opens a test page to load any web address in the web view
                text: qsTr("Web view test")
                //: Web view settings: second line of "Web view test", which opens a page with an address field; keep it short (one line)
                description: qsTr("Load any web address to try it")
                chevron: true
                selected: activeFocus
                keypadActivatable: true
                onClicked: webViewSettings.openSubPage(testPageRow, "qrc:/settings/settings/WebViewTest.qml")

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: clearDataButton
            }
        }
    }

    Loader {
        id: popupListLoader
        anchors.fill: parent
        // above the raised title bar of this page
        z: 500

        Connections {
            target: popupListLoader.item
            ignoreUnknownSignals: true

            function onItemSelected(value) {
                if (webViewSettings.popupSelectionHandler) {
                    webViewSettings.popupSelectionHandler(value);
                }
            }

            function onDone() {
                webViewSettings.popupSelectionHandler = null;
                // the page takes the focus back on its own, on the row the user came from
                popupListLoader.source = "";
            }
        }
    }

    // The test page or the buttons page on the whole screen, over this page: it takes the input when it is loaded and
    // gives it back when it goes (the input controller's stack). The Loader is a focus scope and keeps the active focus
    // when its page goes, which this page's button navigation counts as a control of the page: the row that opened the
    // page takes the focus back itself.
    Loader {
        id: subPageLoader
        anchors.fill: parent
        z: 500

        onLoaded: subPageLoader.item.buttonNavigation.takeControl()

        Connections {
            target: subPageLoader.item
            ignoreUnknownSignals: true

            function onDone() {
                const row = webViewSettings.subPageRow;
                subPageLoader.source = "";
                if (row) {
                    row.forceActiveFocus();
                }
            }
        }
    }
}
