// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Entity.Controller 1.0
import Entity.Activity 1.0
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
        activityLoading.prevController = ui.inputController.activeObject
        mouseArea.enabled = false;
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl(activityLoading.prevController);

        if (!activityLoading.isMacro && entityObj.state === ActivityStates.On) {
            loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj });
        }

        activityLoading.entityId = "";
        activityLoading.isMacro = false;
        activityLoading.stepIcon = "";
        activityLoading.stepName = "";

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
    }

    Connections {
        id: entityConnection
        target: activityLoading.entityObj
        ignoreUnknownSignals: true

        function onStateChanged(entityId, newState) {
            if (entityId !== activityLoading.entityId) {
                return;
            }

            let stateString = entityObj.stateAsString

            if (activityLoading.prevStateString != "Running" && stateString === "Off") {
                activityLoading.end(false);
                return;
            }

            if ((stateString === "Completed") || stateString === "On" || stateString === "Off" ) {
                activityLoading.end(false);
            } else if (stateString === "Error") {
                activityLoading.end(true);
                errorText.text = entityObj.currentStep.error;
            }

            activityLoading.prevStateString = stateString;
        }

        function onCurrentStepChanged() {
            let stepEntityObj = EntityController.get(entityObj.currentStep.entityId);
            activityLoading.stepIcon = stepEntityObj ? stepEntityObj.icon : "";
            activityLoading.stepName = stepEntityObj ? stepEntityObj.name : "";
        }
    }

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onActivityStartedRunning(entityId) {
            if (activityLoading.closed) {
                activityLoading.start(entityId, EntityTypes.Activity);
            }
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    activityLoading.close();
                }
            },
            "HOME": {
                "released": function() {
                    activityLoading.close();
                }
            }
        }
    }

    function start(entityId, type) {
        if (type !== EntityTypes.Activity) {
            isMacro = true;
            console.debug("Entity type is macro");
        }

        activityLoading.entityId = entityId;
        activityLoading.entityObj = EntityController.get(entityId);
        entityConnection.enabled = true;

        if (!activityLoading.entityObj) {
            entityConnection.enabled = false;
            return;
        }

        console.debug("Starting activity loader for: " + entityId);
        activityLoading.open();
    }

    function end(error) {
        console.debug("Activity loading end");
        if (error) {
            errorAnimation.start();
            ui.setTimeOut(1000, function () {
                errorText.text += "\n" + qsTr("Tap to close");
            });
        } else {
            successAnimation.start();
        }

        entityConnection.enabled = false;
    }

    property bool isMacro: false
    property string entityId
    property string prevStateString: "unknown"
    property QtObject entityObj
    property string stepIcon: ""
    property string stepName: ""

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

    Item {
        width: parent.width
        height: ui.height / 2
        anchors.bottom: parent.bottom

        Rectangle {
            id: backgroundCircle
            width: 108
            height: 108
            radius: width / 2
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -100
            color: colors.transparent
            border { width: 14; color: colors.dark }

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
            text: entityObj ? entityObj.name : ""
            width: parent.width - 40
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            horizontalAlignment: Text.AlignHCenter
            color: colors.offwhite
            anchors { horizontalCenter: parent.horizontalCenter; top: backgroundCircle.bottom; topMargin: 40 }
            font: fonts.primaryFont(30)
        }

        Text {
            id: smallTitleText
            maximumLineCount: 1
            elide: Text.ElideRight
            color: colors.offwhite
            opacity: 0.6
            //: Indicating the activity steps
            text: qsTr("Step %1/%2").arg(entityObj ? entityObj.currentStep.index : 0).arg(entityObj ? entityObj.totalSteps : 0)
            anchors { horizontalCenter: parent.horizontalCenter; top: title.bottom; topMargin: 10 }
            font: fonts.secondaryFont(24,  "Medium")
            visible: entityObj ? entityObj.totalSteps !== 0 : false
        }

        RowLayout {
            spacing: 10
            anchors { top: smallTitleText.bottom; topMargin: errorText.lineCount > 1 ? 10 : 30; horizontalCenter: parent.horizontalCenter }
            visible: entityObj ? entityObj.totalSteps !== 0 : false

            Components.Icon {
                id: entityInfoIcon
                color: colors.offwhite
                icon: entityObj ? entityObj.currentStep.type === SequenceStep.Delay ? "uc:clock" : activityLoading.stepIcon : ""
                size: 40
            }

            Text {
                Layout.fillWidth: true
                id: entityInfo
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

                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                maximumLineCount: 1
                color: colors.offwhite
                opacity: 0.6
                font: fonts.secondaryFont(24,  "Medium")
            }
        }

        Text {
            id: errorText
            width: parent.width - 40
            maximumLineCount: 2
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            color: colors.red
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 15 }
            font: fonts.secondaryFont(24,  "Medium")
//            lineHeight: 0.7
        }
    }

    MouseArea {
        id: mouseArea
        enabled: false
        anchors.fill: parent
        onClicked: activityLoading.close()
    }
}
