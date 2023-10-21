// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Config 1.0
import Haptic 1.0
import Battery 1.0
import Wifi 1.0
import Wifi.SignalStrength 1.0
import SoftwareUpdate 1.0

import "qrc:/components" as Components

Item {
    width: parent.width; height: 40

    property int contentY: containerMain.item.currentPage ? containerMain.item.currentPage.contentY : 0
    property int atYBeginning: containerMain.item.currentPage ? containerMain.item.currentPage.atYBeginning : 0
    property int pageNameOffset: 100
    property int scrollDiff:0
    property bool movementStarted: false

    onAtYBeginningChanged: {
        if (atYBeginning) {
            bg.opacity = 0;
            timeText.opacity = 1;
            pageNameOffset = 100;
        }
    }

    Connections {
        target: ui.pages.count > 0 ? containerMain.item.currentPage : null
        ignoreUnknownSignals: true
        enabled: !ui.editMode

        function onContentYChanged() {
            if (!movementStarted) {
                scrollDiff = Math.round(containerMain.item.currentPage.contentY);
                movementStarted = true;
            }

            let contantYRound = Math.round(containerMain.item.currentPage.contentY);
            let diff = scrollDiff  - contantYRound;

            if (contantYRound > -200 && diff < 0) {
                bg.opacity = 1;
                timeText.opacity = 0;
                pageNameOffset = 0;
                movementStarted = false;
            }

            if (contantYRound < -200 && diff > 0) {
                bg.opacity = 0;
                timeText.opacity = 1;
                pageNameOffset = 100;
                movementStarted = false;
            }

        }

        function onMovementStarted() {
            movementStarted = true;
            scrollDiff = Math.round(containerMain.item.currentPage.contentY);
        }

        function onMovementEnded() {
            movementStarted = true;
        }
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        color: colors.black
        opacity: 0

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
        }
    }

    Item {
        anchors.fill: parent
        opacity: ui.editMode ? 0.8 : 0
        layer.enabled: true

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
        }

        Rectangle {
            width: parent.width
            height: parent.height / 2
            color: colors.red
            anchors.top: parent.top
        }

        Rectangle {
            width: parent.width
            height: parent.height
            color: colors.red
            radius: statusBar.height / 2
            anchors.bottom: parent.bottom
        }

        Text {
            color: colors.offwhite
            text: qsTr("Reorder")
            anchors.centerIn: parent
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font: fonts.statusbarClock
        }
    }

    // time
    Text {
        id: timeText
        color: colors.offwhite; opacity: 1
        text: Config.clock24h ? Qt.formatTime(ui.time,"hh:mm") : Qt.formatTime(ui.time,"hh:mm a")
        height: parent.height
        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
        verticalAlignment: Text.AlignVCenter
        font: fonts.statusbarClock

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: containerMain.item.currentPage.positionViewAtBeginning();
    }

    RowLayout {
        id: leftIconsContainer

        Layout.alignment: Qt.AlignVCenter

        anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 5 }
        spacing: 5

        // loading indicator
        Item {
            id: integrationLoadingIndicator

            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignVCenter

            visible: show

            property bool show: ui.isConnecting
            property int circleSize: 14

            Components.HapticMouseArea {
                anchors.fill: parent
                enabled: ui.isConnecting
                onClicked: {
                    loadSecondContainer("qrc:/components/ConnectionStatus.qml");
                }
            }

            Rectangle {
                id: fillCircle
                width: integrationLoadingIndicator.circleSize; height: integrationLoadingIndicator.circleSize
                radius: 7
                color: colors.offwhite
                x: 0
                z: 1
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: outlineCircle
                width: integrationLoadingIndicator.circleSize; height: integrationLoadingIndicator.circleSize
                radius: integrationLoadingIndicator.circleSize/2
                color: colors.offwhite
                opacity: 0.3
                x: 20
                z: 2
                anchors.verticalCenter: parent.verticalCenter
            }

            SequentialAnimation {
                id: integrationLoadinganimation
                running: show
                loops: Animation.Infinite

                ParallelAnimation {
                    NumberAnimation { target: fillCircle; properties: "z"; to: 1; duration: 1  }
                    NumberAnimation { target: outlineCircle; properties: "z"; to: 2; duration: 1  }
                }

                ParallelAnimation {
                    NumberAnimation { target: fillCircle; properties: "x"; to: integrationLoadingIndicator.circleSize; easing.type: Easing.OutExpo; duration: 400  }
                    NumberAnimation { target: outlineCircle; properties: "x"; to: 0; easing.type: Easing.OutExpo; duration: 400  }
                }

                PauseAnimation { duration: 500 }

                ParallelAnimation {
                    NumberAnimation { target: fillCircle; properties: "z"; to: 2; duration: 1  }
                    NumberAnimation { target: outlineCircle; properties: "z"; to: 1; duration: 1  }
                }

                ParallelAnimation {
                    NumberAnimation { target: fillCircle; properties: "x"; to: 0; easing.type: Easing.OutExpo; duration: 400  }
                    NumberAnimation { target: outlineCircle; properties: "x"; to: integrationLoadingIndicator.circleSize; easing.type: Easing.OutExpo; duration: 400  }
                }

                PauseAnimation { duration: 500 }
            }
        }

        // notification indicator
        Rectangle {
            id: notificationIndicator

            Layout.alignment: Qt.AlignVCenter

            width: 12; height: 12
            radius: 6
            color: colors.red
            visible: ui.coreConnected ? 0 : 1
        }

        // software update indicator
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 5

            width: 30; height: 30
            radius: 15
            color: SoftwareUpdate.updateDownloadState === SoftwareUpdate.Downloaded ? colors.red : colors.medium
            visible: SoftwareUpdate.updateAvailable ? 1 : 0

            Components.Icon {
                icon: "uc:down-arrow"
                size: 30
                color: colors.light
            }
        }

        // wifi icon, only shown when there's an issue
        Components.Icon {
            Layout.leftMargin: -10
            Layout.rightMargin: -10

            icon: "uc:wifi-03"
            color: colors.offwhite
            opacity: 0.5
            size: 60
            visible: !Wifi.isConnected || Wifi.currentNetwork.signalStrength === SignalStrength.NONE ||  Wifi.currentNetwork.signalStrength === SignalStrength.WEAK

            Components.Icon {
                size: 60
                icon: {
                    switch (Wifi.currentNetwork.signalStrength) {
                    case SignalStrength.NONE:
                        return "";
                    case SignalStrength.WEAK:
                        return "uc:wifi-01";
                    default:
                        return "";
                    }
                }
                opacity: icon === "" ? 0 : 1
                anchors.centerIn: parent
            }

            Rectangle {
                width: 30
                height: 2
                color: colors.red
                rotation: -45
                transformOrigin: Item.Center
                anchors.centerIn: parent
                visible: !Wifi.isConnected
            }
        }

        // battery icon
        MouseArea {
            Layout.preferredHeight: batteryIcon.height
            Layout.preferredWidth: batteryIcon.width
            Layout.alignment: Qt.AlignVCenter

            pressAndHoldInterval: 500
            onPressAndHold: batteryIcon.showPercentage = !batteryIcon.showPercentage

            RowLayout {
                id: batteryIcon
                Layout.alignment: Qt.AlignVCenter
                spacing: 0

                property bool showPercentage: false

                Text {
                    Layout.alignment: Qt.AlignVCenter

                    color: colors.offwhite
                    text: Battery.level
                    verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                    font: fonts.primaryFontCapitalized(22)
                    visible: Battery.isCharging || batteryIcon.showPercentage
                }

                Components.Icon {
                    id: chargingIcon
                    icon: "uc:charging"
                    color: colors.offwhite
                    size: 40
                    visible: Battery.isCharging
                }

                Item {
                    Layout.leftMargin: 5
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignVCenter

                    visible: !Battery.isCharging

                    Rectangle {
                        width: parent.width
                        height: (parent.height * Battery.level / 100) + ( Battery.level < 10 ? 2 : 0)
                        radius: 4
                        color: Battery.low ? colors.red : colors.offwhite
                        opacity: 0.8
                        anchors { horizontalCenter: batteryBottom.horizontalCenter; bottom: batteryBottom.bottom; bottomMargin: 1 }
                    }

                    Rectangle {
                        id: batteryBottom
                        width: parent.width; height: parent.height
                        radius: 4
                        color: colors.offwhite
                        opacity: 0.3
                        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
                    }
                }
            }
        }

        // profile icon
        Item {
            id: profileIcon

            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.leftMargin: 5
            Layout.alignment: Qt.AlignRight || Qt.AlignVCenter

            Rectangle {
                width: parent.width; height: parent.height
                radius: 16
                color: colors.offwhite
                opacity: 0.8
                anchors.centerIn: parent
            }

            Text {
                color: colors.black
                text: ui.profile.name.substring(0,1);
                verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter

                anchors.centerIn: parent
                font: fonts.primaryFontCapitalized(18)
            }
        }
    }

    // page name
    Item {
        id: pageNameTextContainer
        height: parent.height
        anchors { left: parent.left; right: leftIconsContainer.left; rightMargin: 10 }
        clip: true

        Text {
            id: pageNameText
            color: colors.offwhite
            text: containerMain.item.currentPage ? containerMain.item.currentPage.title : ""
            height: parent.height
            anchors { left: parent.left; verticalCenter: parent.verticalCenter; verticalCenterOffset: pageNameOffset }
            verticalAlignment: Text.AlignVCenter
            font: fonts.statusbarClock

            Behavior on anchors.verticalCenterOffset {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    }

    Components.HapticMouseArea {
        width: profileIcon.width + 20; height: profileIcon.height + 20
        anchors { right: parent.right; verticalCenter: parent.verticalCenter }

        onClicked: {
            loadSecondContainer("qrc:/components/Profile.qml");
        }
    }
}
