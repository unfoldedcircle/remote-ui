// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Settings > Web view > Buttons: what the remote's buttons do in a full-screen web page, for all web views (core-api
// entity_webview.md "Button settings"). One row per button of this remote model (WebViewController.buttonIds) with
// its action under its name; OK or a tap opens the list of the actions: Normal function (`pass`), the functions of the
// web view, and Send key to page, which opens the list of the keys. Each list marks the default. A choice goes to the
// core (WebViewController.setButtonAction(), set_webview_cfg with that button only), and the row shows the core's
// answer; a change made in the web configurator shows at once. HOME, POWER and VOICE have a fixed function and are not
// listed. A web view can bring its own buttons (its `buttons` option), which go before these.
//
// Opened by WebView.qml over the whole screen, like the web view test page: opaque, and it takes the touches its rows
// leave. BACK and the arrow of its title bar close it (done()). The page uses the QML focus chain (ADR 0007): the d-pad
// walks the rows from top to bottom.

import QtQuick 2.15
import QtQuick.Layouts 1.15

import WebView.Controller 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: buttonsPage
    scrollTarget: flickable
    initialFocusItem: buttonRows.count > 0 ? buttonRows.itemAt(0).row : null

    // BACK or the title bar's arrow: the settings page that opened this one closes it
    signal done()

    // the button whose action list is open, and whose key list opens when that list has closed on Send key to page
    property string listButton: ""
    property bool keyListNext: false

    // Opaque, below everything of the page (z -1): a settings page draws no background of its own, and this one lies
    // over the Web view settings page. Touches that no row takes end here.
    Rectangle {
        objectName: "webViewButtonsBackground"
        anchors.fill: parent
        z: -1
        color: colors.bg

        MouseArea {
            anchors.fill: parent
        }
    }

    // the name of a button, as on the remote
    function buttonName(button) {
        switch (button) {
        //: Web view settings, Buttons: the BACK button of the remote
        case "BACK": return qsTr("Back", "remote button");
        //: Web view settings, Buttons: the up button of the remote's d-pad
        case "DPAD_UP": return qsTr("Up", "remote button");
        //: Web view settings, Buttons: the down button of the remote's d-pad
        case "DPAD_DOWN": return qsTr("Down", "remote button");
        //: Web view settings, Buttons: the left button of the remote's d-pad
        case "DPAD_LEFT": return qsTr("Left", "remote button");
        //: Web view settings, Buttons: the right button of the remote's d-pad
        case "DPAD_RIGHT": return qsTr("Right", "remote button");
        //: Web view settings, Buttons: the middle button of the remote's d-pad
        case "DPAD_MIDDLE": return qsTr("OK", "remote button");
        //: Web view settings, Buttons: the VOLUME + button of the remote
        case "VOLUME_UP": return qsTr("Volume up", "remote button");
        //: Web view settings, Buttons: the VOLUME - button of the remote
        case "VOLUME_DOWN": return qsTr("Volume down", "remote button");
        //: Web view settings, Buttons: the CHANNEL + button of the remote
        case "CHANNEL_UP": return qsTr("Channel up", "remote button");
        //: Web view settings, Buttons: the CHANNEL - button of the remote
        case "CHANNEL_DOWN": return qsTr("Channel down", "remote button");
        //: Web view settings, Buttons: the MUTE button of the remote
        case "MUTE": return qsTr("Mute", "remote button");
        //: Web view settings, Buttons: the PREV (previous track) button of the remote
        case "PREV": return qsTr("Previous", "remote button");
        //: Web view settings, Buttons: the PLAY / pause button of the remote
        case "PLAY": return qsTr("Play", "remote button");
        //: Web view settings, Buttons: the NEXT (next track) button of the remote
        case "NEXT": return qsTr("Next", "remote button");
        //: Web view settings, Buttons: the STOP button of the Remote 3
        case "STOP": return qsTr("Stop", "remote button");
        //: Web view settings, Buttons: the RECORD button of the Remote 3
        case "RECORD": return qsTr("Record", "remote button");
        //: Web view settings, Buttons: the MENU button of the Remote 3
        case "MENU": return qsTr("Menu", "remote button");
        //: Web view settings, Buttons: the red colour button of the Remote Two
        case "RED": return qsTr("Red", "remote button");
        //: Web view settings, Buttons: the green colour button of the Remote Two
        case "GREEN": return qsTr("Green", "remote button");
        //: Web view settings, Buttons: the yellow colour button of the Remote Two
        case "YELLOW": return qsTr("Yellow", "remote button");
        //: Web view settings, Buttons: the blue colour button of the Remote Two
        case "BLUE": return qsTr("Blue", "remote button");
        default: return button;
        }
    }

    // what a function of the web view does, as a short command
    function functionName(name) {
        switch (name) {
        //: Web view settings, Buttons: the button zooms the web page in one step
        case "zoom_in": return qsTr("Zoom in");
        //: Web view settings, Buttons: the button zooms the web page out one step
        case "zoom_out": return qsTr("Zoom out");
        //: Web view settings, Buttons: the button sets the zoom of the web page back to the web view's zoom
        case "zoom_reset": return qsTr("Reset zoom");
        //: Web view settings, Buttons: the button scrolls the web page up a little
        case "scroll_up": return qsTr("Scroll up");
        //: Web view settings, Buttons: the button scrolls the web page down a little
        case "scroll_down": return qsTr("Scroll down");
        //: Web view settings, Buttons: the button scrolls the web page up by one screen
        case "page_up": return qsTr("Page up");
        //: Web view settings, Buttons: the button scrolls the web page down by one screen
        case "page_down": return qsTr("Page down");
        //: Web view settings, Buttons: the button scrolls to the top of the web page
        case "top": return qsTr("Top of the page");
        //: Web view settings, Buttons: the button scrolls to the bottom of the web page
        case "bottom": return qsTr("Bottom of the page");
        //: Web view settings, Buttons: the button goes back to the previous web page
        case "back": return qsTr("Back", "web page");
        //: Web view settings, Buttons: the button goes forward to the next web page in the history
        case "forward": return qsTr("Forward", "web page");
        //: Web view settings, Buttons: the button goes back to the previous web page, or closes the web view on the first page
        case "back_or_close": return qsTr("Back or close");
        //: Web view settings, Buttons: the button loads the web page again
        case "reload": return qsTr("Reload", "web page");
        //: Web view settings, Buttons: the button stops loading the web page
        case "stop": return qsTr("Stop loading");
        //: Web view settings, Buttons: the button closes the web view
        case "close": return qsTr("Close", "web page");
        //: Web view settings, Buttons: the button shows or hides the bar with Back, Forward, Reload and Close
        case "toggle_navigation": return qsTr("Show or hide navigation");
        //: Web view settings, Buttons: the button moves the focus to the next element of the web page, like the Tab key
        case "focus_next": return qsTr("Next element");
        //: Web view settings, Buttons: the button moves the focus to the previous element of the web page, like Shift+Tab
        case "focus_previous": return qsTr("Previous element");
        //: Web view settings, Buttons: the button does nothing in a full-screen web page
        case "none": return qsTr("Nothing");
        default: return name;
        }
    }

    // the action of a button as its row shows it
    function actionName(action) {
        if (!action) {
            return "";
        }
        if (action.action === "function") {
            return buttonsPage.functionName(action["function"]);
        }
        if (action.action === "key") {
            //: Web view settings, Buttons: the button sends this key to the web page; %1 is the name of the key a web page receives (for example ArrowUp or ChannelUp), not translated
            return qsTr("Sends %1").arg(action.key);
        }
        //: Web view settings, Buttons: the button keeps its function without the web page, e.g. the button mapping of the activity
        return qsTr("Normal function");
    }

    // the value of an action in the action list: "pass", "function:<name>" or "key"
    function actionValue(action) {
        if (!action) {
            return "";
        }
        return action.action === "function" ? "function:" + action["function"] : action.action;
    }

    function openActionList(button) {
        const action = WebViewController.buttons[button];
        const defaultAction = WebViewController.defaultButtonAction(button);
        const defaultValue = buttonsPage.actionValue(defaultAction);
        //: Web view settings, Buttons: second line of the default action in the list of actions of a button
        const defaultText = qsTr("Default");
        listModel.clear();
        listModel.append({ "name": buttonsPage.actionName({ "action": "pass" }), "value": "pass",
                           "secondary": defaultValue === "pass" ? defaultText : "" });
        const functions = WebViewController.buttonFunctions;
        for (let i = 0; i < functions.length; i++) {
            const value = "function:" + functions[i];
            listModel.append({ "name": buttonsPage.functionName(functions[i]), "value": value,
                               "secondary": defaultValue === value ? defaultText : "" });
        }
        listModel.append({
            //: Web view settings, Buttons: the last entry of the list of actions of a button: opens the list of the keys the button can send to the web page
            "name": qsTr("Send key to page"),
            "value": "key",
            "secondary": action && action.action === "key" ? action.key : (defaultValue === "key" ? defaultText : "")
        });
        let current = 0;
        const currentValue = buttonsPage.actionValue(action);
        for (let i = 0; i < listModel.count; i++) {
            if (listModel.get(i).value === currentValue) {
                current = i;
            }
        }
        buttonsPage.listButton = button;
        buttonsPage.keyListNext = false;
        popupListLoader.setSource("qrc:/components/PopupList.qml", { "title": buttonsPage.buttonName(button),
                                      "listModel": listModel, "showSearch": false, "initialSelected": current,
                                      "currentValue": currentValue });
    }

    function openKeyList() {
        const button = buttonsPage.listButton;
        const action = WebViewController.buttons[button];
        const defaultAction = WebViewController.defaultButtonAction(button);
        const currentKey = action && action.action === "key" ? action.key : "";
        const keys = WebViewController.buttonKeys;
        let current = 0;
        listModel.clear();
        for (let i = 0; i < keys.length; i++) {
            listModel.append({ "name": keys[i], "value": keys[i],
                               //: Web view settings, Buttons: second line of the default key in the list of keys a button can send to the web page
                               "secondary": defaultAction.action === "key" && defaultAction.key === keys[i]
                                            ? qsTr("Default") : "" });
            if (keys[i] === currentKey) {
                current = i;
            }
        }
        popupListLoader.setSource("qrc:/components/PopupList.qml", {
                                      //: Web view settings, Buttons: title of the list of keys a button can send to the web page; %1 is the name of the button
                                      "title": qsTr("Key for %1").arg(buttonsPage.buttonName(button)),
                                      "listModel": listModel, "showSearch": false, "initialSelected": current,
                                      "currentValue": currentKey });
    }

    function selected(value) {
        const button = buttonsPage.listButton;
        if (value === "key") {
            // the key list follows once this list has closed
            buttonsPage.keyListNext = true;
        } else if (value === "pass") {
            WebViewController.setButtonAction(button, { "action": "pass" });
        } else if (value.indexOf("function:") === 0) {
            WebViewController.setButtonAction(button, { "action": "function", "function": value.substring(9) });
        } else {
            WebViewController.setButtonAction(button, { "action": "key", "key": value });
        }
    }

    // the d-pad from row to row; at either end the page scrolls on (Settings.Page)
    function focusRow(index, event) {
        const item = index >= 0 && index < buttonRows.count ? buttonRows.itemAt(index) : null;
        if (item) {
            item.row.forceActiveFocus();
            event.accepted = true;
        } else {
            event.accepted = false;
        }
    }

    Component.onCompleted: {
        buttonsPage.topNavigation.goBack = function() {
            buttonsPage.done();
        };
        buttonsPage.buttonNavigation.extendDefaultConfig({
            "BACK": {
                "pressed": function() {
                    buttonsPage.done();
                }
            }
        });
    }

    ListModel {
        id: listModel
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
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

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 10
                wrapMode: Text.WordWrap
                color: colors.textSecondary
                font: fonts.help()
                //: Web view settings, Buttons: the text above the list of the remote's buttons
                text: qsTr("What the buttons do in a full-screen web page, for all web views. A web view can change single buttons.")
            }

            Repeater {
                id: buttonRows
                model: WebViewController.buttonIds

                Column {
                    Layout.fillWidth: true
                    property alias row: buttonRow

                    Components.MenuRow {
                        id: buttonRow
                        objectName: "buttonRow_" + modelData
                        text: buttonsPage.buttonName(modelData)
                        description: buttonsPage.actionName(WebViewController.buttons[modelData])
                        chevron: true
                        selected: activeFocus
                        keypadActivatable: true
                        onClicked: buttonsPage.openActionList(modelData)

                        /** KEYBOARD NAVIGATION **/
                        Keys.onUpPressed: buttonsPage.focusRow(index - 1, event)
                        Keys.onDownPressed: buttonsPage.focusRow(index + 1, event)
                    }

                    Components.Divider {
                        visible: index < buttonRows.count - 1
                    }
                }
            }

            Item {
                Layout.preferredHeight: 20
            }
        }
    }

    Loader {
        id: popupListLoader
        anchors.fill: parent
        // above the title bar of this page
        z: 500

        Connections {
            target: popupListLoader.item
            ignoreUnknownSignals: true

            function onItemSelected(value) {
                buttonsPage.selected(value);
            }

            function onDone() {
                // the page takes the focus back on its own, on the row the user came from
                popupListLoader.source = "";
                if (buttonsPage.keyListNext) {
                    buttonsPage.keyListNext = false;
                    Qt.callLater(buttonsPage.openKeyList);
                }
            }
        }
    }
}
