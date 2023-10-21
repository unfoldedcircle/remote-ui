// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Onboarding 1.0
import Wifi 1.0

import "qrc:/components" as Components
import "qrc:/settings/settings" as Settings

Item {
    id: onboardingWifiPage

    Connections {
        target: OnboardingController
        ignoreUnknownSignals: true

        function onCurrentStepChanged() {
            if (OnboardingController.currentStep == OnboardingController.Wifi) {
                Wifi.getWifiStatus();
                Wifi.startNetworkScan();
                scanTimer.start();
            }
        }
    }

    Connections {
        target: Wifi
        ignoreUnknownSignals: true

        function onConnecting() {
            connectionTimeoutTimer.start();
        }

        function onConnected(success) {
            connectionTimeoutTimer.stop();

            if (success) {
                loading.success(true, function() {
                    OnboardingController.nextStep();
                    Wifi.stopNetworkScan();
                    scanStartTimer.stop();
                    scanTimer.stop();
                });
            } else {
                Wifi.deleteAllNetworks();
                loading.failure(true, function() { wifiFailed.opacity = 1; });
                Wifi.getWifiStatus();
                Wifi.startNetworkScan();
                scanTimer.start();
            }
        }
    }

    Item {
        id: title
        width: parent.width
        height: 60

        Text {
            text: qsTr("Select your WiFi network")
            width: parent.width
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }
    }

    Item {
        id: macAddressContainer
        width: parent.width - 20
        height: 60
        anchors { top: title.bottom; horizontalCenter: parent.horizontalCenter }

        RowLayout {
            width: parent.width
            height: parent.height
            spacing: 20

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Wi-Fi address")
                wrapMode: Text.NoWrap
                elide: Text.ElideNone
                color: colors.offwhite
                font: fonts.primaryFont(20)
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                text: Wifi.macAddress
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignRight
                color: colors.offwhite
                opacity: 0.7
                font: fonts.secondaryFont(20)
            }
        }

    }

    Settings.WifiNetworkList {
        id: wifiNetworkList
        width: parent.width - 20
        anchors { top: macAddressContainer.bottom; bottom: skipStepButton.top; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }
        popupParent: onboardingWifiPage
        interactive: true
        model: Wifi.unfilteredNetworkList
    }

    Components.Button {
        id: skipStepButton
        width: parent.width - 20
        text: qsTr("Skip")
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        trigger: function() {
            OnboardingController.nextStep();
        }
    }

    Rectangle {
        id: wifiFailed
        color: colors.black
        anchors.fill: parent
        opacity: 0
        enabled: opacity === 1

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300}
        }

        Item {
            id: failedTitle
            width: parent.width
            height: 60

            Text {
                //: Failed to connect to a wifi network
                text: qsTr("Failed to connect")
                width: parent.width
                elide: Text.ElideRight
                color: colors.offwhite
                verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                anchors.centerIn: parent
                font: fonts.primaryFont(24)
            }
        }

        Text {
            id: description
            width: parent.width
            wrapMode: Text.WordWrap
            color: colors.light
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Failed to connect to the WiFi network. You can try again or proceed without setting up a WiFi network. You can set up your WiFi network later in Settings. If you skip this step, dock and integration setup won't be possible now.")
            anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
            font: fonts.secondaryFont(24)
        }

        Components.Button {
            id: skipButton
            text: qsTr("Set up later")
            width: parent.width / 2 - 10
            anchors { right: parent.right; bottom: parent.bottom }
            trigger: function() {
                OnboardingController.nextStep();
                OnboardingController.nextStep();
                OnboardingController.nextStep();
            }
        }

        Components.Button {
            text: qsTr("Try again")
            width: parent.width / 2 - 10
            color: colors.secondaryButton
            anchors { left: parent.left; bottom: parent.bottom }
            trigger: function() {
                wifiFailed.opacity = 0;
            }
        }
    }

    Timer {
        id: scanTimer
        repeat: true
        interval: 2000
        running: false

        onTriggered: {
            Wifi.getWifiScanStatus();

            if (!Wifi.scanActive) {
                scanStartTimer.start();
                scanTimer.stop();
            }
        }
    }

    Timer {
        id: scanStartTimer
        repeat: false
        running: false
        interval: 10000

        onTriggered: {
            Wifi.startNetworkScan();
            scanTimer.start();
        }
    }

    Timer {
        id: connectionTimeoutTimer
        repeat: false
        running: false
        interval: 3000
        onTriggered: {
            Wifi.deleteAllNetworks();
            loading.failure(true, function() { wifiFailed.opacity = 1; });
            Wifi.getWifiStatus();
            Wifi.startNetworkScan();
            scanTimer.start();
        }
    }
}
