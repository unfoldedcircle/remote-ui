// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Test host for the screen stack of main.qml: the real main screen (MainContainer.qml) in containerMain, and copies of
// main.qml's second and third containers, which show the profile page, the entity screens and an entity screen on
// top of another screen, and of its charging screen loader. The ids are resolved by the screens through the QML
// context chain, exactly as in main.qml.

import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 480; height: 850

    property alias containerMain: containerMain
    property alias containerMainItem: containerMain.item
    property bool isActivityOpen: false

    // main.qml loadSecondContainer()
    function loadSecondContainer(source, parameters = {}, openAfterLoad = true) {
        if (String(source) === "") {
            return;
        }

        if (!containerSecond.loader.active) {
            containerSecond.loader.openAfterLoad = openAfterLoad;
            containerSecond.loader.active = true;
            containerSecond.loader.setSource(source, parameters);
        }
    }

    // main.qml loadThirdContainer(): a screen on top of the one in the second container, like an entity of an
    // activity's menu
    function loadThirdContainer(source, parameters = {}, openAfterLoad = true) {
        if (String(source) === "") {
            return;
        }

        if (!containerThird.loaderThird.active) {
            containerThird.loaderThird.openAfterLoad = openAfterLoad;
            containerThird.loaderThird.active = true;
            containerThird.loaderThird.setSource(source, parameters);
        }
    }

    // an entity screen as a page tile opens it (Page.qml, entities/Base.qml), or a command of the core on the main
    // screen
    function openEntityScreen(entityObj) {
        loadSecondContainer("qrc:/components/entities/button/deviceclass/Button.qml",
                            { "entityId": entityObj.id, "entityObj": entityObj });
    }

    // an entity screen on top of the screen that is open
    function openEntityScreenOnTop(entityObj) {
        loadThirdContainer("qrc:/components/entities/button/deviceclass/Button.qml",
                           { "entityId": entityObj.id, "entityObj": entityObj });
    }

    // the profile page as the status bar's profile icon or the page menu opens it
    function openProfilePage() {
        loadSecondContainer("qrc:/components/Profile.qml");
    }

    function showChargingScreen() {
        chargingScreenLoader.active = true;
    }

    readonly property Item secondScreen: containerSecond.loader.item
    readonly property Item thirdScreen: containerThird.loaderThird.item
    readonly property bool secondActive: containerSecond.loader.active
    readonly property bool thirdActive: containerThird.loaderThird.active
    // a Popup, not an Item
    readonly property QtObject chargingScreen: chargingScreenLoader.item

    Loader {
        id: containerMain
        anchors.fill: parent
        source: "qrc:/MainContainer.qml"
    }

    // main.qml containerSecond
    Popup {
        id: containerSecond
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0

        property alias loader: loader

        background: Item {}

        SequentialAnimation {
            id: containerSecondHideAnimation
            running: false
            alwaysRunToEnd: true

            ParallelAnimation {
                NumberAnimation { target: containerSecond; properties: "scale"; from: 1.0; to: 0.7; easing.type: Easing.OutExpo; duration: 300 }
                NumberAnimation { target: containerSecond; properties: "x"; from: 0; to: -root.width; easing.type: Easing.InExpo; duration: 300 }
            }
        }

        SequentialAnimation {
            id: containerSecondShowAnimation
            running: false
            alwaysRunToEnd: true

            PauseAnimation { duration: 200 }
            ParallelAnimation {
                NumberAnimation { target: containerSecond; properties: "scale"; from: 0.7; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
                NumberAnimation { target: containerSecond; properties: "x"; from: -root.width; to: 0; easing.type: Easing.OutExpo; duration: 300 }
            }
        }

        Loader {
            id: loader
            anchors.fill: parent
            asynchronous: true
            active: false

            property bool openAfterLoad: false

            onActiveChanged: {
                if (active) {
                    containerSecond.open();
                }
            }

            onStatusChanged: {
                if (status == Loader.Ready && loader.openAfterLoad) {
                    loader.item.open();
                }
            }
        }

        Connections {
            target: loader.item
            ignoreUnknownSignals: true

            function onClosed() {
                loader.source = "";
                loader.active = false
                containerSecond.close();
                root.isActivityOpen = false;
            }
        }
    }

    // main.qml containerThird
    Popup {
        id: containerThird
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        x: root.width
        scale: 0.7

        property alias loaderThird: loaderThird

        background: Item {}

        enter: Transition {
            SequentialAnimation {
                PauseAnimation { duration: 200 }
                ParallelAnimation {
                    NumberAnimation { properties: "scale"; from: 0.7; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
                    NumberAnimation { properties: "x"; from: root.width; to: 0; easing.type: Easing.OutExpo; duration: 300 }
                }
            }
        }

        exit: Transition {
            SequentialAnimation {
                ParallelAnimation {
                    NumberAnimation { properties: "scale"; from: 1.0; to: 0.7; easing.type: Easing.OutExpo; duration: 300 }
                    NumberAnimation { properties: "x"; from: 0; to: root.width; easing.type: Easing.InExpo; duration: 300 }
                }
                PropertyAction { target: loaderThird; property: "source"; value: "" }
                PropertyAction { target: loaderThird; property: "active"; value: false }
            }
        }

        Loader {
            id: loaderThird
            anchors.fill: parent
            asynchronous: true
            active: false

            property bool openAfterLoad: false

            onActiveChanged: {
                if (active) {
                    containerThird.open();
                }
            }

            onStatusChanged: {
                if (status == Loader.Ready && loaderThird.openAfterLoad) {
                    loaderThird.item.open(true);
                    containerSecondHideAnimation.start();
                }
            }
        }

        Connections {
            target: loaderThird.item
            ignoreUnknownSignals: true

            function onClosed() {
                containerThird.close();
                containerSecondShowAnimation.start();
            }
        }
    }

    // main.qml chargingScreenLoader, shown by the test instead of the power supply
    Loader {
        id: chargingScreenLoader
        anchors.fill: parent
        asynchronous: true
        active: false
        source: "qrc:/components/ChargingScreen.qml"

        onStatusChanged: {
            if (status == Loader.Ready) {
                item.open();
            }
        }

        Connections {
            target: chargingScreenLoader.item
            ignoreUnknownSignals: true

            function onClosed() {
                chargingScreenLoader.active = false;
            }
        }
    }

    // main.qml's on-screen keyboard, its input field and loading screen as far as the screens read them
    QtObject {
        id: keyboard
        property bool active: false
        signal opened()
        signal closed()
        function show() {}
        function hide() {}
    }

    QtObject {
        id: keyboardInputField
        function show(obj, label) {}
        function hide() {}
    }

    QtObject {
        id: loading
        function start() {}
        function stop() {}
        function success() {}
        function failure() {}
    }
}
