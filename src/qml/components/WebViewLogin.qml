// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// The login a web server or proxy asks for (HTTP Basic, Digest, NTLM), for a WebViewAuthRequest of the WebView item
// (libucwebview 0.7.0, docs/webview.md "Dialogs and logins"). Laid out like WifiPassword.qml and WifiSetup.qml: the
// title with the server's address, the server's own text for the protected area (its realm, quoted, it is the
// server's text), a warning for a connection without encryption, user name and password fields with the on-screen
// keyboard below, Cancel and Sign in.
//
// Keys (the dialog owns the input while it is open): the d-pad walks user name, password, Cancel / Sign in; OK on the
// user name goes to the password, OK on the password signs in; BACK and HOME cancel (HOME does not go home, like the
// Wi-Fi popups). Cancel continues without credentials: the server's own page for a refused login loads, as in a desktop
// browser.
//
// The credentials live in the two fields only while the dialog is open: they go to the request on Sign in and the
// fields are emptied at once, also on Cancel and when the request goes away. Nothing logs them. The password field
// opens masked for every login, whatever its eye showed in the last one. A retry (the server
// rejected the last credentials) opens with the error under the password field. When it is the retry of the login
// just signed in to (same origin, realm and proxy), the user name stays filled in (the request's previousUser, kept by
// the WebView item until the retry, see WebViewRequests "Retry") and the empty password field has the focus;
// otherwise both fields are empty and the user name has the focus.

import QtQuick 2.15
import QtQuick.Controls 2.15

import UC.WebView 1.0

import "qrc:/components" as Components

Popup {
    id: webViewLogin
    width: parent.width; height: parent.height
    opacity: 0
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0
    focus: true

    // the request on screen, null when there is none (answered, cancelled, freed with the web view)
    property WebViewAuthRequest request: null

    // what the dialog shows, copied from the request (the item deletes a request after it ended)
    property string origin: ""
    property string realm: ""
    property bool proxy: false
    property bool insecure: false
    property bool retry: false

    // the retry of a login with its user name filled in: only the password is asked for again
    property bool askPasswordOnly: false

    function show(authRequest) {
        webViewLogin.origin = authRequest.origin;
        webViewLogin.realm = authRequest.realm;
        webViewLogin.proxy = authRequest.proxy;
        webViewLogin.insecure = authRequest.insecure;
        webViewLogin.retry = authRequest.retry;
        webViewLogin.clearFields();
        // every login opens with the password masked, a retry too: the field's eye (InputField.qml) sets echoMode
        // itself, which ends the binding below, and the dialog lives on from one login to the next
        passwordField.inputField.echoMode = TextInput.Password;
        userField.inputValue = authRequest.previousUser;
        webViewLogin.askPasswordOnly = authRequest.previousUser !== "";
        webViewLogin.request = authRequest;
        buttonNavigation.lastFocusItem = null;
        buttonNavigation.lastFocusAnchor = null;
        webViewLogin.open();
    }

    // empties both fields through their text, which also drops TextInput's undo history (internalSetText)
    function clearFields() {
        userField.inputValue = "";
        passwordField.inputValue = "";
    }

    function signIn() {
        const answered = webViewLogin.request;
        const user = userField.inputValue;
        const password = passwordField.inputValue;
        webViewLogin.clearFields();
        webViewLogin.request = null;
        if (answered) {
            answered.login(user, password);
        }
    }

    // continue without credentials: the server's page for a refused login loads
    function cancel() {
        const answered = webViewLogin.request;
        webViewLogin.clearFields();
        webViewLogin.request = null;
        if (answered) {
            answered.cancel();
        }
    }

    onRequestChanged: {
        if (!webViewLogin.request && webViewLogin.visible) {
            webViewLogin.clearFields();
            webViewLogin.close();
        }
    }

    Connections {
        target: webViewLogin.request
        ignoreUnknownSignals: true

        // the page went on or WebKit gave the login up: close without answering, and say why
        function onCancelled() {
            webViewLogin.request = null;
            //: Notification when a web page's login dialog closes because the page went on without the login
            ui.createNotification(qsTr("The page cancelled the sign-in"), false);
        }

        // answered elsewhere: nothing left to ask
        function onFinished() {
            webViewLogin.request = null;
        }
    }

    enter: Transition {
        PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    onOpened: {
        buttonNavigation.takeControl();
        keyboard.show();
        if (webViewLogin.retry) {
            //: Error under the password field when a web server rejected the user name or password the user entered
            passwordField.showError(qsTr("Wrong user name or password"));
        }
    }

    onClosed: {
        buttonNavigation.releaseControl();
        keyboard.hide();
        webViewLogin.clearFields();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        manageFocus: true
        initialFocusItem: webViewLogin.askPasswordOnly ? passwordField.inputField : userField.inputField
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    webViewLogin.cancel();
                }
            },
            "HOME": {
                "pressed": function() {
                    webViewLogin.cancel();
                }
            }
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: colors.black
    }

    // the page below stays out of reach while the login is open
    MouseArea {
        anchors.fill: parent
    }

    Column {
        id: content
        width: parent.width
        spacing: 10
        anchors { top: parent.top; topMargin: 10 }

        Text {
            id: titleText
            objectName: "webViewLoginTitle"
            width: parent.width
            // the address is the server's, never markup
            textFormat: Text.PlainText
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: 2
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            color: colors.offwhite
            font: fonts.primaryFont(26)
            text: webViewLogin.proxy
                  //: Title of the login dialog of a proxy server; %1 is the proxy's address, such as http://proxy:3128
                  ? qsTr("Sign in to the proxy %1").arg(webViewLogin.origin)
                  //: Title of the login dialog of a web site; %1 is the site's address, such as https://example.com
                  : qsTr("Sign in to %1").arg(webViewLogin.origin)
        }

        Text {
            id: realmText
            objectName: "webViewLoginRealm"
            width: parent.width
            visible: webViewLogin.realm !== ""
            // the server's own words: quoted and never markup
            textFormat: Text.PlainText
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: 2
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            color: colors.light
            font: fonts.secondaryFont(22)
            //: Line of the web login dialog with the server's own name for the protected area; %1 is that text
            text: qsTr("The site says: “%1”").arg(webViewLogin.realm)
        }

        Text {
            id: insecureText
            objectName: "webViewLoginInsecure"
            width: parent.width
            visible: webViewLogin.insecure
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            color: colors.red
            font: fonts.secondaryFont(22)
            //: Warning of the web login dialog when the site uses http instead of https
            text: qsTr("Not secure: this connection is not encrypted")
        }

        Components.InputField {
            id: userField
            objectName: "webViewLoginUser"
            width: parent.width
            //: Placeholder of the user name field of a web site's login dialog
            inputField.placeholderText: qsTr("User name")
            inputField.inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhSensitiveData
            inputField.onAccepted: passwordField.focus()
            moveInput: false
            keyboardFollowsFocus: true
            navDown: passwordField.inputField
        }

        Components.InputField {
            id: passwordField
            objectName: "webViewLoginPassword"
            width: parent.width
            //: Placeholder of the password field of a web site's login dialog
            inputField.placeholderText: qsTr("Password")
            inputField.inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhSensitiveData
            inputField.echoMode: TextInput.Password
            inputField.passwordMaskDelay: 1000
            inputField.onAccepted: webViewLogin.signIn()
            moveInput: false
            keyboardFollowsFocus: true
            navUp: userField.inputField
            navDown: signInButton
        }

        Item {
            width: parent.width
            height: signInButton.height + 30

            Components.Button {
                id: cancelButton
                objectName: "webViewLoginCancel"
                //: Button of a web site's login dialog: continue without signing in
                text: qsTr("Cancel")
                width: parent.width / 2 - 10
                color: colors.secondaryButton
                anchors { left: parent.left; bottom: parent.bottom }
                trigger: function() {
                    webViewLogin.cancel();
                }

                KeyNavigation.up: passwordField.inputField
                KeyNavigation.right: signInButton
            }

            Components.Button {
                id: signInButton
                objectName: "webViewLoginSignIn"
                //: Button of a web site's login dialog: send the user name and password
                text: qsTr("Sign in")
                width: parent.width / 2 - 10
                anchors { right: parent.right; bottom: parent.bottom }
                trigger: function() {
                    webViewLogin.signIn();
                }

                KeyNavigation.up: passwordField.inputField
                KeyNavigation.left: cancelButton
            }
        }
    }
}
