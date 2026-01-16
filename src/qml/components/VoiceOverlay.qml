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
import Config 1.0
import Entity.Controller 1.0
import Haptic 1.0
import SoundEffects 1.0

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
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
        circleContainer.reset();
        titleText.text = qsTr("Listening ...");
        voice.wasError = false;
        voice.url = "";
        voice.mimeType = "";
        voice.finished = false;
    }

    signal done

    property QtObject voiceEntityObj
    property string profileId
    property bool listening: false
    property bool finished: false
    property int sessionId: 0
    property bool wasError: false
    property string url: ""
    property string mimeType: ""

    function start(entityId = "", profileId = "") {
        if (voice.opened) {
            return;
        }

        let voiceEntityId = entityId;

        if (voiceEntityId == "") {
            voiceEntityId = Config.voiceAssistantId;
        }

        if (voiceEntityId == "" || !Config.micEnabled) {
            return;
        }

        voice.open();

        voice.profileId = profileId;

        console.info("Starting voice assistant", voiceEntityId, voice.profileId);

        voice.listening = true;

        voice.voiceEntityObj = EntityController.get(voiceEntityId);

        if (!voice.voiceEntityObj) {
            EntityController.load(voiceEntityId);
            connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                if (success && entityId == voiceEntityId) {
                    voice.voiceEntityObj = EntityController.get(voiceEntityId);

                    if (voice.voiceEntityObj) {
                        voice.init();
                    } else {
                        circleContainer.start();
                        showError(qsTr("Voice Assistant is not available."));
                    }
                }
            });
        } else {
            voice.init();
        }
    }

    function stop() {
        if (!voice.opened) {
            return;
        }

        if (voice.finished) {
            return;
        }

        voice.listening = false;

        if (voice.wasError) {
            return;
        }

        circleContainer.start();

        titleText.text = qsTr("Processing ...");

        if (voice.voiceEntityObj) {
            voice.voiceEntityObj.voiceEnd();
        }

        timeoutTimer.restart();
    }

    function init() {
        voice.sessionId = Voice.getSessionId();
        voice.voiceEntityObj.voiceStart(voice.sessionId, Config.voiceAssistantSpeechResponse, voice.profileId);
    }

    function showError(message) {
        voice.listening = false;
        voice.wasError = true;
        titleText.text = message;
        if (!rotatingAnimation.running) {
            circleContainer.start();
        }
        circleContainer.failure();
    }

    function checkSession(sessionId) {
        return sessionId == voice.sessionId;
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    Connections {
        target: Voice
        ignoreUnknownSignals: true

        function onAssistantEventReady(entityId, sessionId) {
            console.debug("Assistant Ready", entityId, sessionId);

            if (!checkSession(sessionId)) {
                return;
            }
        }

        function onAssistantEventSttResponse(entityId, sessionId, text) {
            console.debug("Assistant STT Response", entityId, sessionId, text);

            if (!checkSession(sessionId)) {
                return;
            }

            timeoutTimer.stop();

            titleText.text = text;
        }

        function onAssistantEventTextResponse(entityId, sessionId, success, text) {
            console.debug("Assistant Text Response", entityId, sessionId, success, text);

            if (!checkSession(sessionId)) {
                return;
            }

            timeoutTimer.stop();

            if (success) {
                titleText.text = text;
            } else {
                voice.wasError = true;
                voice.showError(text);
            }
        }

        function onAssistantEventSpeechResponse(entityId, sessionId, url, mimeType) {
            console.debug("Assistant Speech Response", entityId, sessionId, url, mimeType);

            if (!checkSession(sessionId)) {
                return;
            }

            timeoutTimer.stop();

            if (Config.voiceAssistantSpeechResponse) {
                voice.url = url;
                voice.mimeType = mimeType;
            }
        }

        function onAssistantEventFinished(entityId, sessionId) {
            console.debug("Assistant Finihsed", entityId, sessionId);

            if (!checkSession(sessionId)) {
                return;
            }

            voice.finished = true;

            timeoutTimer.stop();

            if (voice.wasError) {
                circleContainer.failure();
            } else {
                circleContainer.succeeded();
            }
        }

        function onAssistantEventError(entityId, sessionId, message) {
            console.debug("Assistant Error", entityId, sessionId, message);

            if (!checkSession(sessionId)) {
                return;
            }

            timeoutTimer.stop();

            voice.showError(message);
        }

        function onAssistantAudioSpeechResponseEnd() {
            timeoutTimer.stop();
            voice.close();
        }
    }

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onVoiceAssistantCommandError(entityId, code) {
            if (voice.voiceEntityObj.id == entityId) {
                voice.wasError = true;
                circleContainer.start();
                voice.listening = false;
                timeoutTimer.stop();

                let errorMessage = qsTr("There was an error.");

                switch (code) {
                case 400:
                    errorMessage = qsTr("Request failed.");
                    break;
                case 401:
                    errorMessage = qsTr("Not authenticated.");
                    break;
                case 403:
                    errorMessage = qsTr("Missing rights to use voice assistant.");
                    break;
                case 404:
                    errorMessage = qsTr("Voice assistant not found. Please check configuration.");
                    break;
                case 429:
                    errorMessage = qsTr("There were too many requests. Please try again later.");
                    break;
                case 500:
                    errorMessage = qsTr("Internal server error.");
                    break;
                case 503:
                    errorMessage = qsTr("Voice assistant is unavailable.");
                    break;
                }

                ui.setTimeOut(500, () => {
                                  voice.showError(errorMessage);
                              });
            }
        }
    }

    background: Item {}

    Timer {
        id: hideTimer
        running: false
        interval: 2000
        repeat: false

        onTriggered: {
            voice.close();
        }
    }

    Timer {
        id: timeoutTimer
        running: false
        interval: 15000
        repeat: false

        onTriggered: {
            voice.showError(qsTr("It’s taking longer than expected. Please try your request again."));
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
        anchors { horizontalCenter: parent.horizontalCenter; bottom: titleText.top; bottomMargin: 120 }
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
        anchors { horizontalCenter: parent.horizontalCenter; bottom: titleText.top; bottomMargin: 120 }
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
                if (voice.url == "") {
                    hideTimer.start();
                }
            }

            ParallelAnimation {
                NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 300; }
                NumberAnimation { target: fillCircle2; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 300; }
                SequentialAnimation {
                    PauseAnimation { duration: 200 }
                    ParallelAnimation {
                        SequentialAnimation {
                            ScriptAction { script: SoundEffects.play(SoundEffects.Confirm) }
                            ScriptAction { script: {
                                    if (voice.url != "") {
                                        Voice.playSpeechResponse(voice.url, voice.mimeType);
                                    }
                                }
                            }
                        }
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
                    ScriptAction { script: SoundEffects.play(SoundEffects.Error) }
                    PropertyAnimation { target: xone; properties: "width"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
                    PropertyAnimation { target: xtwo; properties: "height"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
                    PauseAnimation { duration: 2000 }
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
        anchors { horizontalCenter: parent.horizontalCenter; bottom: assistantNameText.top; bottomMargin: 10 }
        font: fonts.primaryFont(30)
    }

    Text {
        id: assistantNameText
        text: voice.voiceEntityObj ? voice.voiceEntityObj.name : ""
        color: colors.light
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        width: parent.width - 80
        anchors { horizontalCenter: parent.horizontalCenter; bottom: assistantProfileNameText.top; bottomMargin: 0 }
        font: fonts.primaryFont(24)
    }

    Text {
        id: assistantProfileNameText
        text: {
            if (voice.voiceEntityObj) {
                const profileObj = voice.voiceEntityObj.getProfile(voice.profileId);

                if (profileObj) {
                    return profileObj.name;
                } else {
                    return "";
                }
            }
        }

        color: colors.light
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        width: parent.width - 80
        height: text == "" ? 0 : implicitHeight
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 60 }
        font: fonts.primaryFont(20)
    }


    MouseArea {
        anchors.fill: parent
        onClicked: {
            voice.close();
        }
    }

    Components.ButtonNavigation {
        overrideActive: voice.opened
        defaultConfig: {
            "HOME": {
                "pressed": function() {
                    voice.close();
                }
            },
            "BACK": {
                "pressed": function() {
                    voice.close();
                }
            }
        }
    }
}
