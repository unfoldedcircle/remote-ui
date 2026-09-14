// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Entity.Controller 1.0
import Entity.Activity 1.0
import Entity.Macro 1.0
import SequenceStep.Type 1.0

import "qrc:/components" as Components

Popup {
    id: activityLoading
    x: 0; y:0
    width: parent.width
    height: parent.height
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0

    property string prevController

    onOpened: {
        mouseArea.enabled = false;
        activityLoading.retrySelected = false;
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();

        if (!activityLoading.isMacro && entityObj.state === ActivityStates.On) {
            loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj });
        }

        activityLoading.entityId = "";
        activityLoading.isMacro = false;
        // without this the next run starts with the state the previous one ended in
        activityLoading.prevState = ActivityStates.Unknown;
        activityLoading.stepIcon = "";
        activityLoading.stepName = "";
        activityLoading.cmdId = "";
        activityLoading.failed = false;
        activityLoading.retrySelected = false;
        activityLoading.stepFailed = false;
        activityLoading.failedSteps = [];

        dotOK.width = 0;
        dotOK.height = 0;
        dotOK.radius = 7;
        dotOK.color = colors.offwhite;

        dot.opacity = 1;

        small.width = 0;
        large.height = 0;

        xone.width = 0;
        xtwo.height = 0;

        errorText.opacity = 0;

        activityLoading.entityObj.clearCurrentStep();

        // "Try again" parks the run here: the command is only sent once the screen is reset, so that the
        // startedRunning path finds the popup closed and reopens it with a fresh ring
        if (activityLoading.pendingRetry) {
            const retry = activityLoading.pendingRetry;
            activityLoading.pendingRetry = null;

            // the activity may have been deleted while the error screen was up
            const target = EntityController.get(retry.entityId);

            if (!target) {
                return;
            }

            if (retry.isMacro) {
                activityLoading.start(retry.entityId, EntityTypes.Macro, "macro.run");
                target.run();
            } else if (retry.cmdId === "activity.off") {
                target.turnOff();
            } else {
                target.turnOn();
            }
        }
    }

    Connections {
        id: entityConnection
        target: activityLoading.entityObj
        ignoreUnknownSignals: true

        function onStateChanged(entityId, newState) {
            if (entityId !== activityLoading.entityId) {
                return;
            }

            console.info((activityLoading.isMacro ? "Macro" : "Activity") + " state changed to: " + entityObj.stateAsString);

            if (entityObj.state === ActivityStates.Timeout) {
                activityLoading.end(true);
                errorText.text = qsTr("Sequence didn't finish within %1 seconds. Check configuration.").arg(entityObj.timeout / 1000);
            }

            if (activityLoading.prevState !== (activityLoading.isMacro ? MacroStates.Running : ActivityStates.Running) && entityObj.state === ActivityStates.Off) {
                activityLoading.end(false);
                return;
            }

            if ((entityObj.state === (activityLoading.isMacro ? MacroStates.Completed : ActivityStates.Completed)) || (!activityLoading.isMacro && (entityObj.state === ActivityStates.On || entityObj.state === ActivityStates.Off)) ) {
                activityLoading.end(false);
            } else if (entityObj.state === (activityLoading.isMacro ? MacroStates.Error : ActivityStates.Error)
                       || entityObj.state === ActivityStates.Unavailable) {
                // Unavailable ends the run as well: the entity dropped out from under the sequence, for
                // example because the connection to the core was lost, and no further state is coming
                activityLoading.end(true);
                errorText.text = activityLoading.sequenceErrorText();
            }

            activityLoading.prevState = entityObj.state;
        }

        function onCurrentStepChanged() {
            console.info("Current step changed: " + entityObj.currentStep.commandId);

            const step = entityObj.currentStep;
            const stepEntityObj = EntityController.get(step.entityId);
            activityLoading.stepIcon = stepEntityObj ? stepEntityObj.icon : "uc:triangle-exclamation";
            //: Shown for a step whose device cannot be resolved, e.g. because it was deleted.
            activityLoading.stepName = stepEntityObj ? stepEntityObj.name : qsTr("Unknown device");

            // A step that fails while the run carries on (error policy "continue") would otherwise leave no
            // trace: the screen only reacts to the state turning Error. Mark the step as it happens and keep
            // a notch on the ring for the rest of the run.
            const running = entityObj.state === (activityLoading.isMacro ? MacroStates.Running : ActivityStates.Running);
            const hasError = step.errorCode > 0 || step.errorMessage !== "" || step.error !== "";
            activityLoading.stepFailed = running && hasError;

            if (activityLoading.stepFailed && activityLoading.failedSteps.indexOf(step.index) < 0) {
                activityLoading.failedSteps = activityLoading.failedSteps.concat([step.index]);
                canvas.requestPaint();
            }
        }
    }

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onActivityStartedRunning(entityId, cmdId) {
            if (activityLoading.closed) {
                activityLoading.start(entityId, EntityTypes.Activity, cmdId);
            }
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    activityLoading.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    activityLoading.close();
                }
            }
        }

        // nothing reacts to the d-pad while the run is still going: the buttons only exist once it failed
        Component.onCompleted: {
            buttonNavigation.extendDefaultConfig({
                "DPAD_LEFT": {
                    "pressed": function() {
                        if (activityLoading.failed) {
                            activityLoading.retrySelected = false;
                        }
                    }
                },
                "DPAD_RIGHT": {
                    "pressed": function() {
                        if (activityLoading.failed) {
                            activityLoading.retrySelected = true;
                        }
                    }
                },
                "DPAD_MIDDLE": {
                    "pressed": function() {
                        if (!activityLoading.failed) {
                            return;
                        }

                        if (activityLoading.retrySelected) {
                            activityLoading.retry();
                        } else {
                            activityLoading.close();
                        }
                    }
                }
            });
        }
    }

    // cmdId is the command the run was started with: "activity.on", "activity.off" or "macro.run"
    function start(entityId, type, cmdId = "macro.run") {
        if (type !== EntityTypes.Activity) {
            isMacro = true;
            console.debug("Entity type is macro");
        }

        activityLoading.entityId = entityId;
        activityLoading.cmdId = cmdId;
        activityLoading.entityObj = EntityController.get(entityId);
        entityConnection.enabled = true;

        if (!activityLoading.entityObj) {
            entityConnection.enabled = false;
            return;
        }

        console.debug("Starting activity loader for: " + entityId);
        activityLoading.open();
    }

    // The core reports why a command step failed, for example "Connection to dock not established". The
    // generic text is only used when it does not, otherwise the screen would tell the user nothing about
    // what went wrong.
    function sequenceErrorText() {
        const step = activityLoading.entityObj ? activityLoading.entityObj.currentStep : null;

        if (!step) {
            return qsTr("There was an error during the sequence.");
        }

        if (step.errorMessage !== "") {
            if (step.errorCode > 0) {
                //: %1 is an error message reported by the device, %2 the error code
                return qsTr("%1 (error %2)").arg(step.errorMessage).arg(step.errorCode);
            }

            return step.errorMessage;
        }

        if (step.errorCode > 0) {
            //: %1 is an error code reported by the device
            return qsTr("There was an error during the sequence. Error code: %1").arg(step.errorCode);
        }

        return qsTr("There was an error during the sequence.");
    }

    // Repeats the run that just failed. The user has already been through the readiness decision, so the
    // command goes out directly - no check in between. The popup's exit transition still runs and onClosed
    // resets every property afterwards: sending the command now would have onClosed wipe the new run, and
    // the screen would not reopen either as the popup is not closed yet. The run is parked for onClosed.
    function retry() {
        activityLoading.pendingRetry = {
            entityId: activityLoading.entityId,
            cmdId: activityLoading.cmdId,
            isMacro: activityLoading.isMacro
        };
        activityLoading.close();
    }

    function end(error) {
        console.debug("Activity loading end");
        if (error) {
            activityLoading.failed = true;
            errorAnimation.start();
        } else {
            successAnimation.start();
        }

        entityConnection.enabled = false;
    }

    property bool isMacro: false
    property string entityId
    property string cmdId: ""
    property int prevState: ActivityStates.Unknown
    property QtObject entityObj
    property string stepIcon: ""
    property string stepName: ""
    // the run ended in an error: the screen shows what failed and offers Close and Try again
    property bool failed: false
    // keypad selection of the two buttons; Close is preselected
    property bool retrySelected: false
    property var pendingRetry: null
    // the current step reported an error but the run carries on
    property bool stepFailed: false
    // indices of the steps that failed while the run carried on, for the notches on the ring
    property var failedSteps: []

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.InExpo; duration: 200 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 200 }
    }

    background: Item {
        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, parent.height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: colors.transparent }
                GradientStop { position: 0.6; color: colors.black }
                GradientStop { position: 1.0; color: colors.black }
            }
        }
    }

    SequentialAnimation {
        id: successAnimation
        running: false
        alwaysRunToEnd: true

        onFinished: activityLoading.close()

        ParallelAnimation {
            PropertyAnimation { target: dot; properties: "opacity"; to: 0; easing.type: Easing.OutExpo; duration: 300 }
            PropertyAnimation { target: dotOK; properties: "width, height"; to: 108; easing.type: Easing.OutExpo; duration: 600; }
            PropertyAnimation { target: dotOK; properties: "radius"; to: 54; easing.type: Easing.OutExpo; duration: 600; }
        }
        PropertyAnimation { target: small; properties: "width"; to: 35; easing.type: Easing.OutExpo; duration: 100 }
        PropertyAnimation { target: large; properties: "height"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
        PauseAnimation { duration: 400 }
    }

    SequentialAnimation {
        id: errorAnimation
        running: false
        alwaysRunToEnd: true

        onFinished: mouseArea.enabled = true

        ParallelAnimation {
            ParallelAnimation {
                PropertyAnimation { target: errorText; properties: "opacity"; to: 1; easing.type: Easing.OutExpo; duration: 300 }
            }
            PropertyAnimation { target: dot; properties: "opacity"; to: 0; easing.type: Easing.OutExpo; duration: 300 }
            PropertyAnimation { target: dotOK; properties: "width, height"; to: 108; easing.type: Easing.OutExpo; duration: 600; }
            PropertyAnimation { target: dotOK; properties: "radius"; to: 54; easing.type: Easing.OutExpo; duration: 600; }
            ColorAnimation { target: dotOK; properties: "color"; to: colors.red; duration: 600; }
        }
        ParallelAnimation {
            PropertyAnimation { target: xone; properties: "width"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
            SequentialAnimation {
                PauseAnimation { duration: 75 }
                PropertyAnimation { target: xtwo; properties: "height"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
            }
        }
        PauseAnimation { duration: 500 }
    }

    ColumnLayout {
        width: parent.width
        height: ui.height
        anchors.bottom: parent.bottom
        spacing: 10
        // above the tap-anywhere area below, so the buttons get their taps and everything else falls through
        z: 1

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Behavior on height {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 500 }
            }
        }

        Rectangle {
            id: backgroundCircle
            width: 108
            height: 108
            radius: width / 2
            color: colors.transparent
            border { width: 14; color: colors.dark }

            Layout.alignment: Qt.AlignHCenter

            Canvas {
                id: canvas

                property real angle: entityObj ? 360 / entityObj.totalSteps * entityObj.currentStep.index : 0

                width: parent.width * ui.ratio
                height: parent.height * ui.ratio
                scale: 1 / ui.ratio
                anchors.centerIn: parent
                antialiasing: true

                onAngleChanged: {
                    requestPaint();
                }

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.save();
                    ctx.scale(ui.ratio, ui.ratio);

                    var x = backgroundCircle.width / 2;
                    var y = backgroundCircle.height / 2;

                    var radius = backgroundCircle.width / 2 - 7
                    var startAngle = (Math.PI / 180) * 270;
                    var progressAngle = (Math.PI / 180) * (270 + angle);

                    ctx.reset();

                    ctx.lineCap = 'round';
                    ctx.lineWidth = 14;

                    ctx.beginPath();
                    ctx.arc(x, y, radius, startAngle, progressAngle);
                    ctx.strokeStyle = colors.primaryButton;
                    ctx.stroke();

                    // the slices of the steps that stayed silent, so the ring keeps the record of the run
                    if (entityObj && entityObj.totalSteps > 0) {
                        const slice = 2 * Math.PI / entityObj.totalSteps;

                        ctx.lineCap = 'butt';
                        ctx.strokeStyle = colors.orange;

                        for (let i = 0; i < activityLoading.failedSteps.length; i++) {
                            const index = activityLoading.failedSteps[i];

                            ctx.beginPath();
                            ctx.arc(x, y, radius, startAngle + slice * (index - 1), startAngle + slice * index);
                            ctx.stroke();
                        }
                    }
                }

                Behavior on angle {
                    NumberAnimation { easing.type: Easing.InOutSine; duration: 200 }
                }
            }

            Item {
                id: loadingCircle
                width: parent.width
                height: parent.height
                anchors.centerIn: parent
                transformOrigin: Item.Center
                rotation: entityObj ? 360 / entityObj.totalSteps * entityObj.currentStep.index : 0

                Behavior on rotation {
                    NumberAnimation { easing.type: Easing.InOutSine; duration: 200 }
                }

                Rectangle {
                    id: dot
                    width: 14
                    height: 14
                    radius: 7
                    color: colors.offwhite
                    anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
                    opacity: entityObj ? (entityObj.totalSteps === entityObj.currentStep.index ? 0 : 1) : 0

                    Behavior on opacity {
                        NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                    }
                }
            }

            Rectangle {
                id: dotOK
                width: 0
                height: 0
                radius: 7
                color: colors.offwhite
                anchors.centerIn: parent

                // checkmark animation
                Item {
                    anchors { horizontalCenter: parent.horizontalCenter; horizontalCenterOffset: -35; verticalCenter: parent.verticalCenter }
                    rotation: 45
                    transformOrigin: Item.Center

                    Rectangle {
                        id: small
                        width: 0
                        height: 4
                        color: colors.black
                    }

                    Rectangle {
                        id: large
                        width: 4
                        height: 0
                        color: colors.black
                        anchors { bottom: small.bottom; right: small.right }
                    }
                } // checkmark end

                // x animation
                Item {
                    anchors {
                        horizontalCenter: parent.horizontalCenter;
                        verticalCenter: parent.verticalCenter
                    }
                    rotation: 45
                    transformOrigin: Item.Center

                    Rectangle {
                        id: xone
                        width: 0
                        height: 4
                        color: colors.black
                        anchors.centerIn: parent
                    }

                    Rectangle {
                        id: xtwo
                        width: 4
                        height: 0
                        color: colors.black
                        anchors.centerIn: parent
                    }
                } // x end
            }
        }

        Text {
            id: title
            text: activityLoading.failed
                  //: %1 is the name of the activity or macro whose run failed.
                  ? qsTr("%1 stopped").arg(entityObj ? entityObj.name : "")
                  : (entityObj ? entityObj.name : "")
            width: parent.width - 40
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            color: colors.offwhite
            font: fonts.primaryFont(30)
            Layout.topMargin: activityLoading.failed ? 34 : 40
            Layout.alignment: Qt.AlignHCenter
        }

        // ----- while the run is going: the current step -----

        Text {
            id: smallTitleText
            maximumLineCount: 1
            elide: Text.ElideRight
            color: colors.offwhite
            //: Indicating the activity steps
            text: qsTr("Step %1/%2").arg(entityObj ? entityObj.currentStep.index : 0).arg(entityObj ? entityObj.totalSteps : 0)
            font: fonts.secondaryFont(26)
            visible: !activityLoading.failed && (entityObj ? entityObj.totalSteps !== 0 : false)

            Layout.topMargin: 10
            Layout.alignment: Qt.AlignHCenter
        }

        Item {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            visible: !activityLoading.failed && (entityObj ? entityObj.totalSteps !== 0 : false)

            Layout.preferredHeight: centeredRow.implicitHeight

            RowLayout {
                id: centeredRow
                spacing: 4

                anchors.horizontalCenter: parent.horizontalCenter

                property int maxTextWidth: parent.width - entityInfoIcon.width - spacing

                Components.Icon {
                    id: entityInfoIcon
                    color: activityLoading.stepFailed ? colors.orange : colors.offwhite
                    icon: {
                        if (!entityObj) {
                            return "";
                        }
                        if (activityLoading.stepFailed) {
                            return "uc:link-slash";
                        }
                        return entityObj.currentStep.type === SequenceStep.Delay ? "uc:clock" : activityLoading.stepIcon;
                    }
                    size: 40
                }

                Text {
                    id: entityInfo

                    Layout.preferredWidth: Math.min(implicitWidth, centeredRow.maxTextWidth)

                    text: {
                        if (!entityObj) {
                            return "";
                        }

                        if (entityObj.currentStep.type === SequenceStep.Delay) {
                            //: Current activity step is a delay of %1 miliseconds
                            return qsTr("Delay %1 ms").arg(entityObj.currentStep.delay);
                        } else {
                            let cmdId = entityObj.currentStep.commandId;
                            let splitCmdId = cmdId.split(".");

                            return activityLoading.stepName + " → " + (splitCmdId.length > 1 ? splitCmdId[1].toUpperCase() : cmdId.toUpperCase())
                        }
                    }
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    color: activityLoading.stepFailed ? colors.orange : colors.offwhite
                    font: fonts.secondaryFont(26)
                }
            }
        }

        Text {
            visible: !activityLoading.failed && activityLoading.stepFailed
            //: Under the name of a device that did not react during an activity whose run continues regardless.
            text: qsTr("No response · carrying on")
            color: colors.orange
            opacity: 0.75
            font: fonts.secondaryFont(24)

            Layout.alignment: Qt.AlignHCenter
        }

        // ----- after a failed run: where it stopped, the device, the reason, and a way out -----

        Text {
            visible: activityLoading.failed && (entityObj ? entityObj.totalSteps !== 0 : false)
            //: Position of the step an activity stopped at. %1 is the step, %2 the number of steps.
            text: qsTr("at step %1 of %2").arg(entityObj ? entityObj.currentStep.index : 0).arg(entityObj ? entityObj.totalSteps : 0)
            color: colors.light
            font: fonts.secondaryFont(24)

            Layout.topMargin: 8
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            visible: activityLoading.failed && activityLoading.stepName !== ""
            text: activityLoading.stepName
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            color: colors.offwhite
            font: fonts.primaryFont(28)

            Layout.preferredWidth: 440
            Layout.topMargin: 26
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            id: errorText
            visible: activityLoading.failed
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            horizontalAlignment: Text.AlignHCenter
            color: colors.red
            font: fonts.secondaryFont(26)
            lineHeight: 1.3

            Layout.topMargin: 8
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
        }

        Item {
            visible: activityLoading.failed
            // fades in with the reason
            opacity: errorText.opacity

            Layout.preferredWidth: 440
            Layout.preferredHeight: retryButton.height
            Layout.topMargin: 40
            Layout.bottomMargin: 30
            Layout.alignment: Qt.AlignHCenter

            Text {
                id: closeButton
                //: Button on the failed activity screen: dismiss it.
                text: qsTr("Close")
                width: parent.width / 2 - 10
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                color: colors.offwhite
                font: fonts.secondaryFont(26, "Bold")
                anchors { left: parent.left; verticalCenter: parent.verticalCenter }

                Rectangle {
                    anchors { fill: parent; margins: -8 }
                    radius: ui.cornerRadiusSmall
                    color: colors.transparent
                    border { width: 2; color: !activityLoading.retrySelected && ui.keyNavigationActive
                                              ? colors.highlight : colors.transparent }
                }

                Components.HapticMouseArea {
                    width: parent.width + 40
                    height: parent.height + 40
                    anchors.centerIn: parent
                    onClicked: activityLoading.close()
                }
            }

            Text {
                id: retryButton
                //: Button on the failed activity screen: run the activity again.
                text: qsTr("Try again")
                width: parent.width / 2 - 10
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignRight
                color: colors.offwhite
                font: fonts.secondaryFont(26, "Bold")
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }

                Rectangle {
                    anchors { fill: parent; margins: -8 }
                    radius: ui.cornerRadiusSmall
                    color: colors.transparent
                    border { width: 2; color: activityLoading.retrySelected && ui.keyNavigationActive
                                              ? colors.highlight : colors.transparent }
                }

                Components.HapticMouseArea {
                    width: parent.width + 40
                    height: parent.height + 40
                    anchors.centerIn: parent
                    onClicked: activityLoading.retry()
                }
            }
        }
    }

    MouseArea {
        id: mouseArea
        enabled: false
        anchors.fill: parent
        onClicked: activityLoading.close()
    }
}
