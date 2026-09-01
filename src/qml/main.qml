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
        // Without this the simulator window takes the window manager focus, the main window never
        // gets an active focus item, and every key-navigated settings page appears dead on the
        // desktop simulator even though it works on the device (which has no second window).
        flags: Qt.Window | Qt.WindowDoesNotAcceptFocus
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

    /**
      Drops whatever the second and third containers are showing, without playing their close
      animations. Used when a screen has to be replaced by something else right away.
      */
    function unloadSecondAndThirdContainer() {
        if (containerThird.loaderThird.active) {
            containerThird.close();
            containerThird.loaderThird.source = "";
            containerThird.loaderThird.active = false;
        }

        if (containerSecond.loader.active) {
            containerSecond.close();
            containerSecond.loader.source = "";
            containerSecond.loader.active = false;
            root.isActivityOpen = false;
        }
    }

    /**
      Brings up an activity screen on top of everything else, replacing the currently open screens.
      */
    function openActivity(entityObj) {
        // already showing this activity and nothing on top of it: leave it alone
        if (containerSecond.loader.item && containerSecond.loader.item.entityId === entityObj.id
                && !containerThird.loaderThird.active) {
            return;
        }

        unloadSecondAndThirdContainer();
        loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityObj.id, "entityObj": entityObj });
    }

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

    // Runs the readiness check of a sequence command in the core and calls callback(summary).
    //
    // summary is null when the check could not be made at all - no connection to the core, a core that does not
    // know the request, an error or an answer that takes too long: the check tells the user what is wrong, it
    // never decides whether the command may run.
    function requestSequenceReadiness(entityId, cmdId, callback) {
        const checkId = EntityController.checkSequenceReadiness(entityId, cmdId);

        if (checkId < 0) {
            callback(null);
            return;
        }

        function handler(id, result) {
            // several checks can be in flight at once, e.g. when turning off every activity at once
            if (id !== checkId) {
                return;
            }

            EntityController.sequenceReadinessResult.disconnect(handler);
            callback(result.supported ? result : null);
        }

        EntityController.sequenceReadinessResult.connect(handler);
    }

    function readinessTitle(summary) {
        return summary.verdict === "not_ready"
            //: Title of the popup shown when an activity cannot start because a device blocks it. %1 is the
            //: name of the activity.
            ? qsTr("%1 is not ready").arg(summary.name)
            //: Title of the popup shown when an activity starts, but some of its devices will not react.
            : qsTr("Some devices are not ready");
    }

    // One sentence per reported cause. The report's own message is an English diagnostic and is never shown: the
    // sentence is composed from the reason code and the names in it, which the core already translated.
    function readinessCauseText(cause) {
        switch (cause.code) {
        case "INTEGRATION_NOT_CONNECTED":
            //: %1 is the name of an integration, e.g. "Denon AVR".
            return qsTr("%1 is not connected").arg(cause.integrationName);
        case "INTEGRATION_DISABLED":
            //: %1 is the name of an integration, e.g. "Denon AVR".
            return qsTr("The %1 integration is disabled").arg(cause.integrationName);
        case "INTEGRATION_NOT_FOUND":
            //: %1 is the name of a device.
            return qsTr("%1 has no integration").arg(cause.entityName);
        case "IR_EMITTER_NOT_AVAILABLE":
            //: %1 is the name of a dock.
            return cause.dockName !== "" ? qsTr("The IR emitter of %1 is not available").arg(cause.dockName)
                //: %1 is the name of an IR emitter.
                : (cause.emitterName !== "" ? qsTr("IR emitter %1 is not available").arg(cause.emitterName)
                                            : qsTr("An IR emitter is not available"));
        case "REMOTE_IR_OUTPUT_MISSING":
            //: %1 is the name of an IR remote.
            return qsTr("%1 has no IR output configured").arg(cause.entityName);
        case "REMOTE_IR_OUTPUT_INVALID":
            //: %1 is the name of an IR remote.
            return qsTr("The IR output of %1 is not available").arg(cause.entityName);
        case "BT_NOT_CONNECTED":
            return qsTr("Bluetooth is not connected");
        case "ENTITY_UNAVAILABLE":
            //: %1 is the name of a device.
            return qsTr("%1 is not available").arg(cause.entityName);
        case "ENTITY_NOT_FOUND":
            //: %1 is the name of a device that was deleted after the activity was set up.
            return qsTr("%1 no longer exists").arg(cause.entityName);
        case "SEQUENCE_ALREADY_RUNNING":
            //: %1 is the name of an activity or macro that is already running.
            return cause.entityName !== "" ? qsTr("%1 is already running").arg(cause.entityName)
                                           : qsTr("The sequence is already running");
        default:
            // NESTING_LIMIT, INVALID_COMMAND and codes a newer core may add
            //: Fallback for a problem this version has no wording for. %1 is the name of a device.
            return cause.entityName !== "" ? qsTr("%1 cannot run right now").arg(cause.entityName)
                                           : qsTr("A step cannot run right now");
        }
    }

    // The worst causes first, then the hint naming the button that runs the sequence anyway.
    function readinessMessage(summary) {
        const maxCauses = 3;
        let lines = [];

        for (let i = 0; i < summary.causes.length && i < maxCauses; i++) {
            lines.push(readinessCauseText(summary.causes[i]));
        }

        if (summary.causes.length > maxCauses) {
            //: Stands for the problems that did not fit in the popup, e.g. "+2 more issues".
            lines.push(qsTr("+%n more issue(s)", "", summary.causes.length - maxCauses));
        }

        lines.push(summary.verdict === "not_ready"
            //: Last line of the popup. "Proceed" is the button label.
            ? qsTr("The activity would stop at a blocked step. Tap Proceed to try anyway.")
            //: Last line of the popup. "Proceed" is the button label.
            : qsTr("Tap Proceed to continue anyway."));

        return lines.join("\n");
    }

    // keyed by activity id and direction: the readiness check of a sequence that is already waiting
    property var activityReadinessWaits: ({})

    // Runs the readiness check of an activity and calls proceed(activityObj) once its sequence may run.
    //
    // A wakeup brings the integrations back one by one and the core reports it only once it is through, so
    // an activity started by the very button press that woke the remote regularly finds all of its devices
    // still disconnected. The sequence waits for them for as long as the resume window configured under
    // Power lasts - the same budget EntityController gives a single entity command - and only asks the user
    // whether to proceed anyway once that budget is spent.
    // title overrides the heading of the prompt, for callers that would otherwise raise several
    // indistinguishable ones at once
    function checkActivityReadiness(activityObj, onSequence, proceed, title = "") {
        if (!activityObj) {
            return;
        }

        const activityId = activityObj.id;
        const cmdId = onSequence ? "activity.on" : "activity.off";
        const key = activityId + (onSequence ? ".on" : ".off");

        // a second press while the first one is still waiting would wait on its own and run the sequence
        // one more time once the devices are back
        if (applicationWindow.activityReadinessWaits[key]) {
            return;
        }

        // Eligibility is sampled once, when the sequence is requested: the wait exists for the devices that
        // a wakeup took away, not for the ones that are down for good.
        const waitForDevices = EntityController.resumePending;
        // provisional as long as the remote is still waking up, extended below once the window opens
        let deadline = Date.now() + EntityController.resumeTimeout;
        let windowSeen = EntityController.resumeWindow;

        function step() {
            const obj = EntityController.get(activityId);

            if (!obj) {
                // the activity is gone, there is nothing left to start
                delete applicationWindow.activityReadinessWaits[key];
                return;
            }

            // nothing is waking up and the user turned the check off for this activity: don't spend a request
            // on a report that would not be shown
            if (!obj.readyCheck && !waitForDevices) {
                delete applicationWindow.activityReadinessWaits[key];
                proceed(obj);
                return;
            }

            requestSequenceReadiness(activityId, cmdId, function(summary) {
                const current = EntityController.get(activityId);

                if (!current) {
                    delete applicationWindow.activityReadinessWaits[key];
                    return;
                }

                if (summary && summary.verdict === "ready") {
                    delete applicationWindow.activityReadinessWaits[key];
                    proceed(current);
                    return;
                }

                // the wakeup is reported after the fact, so the window opens while we are already waiting: give
                // the sequence the full window from that point, the way EntityController extends its commands
                if (!windowSeen && EntityController.resumeWindow) {
                    windowSeen = true;
                    deadline = Math.max(deadline, Date.now() + EntityController.resumeTimeout);
                }

                // Not ready, or the check itself failed - right after a wakeup that usually means the connection
                // to the core is still coming back. Both are worth waiting out, for as long as the resume budget
                // lasts: waiting past a lost connection is pointless, nothing can be sent over it and the devices
                // cannot come back before it does.
                if (waitForDevices && EntityController.resumePending && ui.coreConnected && Date.now() < deadline) {
                    applicationWindow.activityReadinessWaits[key] = true;
                    ui.setTimeOut(1000, step);
                    return;
                }

                delete applicationWindow.activityReadinessWaits[key];

                // no report to show, or the user turned the check off: the check never holds the sequence back
                if (!summary || !current.readyCheck) {
                    proceed(current);
                    return;
                }

                ui.createActionableNotification(title !== "" ? title : readinessTitle(summary),
                                                readinessMessage(summary),
                                                "uc:link-slash",
                                                () => {
                                                    // the activity may be gone by the time the user answers
                                                    const target = EntityController.get(activityId);

                                                    if (target) {
                                                        proceed(target);
                                                    }
                                                },
                                                //: Button label, quoted by name in the "not ready" messages.
                                                qsTr("Proceed"));
            });
        }

        step();
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

    Connections {
        target: ui.inputController
        ignoreUnknownSignals: true

        function onGlobalPowerLongPressed() {
            if (!SoftwareUpdate.updateInProgress) {
                powerOffLoader.active = true;
            }
        }
    }

    Components.ButtonNavigation {
        overrideActive: true
        defaultConfig: {
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

    Item {
        id: root
        objectName: "root"
        width: ui.width
        height: ui.height

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
                if (!ui.isOnboarding) {
                    loadingFirst.stop();

                    containerSecond.close();
                    containerThird.close();

                    if (ui.profiles.count === 0) {
                        containerMain.setSource("qrc:/components/ProfileAdd.qml", { state: "visible", noProfile: true })
                    } else {
                        containerMain.setSource("qrc:/components/ProfileSwitch.qml", { state: "visible", noProfile: true })
                    }
                }
            }
        }

        Connections {
            target: EntityController
            ignoreUnknownSignals: true

            // an activity turned on without being started here, e.g. through the API: optionally
            // bring its screen up, replacing whatever is currently open
            function onActivityStartedExternally(entityId) {
                if (!Config.openActivityOnApiStart || ui.isOnboarding) {
                    return;
                }

                const entityObj = EntityController.get(entityId);

                if (!entityObj) {
                    return;
                }

                console.debug("Opening activity started via the API:", entityId);
                openActivity(entityObj);
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
        VirtualKeyboardSettings.locale = Qt.binding(function() { return Config.language })

        if (ui.isOnboarding) {
            loadingFirst.stop();
            containerMain.source = "qrc:/OnboardingContainer.qml";
        }
    }
}
