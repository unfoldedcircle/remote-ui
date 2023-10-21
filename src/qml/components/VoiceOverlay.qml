// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 VOICE OVERLAY COMPONENT

 This is a placeholder element until the real functionality is implemented.
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Voice 1.0
import Entity.Controller 1.0
import Haptic 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as Entities

Popup {
    id: voice
    width: parent.width; height: parent.height
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    onOpened: {
        voice.listening = true;
        Voice.startListening();
        buttonNavigation.takeControl();
    }

    onClosed: {
        showExamples();
        circleContainer.reset();
        entityList.model = [];
        voice.multiple = false;
        buttonNavigation.releaseControl();
    }

    signal done

    property bool listening: false
    property bool multiple: false
    property string command
    property var param

    function showExamples() {
        exampleTexts.opacity = 1;
        //: Waiting for audio/voice input
        titleText.text = qsTr("Listening ...");
    }

    function hideExamples() {
        exampleTexts.opacity = 0;
    }

    function showError(message) {
        titleText.text = message;
        circleContainer.failure();
    }

    function executeCommand(entity) {
        if (voice.command === "turnOff") {
            titleText.text = "Turn off";
            entity.turnOff();
        } else if (voice.command === "turnOn") {
            titleText.text = "Turn on";
            entity.turnOn();
        } else if (voice.command === "setBrightness") {
            titleText.text = qsTr("Set brightness %1%").arg(voice.param);
            entity.setBrightness(voice.param/100*255);
        }

        circleContainer.succeeded();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "VOICE": {
                "released": function() {
                    if (voice.listening) {
                        voice.listening = false;
                        Voice.stopListening();
                        circleContainer.start();
                    }
                }
            },
            "BACK": {
                "released": function() {
                    voice.close();
                }
            },
            "HOME": {
                "released": function() {
                    voice.close();
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    Connections {
        target: Voice
        enabled: voice.opened
        ignoreUnknownSignals: true

        function onTranscriptionUpdated(text) {
            if (voice.listening)  {
                hideExamples();
                titleText.text = text;
            }
        }

        function onCommandExecuted(command, entity, param) {
            voice.command = command;
            voice.param = param;

            if (command === "turnOff") {
                titleText.text = "Turn off";
            } else if (command === "turnOn") {
                titleText.text = "Turn on";
            } else if (command === "setBrightness") {
                titleText.text = qsTr("Set brightness %1%").arg(param);
            }

            let objList = EntityController.getEntitiesByName(entity);

            entityList.model = objList;

            if (objList.length === 0) {
                showError(qsTr("Entity was not recognised"));

            } else if (objList.length === 1) {
                voice.listening = false;
                Voice.stopListening();
                circleContainer.start();
                voice.executeCommand(objList[0]);
            } else {
                voice.multiple = true;
                titleText.text = qsTr("Found %1 similar entities. Please select one to use").arg(objList.length);
                voice.listening = false;
                Voice.stopListening();
                circleContainer.start();
            }
        }

        function onError(message) {
            showError(message);
        }
    }

    background: Item {}

    Timer {
        id: hideTimer
        running: false
        interval: 500
        repeat: false

        onTriggered: {
            voice.close();
        }
    }

    Item {
        id: gradient
        width: parent.width; height: parent.height/2
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, parent.height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: colors.black }
            }
        }
    }

    Rectangle {
        width: parent.width; height: parent.height/2
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        color: colors.black
    }

    Item {
        id: eqAnimation
        width: 120; height: 108
        anchors { horizontalCenter: parent.horizontalCenter; bottom: entityList.top; bottomMargin: 120 }
        opacity: voice.listening ? 1 : 0

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
        }

        property int randomHeight1
        property int randomHeight2
        property int randomHeight3

        Timer {
            id: eqAnimationTimer
            running: eqAnimation.opacity == 1
            repeat: true
            interval: 200

            onTriggered: {
                leftBar.height = Math.floor(Math.random() * 108);
                mediumBar.height = Math.floor(Math.random() * 108);
                rightBar.height = Math.floor(Math.random() * 108);
            }
        }

        Rectangle {
            id: leftBar
            width: 14
            height: 108
            radius: 7
            color: colors.dark
            anchors { left: parent.left; verticalCenter: parent.verticalCenter }

            Behavior on height {
                PropertyAnimation { easing.type: Easing.InOutSine; duration: 200; }
            }

            Rectangle {
                width: 14; height: 14
                radius: 7
                color: colors.offwhite
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; }
            }

            Rectangle {
                width: 14; height: 14
                radius: 7
                color: colors.offwhite
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; }
            }
        }

        Rectangle {
            id: mediumBar
            width: 14
            height: 108
            radius: 7
            color: colors.medium
            anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }

            Behavior on height {
                PropertyAnimation { easing.type: Easing.InOutSine; duration: 200; }
            }

            Rectangle {
                width: 14; height: 14
                radius: 7
                color: colors.offwhite
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; }
            }

            Rectangle {
                width: 14; height: 14
                radius: 7
                color: colors.offwhite
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; }
            }
        }

        Rectangle {
            id: rightBar
            width: 14
            height: 108
            radius: 7
            color: colors.medium
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }

            Behavior on height {
                PropertyAnimation { easing.type: Easing.InOutSine; duration: 200; }
            }

            Rectangle {
                width: 14; height: 14
                radius: 7
                color: colors.offwhite
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; }
            }

            Rectangle {
                width: 14; height: 14
                radius: 7
                color: colors.offwhite
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; }
            }
        }
    }


    Item {
        id: circleContainer
        width: 160; height: 160
        anchors { horizontalCenter: parent.horizontalCenter; bottom: entityList.top; bottomMargin: 120 }
        opacity: 0
        transformOrigin: Item.Center

        property bool success: true

        function start() {
            circleContainer.opacity = 1;
            rotatingAnimation.start();
        }

        function succeeded() {
            circleContainer.success = true;
            rotatingAnimation.stop();
        }

        function failure() {
            circleContainer.success = false;
            rotatingAnimation.stop();
        }

        function reset() {
            circleContainer.opacity = 0;

            small.width = 0;
            large.height = 0;

            xone.width = 0;
            xtwo.height = 0;

            fillCircle1.width = 20;
            fillCircle1.y = 0;
            fillCircle1.opacity = 1;
            fillCircle1.color = colors.offwhite;

            fillCircle2.y = circleContainer.height-fillCircle2.height;
            fillCircle2.opacity = 1;
            fillCircle2.color = colors.offwhite;
        }

        Rectangle {
            id: fillCircle1
            width: 20; height: width
            radius: width/2
            color: colors.offwhite
            y: 0
            z: 2
            anchors.horizontalCenter: parent.horizontalCenter
            transformOrigin: Item.Center

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

        Rectangle {
            id: fillCircle2
            width: 20; height: width
            radius: width/2
            color: colors.offwhite
            y: circleContainer.height-height
            anchors.horizontalCenter: parent.horizontalCenter
            transformOrigin: Item.Center
        }

        SequentialAnimation {
            id: rotatingAnimation
            running: false
            loops: Animation.Infinite
            alwaysRunToEnd: true

            onFinished: {
                if (circleContainer.success) {
                    successAnimaton.start();
                } else {
                    failAnimation.start();
                }
            }

            NumberAnimation { target: circleContainer; properties: "rotation"; to: 0; duration: 1; }
            NumberAnimation { target: circleContainer; properties: "rotation"; to: 360; easing.type: Easing.OutSine; duration: 800; }
            PauseAnimation { duration: 200 }
        }

        SequentialAnimation {
            id: successAnimaton
            running: false
            alwaysRunToEnd: true

            onFinished: {
                hideTimer.start();
            }

            ParallelAnimation {
                NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 300; }
                NumberAnimation { target: fillCircle2; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 300; }
                SequentialAnimation {
                    PauseAnimation { duration: 200 }
                    ParallelAnimation {
                        NumberAnimation { target: fillCircle1; properties: "width"; to: 120; easing.type: Easing.OutExpo; duration: 300; }
                        NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2-60; easing.type: Easing.OutExpo; duration: 300; }
                    }
                    PropertyAnimation { target: small; properties: "width"; to: 35; easing.type: Easing.OutExpo; duration: 100 }
                    PropertyAnimation { target: large; properties: "height"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
                }
            }
        }

        SequentialAnimation {
            id: failAnimation
            running: false
            alwaysRunToEnd: true

            onFinished: {
                hideTimer.start();
            }

            ParallelAnimation {
                NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 300; }
                NumberAnimation { target: fillCircle2; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 300; }
                SequentialAnimation {
                    PauseAnimation { duration: 200 }
                    ParallelAnimation {
                        NumberAnimation { target: fillCircle1; properties: "width"; to: 120; easing.type: Easing.OutExpo; duration: 300; }
                        PropertyAnimation { target: fillCircle1; property: "color"; to: colors.red; easing.type: Easing.OutExpo; duration: 300; }
                        PropertyAnimation { target: fillCircle2; property: "color"; to: colors.red; easing.type: Easing.OutExpo; duration: 300; }
                        NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2-60; easing.type: Easing.OutExpo; duration: 300; }
                    }
                    PropertyAnimation { target: xone; properties: "width"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
                    PropertyAnimation { target: xtwo; properties: "height"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
                }
            }
        }
    }

    Text {
        id: titleText
        color: colors.offwhite
        //: Waiting for audio/voice input
        text: qsTr("Listening ...")

        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        width: parent.width - 80
        anchors { horizontalCenter: parent.horizontalCenter; bottom: entityList.top; bottomMargin: 20 }
        font: fonts.primaryFont(30)
    }

    ListView {
        id: entityList
        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        width: parent.width - 40
        height: voice.multiple ? 310 : exampleTexts.height + 60
        clip: true
        spacing: 10

        Behavior on height {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
        }

        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 10 }

        delegate: Entities.Base {
            entityId: modelData.id
            isSelected: true
            width: entityList.width

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    voice.executeCommand(modelData);
                    voice.multiple = false;
                    entityList.model = modelData;
                }
            }
        }
    }

    Item {
        id: exampleTexts
        width: parent.width - 20
        height: exampleTitleText.implicitHeight + exampleVoiceCommandsText.implicitHeight
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 20 }

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
        }

        Text {
            id: exampleTitleText
            color: colors.offwhite
            opacity: 0.6
            text: qsTr("You can say commands like")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; bottom: exampleVoiceCommandsText.top; bottomMargin: 20 }
            font: fonts.secondaryFont(22)
        }

        Text {
            id: exampleVoiceCommandsText
            color: colors.offwhite
            opacity: 0.8
            text: qsTr("“Turn on the Living room lights”\n“Start activity Watch TV”\n“Set Kitchen radiator temperature to 24º”")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 20 }
            font: fonts.secondaryFont(22, "Italic")
        }
    }
}
