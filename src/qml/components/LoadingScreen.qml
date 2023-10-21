// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import SoundEffects 1.0

Popup {
    id: loadingScreenBase
    width: parent.width; height: parent.height
    anchors.centerIn: parent
    modal: false
    padding: 0

    property bool _success: false
    property bool _failure: false
    property bool closeOnFinished: true
    property var stopCallback
    property var successCallback
    property var failureCallback
    property int yOffset

    function start(showGradient = true, yOffSet = 0) {
        ui.inputController.blockInput(true);
        reset();
        yOffset = yOffSet;
        gradient.visible = showGradient;
        blockingMouseArea.enabled = showGradient;
        //        visible = true;
        loadingScreenBase.open();
        rotatingAnimation.start();
        openAnimation.start();
        timeOutTimer.start();
    }

    function stop(callBack) {
        ui.inputController.blockInput(false);
        stopCallback = callBack;
        rotatingAnimation.stop();
        closeAnimation.start();
    }

    function success(close = true, callBack) {
        ui.inputController.blockInput(false);
        successCallback = callBack;
        closeOnFinished = close;
        _success = true;
        _failure = false;
        rotatingAnimation.alwaysRunToEnd = true;
        rotatingAnimation.stop();
    }

    function failure(close = true, callBack) {
        ui.inputController.blockInput(false);
        failureCallback = callBack;
        closeOnFinished = close;
        _success = false;
        _failure = true;
        rotatingAnimation.alwaysRunToEnd = true;
        rotatingAnimation.stop();
    }

    function reset() {
        rotatingAnimation.alwaysRunToEnd = false;

        _success = false;
        _failure = false;

        yOffset = 0;

        small.width = 0;
        large.height = 0;

        xone.width = 0;
        xtwo.height = 0;

        fillCircle1.width = 20;
        fillCircle1.y = 0;
        fillCircle1.opacity = 1;

        fillCircle2.y = circleContainer.height-fillCircle2.height;
        fillCircle2.opacity = 1;
    }

    background: Item {}

    Item {
        id: gradient
        width: parent.width; height: parent.height/2
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        opacity: 0

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
        opacity: gradient.opacity
        color: colors.black
    }

    Item {
        id: circleContainer
        width: 200; height: 200
        anchors { verticalCenter: parent.verticalCenter; verticalCenterOffset: loadingScreenBase.yOffset; horizontalCenter: parent.horizontalCenter }
        transformOrigin: Item.Center

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
            alwaysRunToEnd: false

            onFinished: {
                if (loadingScreenBase._success) {
                    successAnimaton.start();
                } else if (loadingScreenBase._failure) {
                    failAnimation.start();
                }
            }

            NumberAnimation { target: circleContainer; properties: "rotation"; to: 0; duration: 1;  }
            NumberAnimation { target: circleContainer; properties: "rotation"; to: 360; easing.type: Easing.OutSine; duration: 800;  }
            PauseAnimation { duration: 200 }
        }

        SequentialAnimation {
            id: successAnimaton
            running: false
            alwaysRunToEnd: true

            onFinished: {          
                if (closeOnFinished) {
                    closeAnimation.start();
                }
                if (successCallback) {
                    successCallback();
                    successCallback = null;
                }
            }

            ParallelAnimation {
                NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 500;  }
                NumberAnimation { target: fillCircle2; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 500;  }
                SequentialAnimation {
                    PauseAnimation { duration: 200 }
                    ParallelAnimation {
                        ScriptAction { script: SoundEffects.play(SoundEffects.Confirm) }
                        NumberAnimation { target: fillCircle1; properties: "width"; to: 120; easing.type: Easing.OutExpo; duration: 500;  }
                        NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2-60; easing.type: Easing.OutExpo; duration: 500;  }
                    }
                    PropertyAnimation { target: small; properties: "width"; to: 35; easing.type: Easing.OutExpo; duration: 100 }
                    PropertyAnimation { target: large; properties: "height"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
                }
            }
            PauseAnimation { duration: closeOnFinished ? 1000 : 0 }
            ParallelAnimation {
                NumberAnimation { target: fillCircle1; properties: "opacity"; to: closeOnFinished ? 0 : 1; easing.type: Easing.InExpo; duration: closeOnFinished ? 300 : 0;  }
                NumberAnimation { target: fillCircle2; properties: "opacity"; to: closeOnFinished ? 0 : 1; easing.type: Easing.InExpo; duration: closeOnFinished? 200 : 0;  }
            }
        }

        SequentialAnimation {
            id: failAnimation
            running: false
            alwaysRunToEnd: true

            onFinished: {
                if (closeOnFinished) {
                    closeAnimation.start();
                }
                if (failureCallback) {
                    failureCallback();
                    failureCallback = null;
                }
            }

            ParallelAnimation {
                NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 500;  }
                NumberAnimation { target: fillCircle2; properties: "y"; to: circleContainer.height/2; easing.type: Easing.OutExpo; duration: 500;  }
                SequentialAnimation {
                    PauseAnimation { duration: 200 }
                    ParallelAnimation {
                        NumberAnimation { target: fillCircle1; properties: "width"; to: 120; easing.type: Easing.OutExpo; duration: 500;  }
                        NumberAnimation { target: fillCircle1; properties: "y"; to: circleContainer.height/2-60; easing.type: Easing.OutExpo; duration: 500;  }
                    }
                    ParallelAnimation {
                        ScriptAction { script: SoundEffects.play(SoundEffects.Error) }
                        PropertyAnimation { target: xone; properties: "width"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
                        SequentialAnimation {
                            PauseAnimation { duration: 75 }
                            PropertyAnimation { target: xtwo; properties: "height"; to: 70; easing.type: Easing.OutExpo; duration: 150 }
                        }
                    }
                }
            }
            PauseAnimation { duration: closeOnFinished ? 1000 : 0 }
            ParallelAnimation {
                NumberAnimation { target: fillCircle1; properties: "opacity"; to: closeOnFinished ? 0 : 1; easing.type: Easing.InExpo; duration: closeOnFinished ? 300 : 0;  }
                NumberAnimation { target: fillCircle2; properties: "opacity"; to: closeOnFinished ? 0 : 1; easing.type: Easing.InExpo; duration: closeOnFinished? 200 : 0;  }
            }
        }
    }

    MouseArea {
        id: blockingMouseArea
        anchors.fill: parent
    }

    PropertyAnimation {
        id: openAnimation
        running: false
        target: gradient; properties: "opacity"; to: 1; easing.type: Easing.OutExpo; duration: 300
    }

    PropertyAnimation {
        id: closeAnimation
        running: false
        target: gradient; properties: "opacity"; to: 0; easing.type: Easing.OutExpo; duration: 300
        onFinished: {
            loadingScreenBase.close();
            if (stopCallback) {
                stopCallback();
                stopCallback = null;
            }
        }
    }

    Timer {
        id: timeOutTimer
        repeat: false
        running: false
        interval: 180000
        onTriggered: loadingScreenBase.stop()
    }
}
