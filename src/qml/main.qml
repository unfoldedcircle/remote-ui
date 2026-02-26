// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.VirtualKeyboard 2.3
import QtQuick.VirtualKeyboard.Settings 2.3
import QtQuick.Window 2.2

import Haptic 1.0
import HwInfo 1.0
import Config 1.0
import Battery 1.0
import Power 1.0
import Power.Modes 1.0
import SoundEffects 1.0
import SoftwareUpdate 1.0
import Entity.Controller 1.0
import Integration.Controller 1.0

import "qrc:/components" as Components
import "qrc:/settings/softwareupdate" as Softwareupdate
import "qrc:/components/entities/activity" as ActivityComponents

ApplicationWindow {
    id: applicationWindow
    objectName : "applicationWindow"
    title: "Remote Two simulator"
    visible: true

    minimumWidth: ui.width * ui.ratio
    maximumWidth: minimumWidth
    minimumHeight: ui.height * ui.ratio
    maximumHeight: minimumHeight
    color: colors.black

    Window {
        id: buttonSimulator
        visible: !ui.showRegulatoryInfo
        title: "Button simulator"
        color: colors.black
        minimumWidth: ui.width * ui.ratio
        maximumWidth: minimumWidth
        minimumHeight: ui.width * ui.ratio * 0.95
        maximumHeight: minimumHeight
        x: applicationWindow.height + buttonSimulator.height > Screen.desktopAvailableHeight ? applicationWindow.x+applicationWindow.width : applicationWindow.x
        y: applicationWindow.height + buttonSimulator.height > Screen.desktopAvailableHeight ? applicationWindow.y : applicationWindow.y + applicationWindow.height + 60

        Loader {
            anchors.fill: parent
            source: "qrc:/button-simulator/Buttons.qml"
            active: buttonSimulator.visible
        }
    }

    function loadSecondContainer(source, parameters = {}, openAfterLoad = true) {
        if (!containerSecond.loader.active) {
            console.debug("Loading second container", source);
            containerSecond.loader.openAfterLoad = openAfterLoad;
            containerSecond.loader.active = true;
            containerSecond.loader.setSource(source, parameters);
        }
    }

    function loadActivityToSecondContainer(entityObj) {
        ui.setTimeOut(1000, () => {
                          loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityObj.id, "entityObj": entityObj });
                      });
    }

    property bool isSecondContainerLoaded: containerSecond.loader.source != ""

    function loadThirdContainer(source, parameters = {}, openAfterLoad = true) {
        if (!containerThird.loaderThird.active) {
            console.debug("Loading third container", source);
            containerThird.loaderThird.openAfterLoad = openAfterLoad;
            containerThird.loaderThird.active = true;
            containerThird.loaderThird.setSource(source, parameters);
        }
    }

    function connectSignalSlot(sig, slot) {
        let slotConn = (...args) => {
            slot(...args);
            sig.disconnect(slotConn);
        }
        sig.connect(slotConn)
    }

    function checkActivityIncludedEntities(activityObj, onSequence = true) {
        // check if all entities in the activity has a connected integraiton
        let entityListToCheck = onSequence ? activityObj.onSequenceEntities : activityObj.offSequenceEntities;
        let allIncludedEntitiesConnected = true;
        let notReadyEntities = "";
        let notReadyEntityQty = 0;

        if (entityListToCheck.length === 0) {
            return {
                allIncludedEntitiesConnected: true,
                notReadyEntities: notReadyEntities,
                notReadyEntityQty: notReadyEntityQty
            }
        }

        for (let i = 0; i < entityListToCheck.length; i++) {
            const includedEntityObj = EntityController.get(entityListToCheck[i]);

            if (includedEntityObj) {
                const includedEntityIntegrationObj = IntegrationController.getModelItem(includedEntityObj.integrationId);
                if (includedEntityIntegrationObj) {
                    if (includedEntityIntegrationObj.state !== "connected") {
                        allIncludedEntitiesConnected = false;
                        notReadyEntities += includedEntityObj.name + ",  ";
                        notReadyEntityQty++;
                    } else if (includedEntityIntegrationObj.state === "connected") {
                        if (!includedEntityObj.enabled) {
                            allIncludedEntitiesConnected = false;
                            notReadyEntities += includedEntityObj.name + ",  ";
                            notReadyEntityQty++;
                        }
                    }
                }
            }
        }

        // chop the last comma
        notReadyEntities = notReadyEntities.slice(0, -3);

        return {
            allIncludedEntitiesConnected: allIncludedEntitiesConnected,
            notReadyEntities: notReadyEntities,
            notReadyEntityQty: notReadyEntityQty
        }
    }

    Connections {
        target: Power
        ignoreUnknownSignals: true

        function onPowerModeChanged(fromPowerMode, toPowerMode) {
            if (toPowerMode == PowerModes.Low_power && fromPowerMode == PowerModes.Idle) {
                applicationWindow.visible = false;
            }

            if (toPowerMode == PowerModes.Normal) {
                if (!applicationWindow.visible) {
                    applicationWindow.visible = true;
                }
            }
        }
    }

    Components.ButtonNavigation {
        overrideActive: true
        defaultConfig: {
            "POWER": {
                "pressed": function() {
                    if (!SoftwareUpdate.updateInProgress) {
                        powerOffButtonTimer.start();
                    }
                },
                "released": function() {
                    if (!SoftwareUpdate.updateInProgress) {
                        powerOffButtonTimer.stop();
                    }
                }
            },
            "VOICE": {
                "long_press": function() {
                    if (!isSecondContainerLoaded || (isSecondContainerLoaded && !root.isActivityOpen)) {
                        voice.start(Config.voiceAssistantId, Config.voiceAssistantProfileId);
                    }
                },
                "released": function() {
                    if (!isSecondContainerLoaded || (isSecondContainerLoaded && !root.isActivityOpen)) {
                        voice.stop();
                    }
                }
            }
        }
    }

    Timer {
        id: powerOffButtonTimer
        running: false
        repeat: false
        interval: 3000
        onTriggered: {
            powerOffLoader.active = true;
        }
    }


    Item {
        id: root
        objectName: "root"
        width: ui.width
        height: ui.height
        layer.enabled: true

        anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
        transformOrigin: Item.Center

        Behavior on anchors.verticalCenterOffset {
            PropertyAnimation { duration: 300; easing.type: Easing.OutExpo }
        }

        property alias containerMain: containerMain
        property alias containerMainItem: containerMain.item
        property alias activityLoading: activityLoading
        property alias loading: loading
        property alias volume: volume
        property alias keyboard: keyboard
        property alias keyboardInputField: keyboardInputField
        property bool isActivityOpen: false

        Loader {
            id: containerMain
            anchors.fill: parent
            asynchronous: true
        }

        Connections {
            target: ui
            ignoreUnknownSignals: true

            function onConfigLoaded() {
                if (!ui.isOnboarding) {
                    if (ui.pages.count === 0) {
                        containerMain.source = "qrc:/NoPage.qml";
                    } else {
                        containerMain.source = "qrc:/MainContainer.qml";
                    }
                }
            }

            function onIsOnboardingChanged() {
                if (!ui.isOnboarding) {
                    if (ui.pages.count === 0) {
                        containerMain.source = "qrc:/NoPage.qml";
                    } else {
                        containerMain.source = "qrc:/MainContainer.qml";
                    }
                } else {
                    containerMain.source = "qrc:/OnboardingContainer.qml";
                }
            }

            function onIsNoProfileChanged() {
                if (ui.isNoProfile && !ui.isOnboarding) {
                    loadingFirst.stop();

                    containerSecond.close();
                    containerThird.close();

                    if (ui.profiles.count === 0) {
                        containerMain.setSource("qrc:/components/ProfileAdd.qml", { state: "visible", noProfile: true })
                    } else {
                        containerMain.setSource("qrc:/components/ProfileSwitch.qml", { state: "visible", noProfile: true })
                    }
                }  else if (!ui.isNoProfile && !ui.isOnboarding) {
                    if (ui.pages.count === 0) {
                        containerMain.source = "qrc:/NoPage.qml";
                    } else {
                        containerMain.source = "qrc:/MainContainer.qml";
                    }
                }
            }
        }

        Connections {
            target: ui.pages
            ignoreUnknownSignals: true

            function onCountChanged() {
                if (!ui.isOnboarding) {
                    if (ui.pages.count === 0) {
                        containerMain.source = "qrc:/NoPage.qml";
                    } else {
                        containerMain.source = "qrc:/MainContainer.qml";
                    }
                }
            }
        }

        Popup {
            id: containerSecond
            objectName: "containerSecond"
            width: parent.width; height: parent.height
            modal: false
            closePolicy: Popup.NoAutoClose
            padding: 0

            property alias loader: loader

            Behavior on y {
                PropertyAnimation { duration: 300; easing.type: Easing.OutExpo }
            }

            background: Item {}

            SequentialAnimation {
                id: containerSecondHideAnimation
                running: false
                alwaysRunToEnd: true

                ParallelAnimation {
                    NumberAnimation { target: containerSecond; properties: "scale"; from: 1.0; to: 0.7; easing.type: Easing.OutExpo; duration: 300 }
                    NumberAnimation { target: containerSecond; properties: "x"; from: 0; to: -ui.width; easing.type: Easing.InExpo; duration: 300 }
                }
            }

            SequentialAnimation {
                id: containerSecondShowAnimation
                running: false
                alwaysRunToEnd: true

                PauseAnimation { duration: 200 }
                ParallelAnimation {
                    NumberAnimation { target: containerSecond; properties: "scale"; from: 0.7; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
                    NumberAnimation { target: containerSecond; properties: "x"; from: -ui.width; to: 0; easing.type: Easing.OutExpo; duration: 300 }
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
                    console.debug("Second container closed signal called");
                    loader.source = "";
                    loader.active = false
                    containerSecond.close();
                    root.isActivityOpen = false;
                }
            }
        }

        Popup {
            id: containerThird
            objectName: "containerThird"
            width: parent.width; height: parent.height
            modal: false
            closePolicy: Popup.NoAutoClose
            padding: 0
            x: ui.width
            scale: 0.7

            property alias loaderThird: loaderThird

            Behavior on y {
                PropertyAnimation { duration: 300; easing.type: Easing.OutExpo }
            }

            background: Item {}

            enter: Transition {
                SequentialAnimation {
                    PauseAnimation { duration: 200 }
                    ParallelAnimation {
                        NumberAnimation { properties: "scale"; from: 0.7; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
                        NumberAnimation { properties: "x"; from: ui.width; to: 0; easing.type: Easing.OutExpo; duration: 300 }
                    }
                }
            }

            exit: Transition {
                SequentialAnimation {
                    ParallelAnimation {
                        NumberAnimation { properties: "scale"; from: 1.0; to: 0.7; easing.type: Easing.OutExpo; duration: 300 }
                        NumberAnimation { properties: "x"; from: 0; to: ui.width; easing.type: Easing.InExpo; duration: 300 }
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
                    console.debug("Third container closed signal called");
                    containerThird.close();
                    containerSecondShowAnimation.start();
                }
            }
        }

        NoProfile {
            visible: ui.profile.id === "" && !ui.isOnboarding
        }

        ActivityComponents.LoadingScreen {
            id: activityLoading
        }

        Components.LoadingScreen {
            id: loading
            objectName: "loading"
        }

        Components.LoadingFirst {
            id: loadingFirst
        }

        Components.VolumeOverlay {
            id: volume
            anchors.centerIn: parent
        }

        Components.VoiceOverlay {
            id: voice
            anchors.centerIn: parent
        }

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
                target: Battery
                ignoreUnknownSignals: true

                function onPowerSupplyChanged(value) {
                    if (value) {
                        chargingScreenLoader.active = true;
                        SoundEffects.play(SoundEffects.BatteryCharge);
                    } else {
                        if (chargingScreenLoader.active) {
                            chargingScreenLoader.item.close();
                        }
                    }
                }
            }

            Connections {
                target: Power
                ignoreUnknownSignals: true

                function onPowerModeChanged(fromPowerMode, toPowerMode) {
                    if (toPowerMode === PowerModes.Normal && fromPowerMode !== PowerModes.Idle && Battery.isCharging && Battery.powerSupply) {
                        chargingScreenLoader.active = true;
                    }
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

        Loader {
            id: powerOffLoader
            anchors.fill: parent
            asynchronous: true
            active: false
            source: "qrc:/components/Poweroff.qml"

            onStatusChanged: {
                if (status == Loader.Ready) {
                    powerOffLoader.item.open();
                }
            }

            Connections {
                target: powerOffLoader.item
                ignoreUnknownSignals: true

                function onClosed() {
                    powerOffLoader.active = false;
                }
            }
        }

        Softwareupdate.UpdateProgress {
            id: updateProgress

            Connections {
                target: SoftwareUpdate
                ignoreUnknownSignals: true

                function onUpdateStarted() {
                    updateProgress.open();
                }
            }
        }

        Components.Notification {}
        Components.ActionableNotification {}

        Loader {
            id: remoteOpenLoader
            objectName: "remoteOpenLoader"
            anchors.fill: parent
            asynchronous: true
            active: false
            source: "qrc:/components/RemoteOpen.qml"

            onStatusChanged: {
                if (status == Loader.Ready) {
                    remoteOpenLoader.item.open();
                }
            }

            onActiveChanged: {
                if (active) {
                    keyboard.hide();
                }
            }
        }

        Rectangle {
            parent:keyboard
            width: keyboard.width
            height: keyboard.height
            color: colors.black
            opacity: ui.globalBrightness
            z: 5000
        }

        Rectangle {
            id: keyboardInputField
            parent: Overlay.overlay
            width: parent.width
            height: parent.height - keyboard.height + 10
            color: colors.black
            state: "hidden"
            z: 10000
            anchors.top: parent.top

            property QtObject originObj
            property alias keyboardInput: keyboardInput

            function show(obj, label = "") {
                if (keyboardInputField.state === "hidden") {
                    console.debug("Show input", obj, label);
                    keyboardInputField.originObj = obj;

                    keyboardInput.inputValue = obj.inputValue;
                    keyboardInput.inputField.placeholderText = obj.inputField.placeholderText;
                    keyboardInput.inputField.inputMethodHints = obj.inputField.inputMethodHints;
                    keyboardInput.errorMsg = obj.errorMsg;
                    keyboardInput.password = obj.password;

                    if (keyboardInput.password) {
                        keyboardInput.inputField.passwordMaskDelay = obj.inputField.passwordMaskDelay;

                    }

                    if (label !== "") {
                        keyboardInputLabel.text = label;
                        keyboardInputLabel.visible = true;
                    }

                    keyboardInputField.state = "visible";
                    ui.setTimeOut(200, () => { keyboardInput.focus(); });
                }
            }

            function hide() {
                if (keyboardInputField.state === "visible") {
                    console.debug("Hide input");
                    keyboardInputField.originObj.inputValue = keyboardInput.inputValue;

                    keyboardInputField.state = "hidden";
                    keyboardInputLabel.visible = false;
                }
            }

            states: [
                State {
                    name: "hidden"
                    PropertyChanges { target: keyboardInputField; anchors.topMargin: -keyboardInputField.height; opacity: 0; visible: false }
                },
                State {
                    name: "visible"
                    PropertyChanges { target: keyboardInputField; anchors.topMargin: 0; opacity: 1; visible: true }
                }
            ]
            transitions: [
                Transition {
                    from: "visible"
                    to: "hidden"
                    SequentialAnimation {
                        PropertyAnimation { target: keyboardInputField; properties: "anchors.topMargin, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: keyboardInputField; properties: "visible"; duration: 0 }
                        ScriptAction { script: buttonNavigation.releaseControl() }
                    }
                },
                Transition {
                    from: "hidden"
                    to: "visible"
                    SequentialAnimation {
                        PropertyAnimation { target: keyboardInputField; properties: "visible"; duration: 0 }
                        PropertyAnimation { target: keyboardInputField; properties: "anchors.topMargin, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        ScriptAction { script: buttonNavigation.takeControl() }
                    }
                }
            ]

            Components.ButtonNavigation {
                id: buttonNavigation
                defaultConfig: {
                    "HOME": {
                        "pressed": function() {
                            keyboard.hide();
                        }
                    },
                    "BACK": {
                        "pressed": function() {
                            keyboard.hide();
                        }
                    },
                    "DPAD_MIDDLE": {
                        "pressed": function() {
                            keyboard.hide();
                        }
                    }
                }
            }

            Text {
                id:  keyboardInputLabel
                width: parent.width
                height: visible ? implicitHeight : 0
                color: colors.offwhite
                anchors { top: parent.top; topMargin: 10 }
                maximumLineCount: 1
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                font: fonts.primaryFont(30)
                visible: false
            }

            Components.InputField {
                id: keyboardInput
                width: parent.width - 40
                anchors { top: keyboardInputLabel.bottom; topMargin: 10; horizontalCenter: parent.horizontalCenter }
            }

            Components.Button {
                text: qsTr("Done")
                width: parent.width - 40
                anchors { top: keyboardInput.bottom; topMargin: 20; horizontalCenter: parent.horizontalCenter }
                trigger: function() {
                    keyboard.hide();
                }
            }

            Rectangle {
                anchors.fill: parent
                color: colors.black
                opacity: ui.globalBrightness
            }
        }

        InputPanel {
            id: keyboard
            objectName: "keyboard"
            width: ui.width
            x: hiddenX
            y: hiddenY

            property int hiddenX
            property int hiddenY
            property int visibleX
            property int visibleY

            signal opened()
            signal closed()

            transformOrigin: Item.Center

            function show() {
                keyboard.active = true;
                keyboard.opened();
            }

            function hide() {
                if (keyboard.active) {
                    keyboard.active = false;
                    keyboard.closed();
                    keyboardInputField.hide();
                }
            }

            Connections{
                target: Qt.inputMethod

                function onVisibleChanged(){
                    if(!Qt.inputMethod.visible){
                        keyboard.hide();
                    } else {
                        keyboard.show();
                    }
                }
            }

            states: State {
                name: "visible"
                when: keyboard.active
                PropertyChanges {
                    target: keyboard
                    x: visibleX
                    y: visibleY
                }
            }
            transitions: Transition {
                id: inputPanelTransition
                from: ""; to: "visible"
                reversible: true
                ParallelAnimation {
                    NumberAnimation {
                        properties: "x, y"
                        duration: 300
                        easing.type: Easing.InOutExpo
                    }
                }
            }
        }

    } // end root

    Rectangle {
        anchors.fill: parent
        parent: Overlay.overlay
        color: colors.black
        opacity: ui.globalBrightness
        layer.enabled: true
        z: 4000

        Behavior on opacity {
            OpacityAnimator { duration: 300 }
        }
    }

    Component.onCompleted: {
        ui.inputController.setSource(applicationWindow);
        ui.inputController.activeController = containerMain;
        VirtualKeyboardSettings.locale = Qt.binding(function() { return Config.language })

        if (ui.isOnboarding) {
            loadingFirst.stop();
            containerMain.source = "qrc:/OnboardingContainer.qml";
        }
    }
}
