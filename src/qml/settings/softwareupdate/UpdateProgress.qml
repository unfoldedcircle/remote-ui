// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import SoftwareUpdate 1.0

import "qrc:/components" as Components

Popup {
    id: updateProgress
    width: parent.width; height: parent.height
    opacity: 0
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0

    property QtObject prevKeyController

    Connections {
        target: SoftwareUpdate
        ignoreUnknownSignals: true

        function onUpdateSucceeded() {
            console.debug("UpdateProgress: onUpdateSucceeded received, showing success screen");
            successScreen.opacity = 1;
        }

        function onUpdateFailed(error) {
            console.debug("UpdateProgress: onUpdateFailed received, error:", error);
            failMessage.text = error;
            failedScreen.opacity = 1;
        }
    }

    onOpened: {
        console.debug("UpdateProgress: popup opened, progress:", SoftwareUpdate.updateProgress,
                       "step:", SoftwareUpdate.currentStep, "/", SoftwareUpdate.totalSteps);
        ui.inputController.blockInput(true);
    }

    onClosed: {
        console.debug("UpdateProgress: popup closed, failedScreen.opacity:", failedScreen.opacity,
                       "successScreen.opacity:", successScreen.opacity);
        failedScreen.opacity = 0;
        ui.inputController.blockInput(false);
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    background: Rectangle { color: colors.black }

    Text {
        id: title
        width: parent.width - 40
        wrapMode: Text.WordWrap
        color: colors.offwhite
        text: qsTr("Update in progress")
        horizontalAlignment: Text.AlignHCenter
        font: fonts.primaryFont(30)
        anchors { top: parent.top; topMargin: 320; horizontalCenter: parent.horizontalCenter }
    }

    Rectangle {
        id: progress
        width: parent.width - 40
        height: 10
        color: colors.dark
        radius: 5
        anchors { top: title.bottom; topMargin: 30; horizontalCenter: parent.horizontalCenter }

        Rectangle {
            width: parent.width * SoftwareUpdate.updateProgress / 100
            height: parent.height
            color: colors.offwhite
            radius: parent.radius
            anchors { left: parent.left }
        }
    }

    Text {
        id: percentage
        width: parent.width - 40
        wrapMode: Text.WordWrap
        color: colors.light
        text: qsTr("Installing step %1/%2 %3%").arg(SoftwareUpdate.currentStep).arg(SoftwareUpdate.totalSteps).arg(SoftwareUpdate.updateProgress)
        horizontalAlignment: Text.AlignHCenter
        font: fonts.secondaryFont(20)
        anchors { top: progress.bottom; topMargin: 20; horizontalCenter: parent.horizontalCenter }
    }

    Text {
        id: warning
        width: parent.width - 40
        wrapMode: Text.WordWrap
        color: colors.red
        text: qsTr("Do not turn off the remote during the installation process!")
        horizontalAlignment: Text.AlignHCenter
        font: fonts.secondaryFont(20)
        anchors { bottom: parent.bottom; bottomMargin: 30; horizontalCenter: parent.horizontalCenter }
    }

    Rectangle {
        id: successScreen
        color: colors.black
        anchors.fill: parent
        opacity: 0
        enabled: opacity === 1

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
        }

        Text {
            id: successTitle
            width: parent.width - 40
            wrapMode: Text.WordWrap
            color: colors.green
            text: qsTr("Update success")
            horizontalAlignment: Text.AlignHCenter
            font: fonts.primaryFont(30)
            anchors { top: parent.top; topMargin: 320; horizontalCenter: parent.horizontalCenter }
        }

        Text {
            width: parent.width - 40
            wrapMode: Text.WordWrap
            color: colors.offwhite
            text: qsTr("Software update was successful.%1The remote will reboot now.").arg("\n");
            horizontalAlignment: Text.AlignHCenter
            font: fonts.secondaryFont(20)
            anchors { top: successTitle.bottom; topMargin: 40; horizontalCenter: parent.horizontalCenter }
        }
    }

    Rectangle {
        id: failedScreen
        color: colors.black
        anchors.fill: parent
        opacity: 0
        enabled: opacity === 1

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
        }

        Text {
            id: failTitle
            width: parent.width - 40
            wrapMode: Text.WordWrap
            color: colors.red
            text: qsTr("Update failed")
            horizontalAlignment: Text.AlignHCenter
            font: fonts.primaryFont(30)
            anchors { top: parent.top; topMargin: 260; horizontalCenter: parent.horizontalCenter }
        }

        Text {
            id: failDescriptiopn
            width: parent.width - 40
            wrapMode: Text.WordWrap
            color: colors.offwhite
            text: qsTr("There was an error during installing the update.")
            horizontalAlignment: Text.AlignHCenter
            font: fonts.secondaryFont(20)
            anchors { top: failTitle.bottom; topMargin: 40; horizontalCenter: parent.horizontalCenter }
        }

        Text {
            id: failMessage
            width: parent.width - 40
            wrapMode: Text.WordWrap
            color: colors.offwhite
            horizontalAlignment: Text.AlignHCenter
            font: fonts.secondaryFont(20)
            anchors { top: failDescriptiopn.bottom; topMargin: 30; horizontalCenter: parent.horizontalCenter }
        }

        Components.Button {
            width: parent.width - 40
            text: qsTr("Back")
            trigger: function() { updateProgress.close(); }
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        }
    }
}
