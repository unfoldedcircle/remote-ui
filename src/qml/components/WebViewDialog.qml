// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// A web page's script dialog: alert(), confirm(), prompt() and the "leave this page?" question of a beforeunload
// handler, for a WebViewDialogRequest of the WebView item (libucwebview 0.7.0, docs/webview.md "Dialogs and logins").
//
// A bottom sheet like WifiJoin.qml, titled with the page's origin ("https://example.com says") so that the user knows
// which site asks. A prompt sits at the top with its text field, like WifiPassword.qml, so that the on-screen keyboard
// fits below. A long message scrolls inside the sheet.
//
// Keys (the dialog owns the input while it is open, so the page's keys, PREV/NEXT, VOLUME and CHANNEL, do nothing):
// - OK presses the selected button. When the dialog opens that is OK, on the leave-page question Stay (OK stays,
//   leaving needs a d-pad move to Leave first), on a prompt the text field, whose OK is the dialog's OK;
// - BACK and HOME answer no: alert closed, confirm() false, prompt() null, stay on the page (HOME does not go home,
//   like the Wi-Fi popups). So does a tap above the sheet (not on a prompt: the keyboard is
//   there);
// - LEFT and RIGHT move between the buttons, UP from the buttons of a prompt reaches its field; UP and DOWN that no
//   control takes scroll a long message.
// The request's cancelled() (the page went on, a navigation, a crash) closes the dialog without an answer.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import UC.WebView 1.0

import "qrc:/components" as Components

Popup {
    id: webViewDialog
    width: parent.width; height: parent.height
    opacity: 0
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0
    // the popup takes the keyboard focus: its controls are reached with the d-pad (WifiJoin.qml)
    focus: true

    // the request on screen, null when there is none: answered, cancelled, or freed with the web view (the property
    // is guarded and turns null then)
    property WebViewDialogRequest request: null

    // what the dialog shows, copied from the request: the item deletes a request after it ended, and the dialog still
    // fades out with its texts
    property int type: WebViewDialogRequest.Alert
    property string origin: ""
    property string message: ""

    readonly property bool isPrompt: type === WebViewDialogRequest.Prompt
    readonly property bool isLeavePage: type === WebViewDialogRequest.LeavePage
    readonly property bool hasTwoButtons: type !== WebViewDialogRequest.Alert

    // A long message scrolls above this height: the sheet keeps the page's top visible, a prompt keeps room for the
    // keyboard.
    readonly property int messageMaxHeight: isPrompt ? 120 : Math.max(120, height - 400)

    // the d-pad and the keyboard: the text field of a prompt, Stay on the leave-page question (an OK pressed out of
    // habit keeps the page and what the user typed into it), else OK
    readonly property Item initialFocusItem: isPrompt ? promptField.inputField : (isLeavePage ? noButton : yesButton)

    function show(dialogRequest) {
        webViewDialog.type = dialogRequest.type;
        webViewDialog.origin = dialogRequest.origin;
        webViewDialog.message = dialogRequest.message;
        promptField.inputValue = dialogRequest.type === WebViewDialogRequest.Prompt ? dialogRequest.defaultText : "";
        messageFlickable.contentY = 0;
        webViewDialog.request = dialogRequest;
        // a reopened dialog starts on its initial control again
        buttonNavigation.lastFocusItem = null;
        buttonNavigation.lastFocusAnchor = null;
        webViewDialog.open();
    }

    // UP on a button: back to the prompt's field, else the message scrolls. Handled by Keys before KeyNavigation: the
    // field's navDown links the buttons' `up` back to the field also when it is hidden (QQuickKeyNavigationAttached
    // sets the reverse link of a neighbour without its own), which would swallow the key in an alert.
    function upFromButtons() {
        if (webViewDialog.isPrompt) {
            promptField.focus();
        } else {
            messageFlickable.scrollBy(-1);
        }
    }

    // the user's answer: yes (OK, Leave) or no (Cancel, Stay, BACK)
    function answer(yes) {
        const answered = webViewDialog.request;
        // closes the dialog (onRequestChanged); the answer below goes to the page once, whatever comes after
        webViewDialog.request = null;
        if (!answered) {
            return;
        }
        if (yes) {
            answered.accept(webViewDialog.isPrompt ? promptField.inputValue : "");
        } else {
            answered.reject();
        }
    }

    onRequestChanged: {
        if (!webViewDialog.request && webViewDialog.visible) {
            webViewDialog.close();
        }
    }

    Connections {
        target: webViewDialog.request
        ignoreUnknownSignals: true

        // the page, a navigation or a crash ended the request: close without answering
        function onCancelled() {
            webViewDialog.request = null;
        }

        // answered elsewhere (the item answers a leave-page question before it navigates): nothing left to ask
        function onFinished() {
            webViewDialog.request = null;
        }
    }

    enter: Transition {
        ParallelAnimation {
            PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        }
    }

    exit: Transition {
        PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    onOpened: {
        buttonNavigation.takeControl();
        if (webViewDialog.isPrompt) {
            keyboard.show();
        }
    }

    onClosed: {
        buttonNavigation.releaseControl();
        if (webViewDialog.isPrompt) {
            keyboard.hide();
        }
        promptField.inputValue = "";
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        manageFocus: true
        initialFocusItem: webViewDialog.initialFocusItem
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    webViewDialog.answer(false);
                }
            },
            "HOME": {
                "pressed": function() {
                    webViewDialog.answer(false);
                }
            }
        }
    }

    background: Item {
        Rectangle {
            id: bg
            width: parent.width
            height: webViewDialog.isPrompt ? parent.height : sheet.height + 40
            color: colors.black
            anchors.bottom: parent.bottom
        }

        Item {
            width: parent.width; height: parent.height - bg.height
            anchors { bottom: bg.top; horizontalCenter: parent.horizontalCenter }
            visible: !webViewDialog.isPrompt

            LinearGradient {
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(0, parent.height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: colors.transparent }
                    GradientStop { position: 1.0; color: colors.black }
                }
            }
        }
    }

    // a tap above the sheet is a no, like BACK (WifiJoin closes there too); a tap on the sheet does nothing. Below the
    // sheet of a prompt is the keyboard: a tap that misses a key does nothing either.
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (!webViewDialog.isPrompt) {
                webViewDialog.answer(false);
            }
        }
    }

    Rectangle {
        id: sheet
        objectName: "webViewDialogSheet"
        width: parent.width
        height: content.height + 40
        color: colors.dark
        radius: ui.cornerRadiusSmall
        y: webViewDialog.isPrompt ? 0 : parent.height - height

        MouseArea {
            anchors.fill: parent
        }

        // UP and DOWN that no control took: the message scrolls
        Keys.onUpPressed: messageFlickable.scrollBy(-1)
        Keys.onDownPressed: messageFlickable.scrollBy(1)

        Column {
            id: content
            spacing: 20
            width: parent.width - 40
            anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }

            Text {
                id: titleText
                objectName: "webViewDialogTitle"
                width: parent.width
                // the origin is the page's text (a host name), never markup
                textFormat: Text.PlainText
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 3
                elide: Text.ElideRight
                color: colors.offwhite
                font: fonts.primaryFont(30)
                text: {
                    if (webViewDialog.isLeavePage) {
                        //: Title of the question a web page asks before the user leaves it (the page may have unsaved changes)
                        return qsTr("Leave this page?");
                    }
                    if (webViewDialog.origin !== "") {
                        //: Title of a message, question or text input a web page shows; %1 is the page's address, such as https://example.com
                        return qsTr("%1 says").arg(webViewDialog.origin);
                    }
                    //: Title of a message, question or text input of a web page that has no address of its own
                    return qsTr("This page says");
                }
            }

            // the site that asks the leave-page question (the other dialogs name it in the title)
            Text {
                id: originText
                objectName: "webViewDialogOrigin"
                width: parent.width
                visible: webViewDialog.isLeavePage && webViewDialog.origin !== ""
                textFormat: Text.PlainText
                elide: Text.ElideMiddle
                color: colors.light
                font: fonts.secondaryFont(22)
                text: webViewDialog.origin
            }

            Rectangle {
                width: parent.width
                height: 2
                color: colors.medium
            }

            Item {
                width: parent.width
                height: messageFlickable.height
                visible: messageText.text !== ""

                Flickable {
                    id: messageFlickable
                    objectName: "webViewDialogMessage"
                    width: parent.width
                    height: Math.min(contentHeight, webViewDialog.messageMaxHeight)
                    contentWidth: width
                    contentHeight: messageText.implicitHeight
                    interactive: contentHeight > height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    // one step of UP or DOWN: half the visible height
                    function scrollBy(direction) {
                        const maxContentY = Math.max(0, contentHeight - height);
                        contentY = Math.min(Math.max(0, contentY + direction * Math.round(height / 2)), maxContentY);
                    }

                    Text {
                        id: messageText
                        objectName: "webViewDialogMessageText"
                        // room for the scroll indicator
                        width: messageFlickable.width - 50
                        // the page's text as it wrote it: never interpreted as markup (no images, no links)
                        textFormat: Text.PlainText
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        color: colors.offwhite
                        font: fonts.secondaryFont(26)
                        //: Text of the question a web page asks before the user leaves it; pages cannot word it themselves
                        text: webViewDialog.isLeavePage ? qsTr("Changes you made may not be saved.") : webViewDialog.message
                    }
                }

                Components.ScrollIndicator {
                    parentObj: messageFlickable
                    padding: 0
                    hideOverride: !messageFlickable.interactive
                }
            }

            Components.InputField {
                id: promptField
                objectName: "webViewDialogField"
                width: parent.width
                visible: webViewDialog.isPrompt
                //: Placeholder of the text field of a web page's text input dialog
                inputField.placeholderText: qsTr("Your answer")
                inputField.onAccepted: webViewDialog.answer(true)
                // edit in place with the keyboard below, like WifiPassword.qml
                moveInput: false
                keyboardFollowsFocus: true
                navDown: yesButton
            }

            Item {
                width: parent.width
                height: yesButton.height

                Components.Button {
                    id: noButton
                    objectName: "webViewDialogNo"
                    visible: webViewDialog.hasTwoButtons
                    width: parent.width / 2 - 10
                    color: colors.secondaryButton
                    anchors { left: parent.left; bottom: parent.bottom }
                    text: webViewDialog.isLeavePage
                          //: Button of the question a web page asks before the user leaves it: stay on the page
                          ? qsTr("Stay")
                          //: Button of a web page's question or text input dialog that answers no
                          : qsTr("Cancel")
                    trigger: function() {
                        webViewDialog.answer(false);
                    }

                    KeyNavigation.right: yesButton
                    Keys.onUpPressed: webViewDialog.upFromButtons()
                    Keys.onDownPressed: messageFlickable.scrollBy(1)
                }

                Components.Button {
                    id: yesButton
                    objectName: "webViewDialogYes"
                    width: webViewDialog.hasTwoButtons ? parent.width / 2 - 10 : parent.width
                    anchors { right: parent.right; bottom: parent.bottom }
                    text: webViewDialog.isLeavePage
                          //: Button of the question a web page asks before the user leaves it: leave the page
                          ? qsTr("Leave")
                          //: Button of a web page's message, question or text input dialog that answers yes
                          : qsTr("OK")
                    trigger: function() {
                        webViewDialog.answer(true);
                    }

                    KeyNavigation.left: webViewDialog.hasTwoButtons ? noButton : null
                    Keys.onUpPressed: webViewDialog.upFromButtons()
                    Keys.onDownPressed: messageFlickable.scrollBy(1)
                }
            }
        }
    }
}
