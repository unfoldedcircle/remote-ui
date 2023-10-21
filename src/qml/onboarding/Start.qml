// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Haptic 1.0
import Wifi 1.0

import "qrc:/components" as Components

Item {
    property var greetings: ["Hallo", "Hoi", "Szia", "Hej", "Hei", "Hello"]
    property int count: 0

    Timer {
        repeat: true
        running: OnboardingController.currentStep === OnboardingController.Start
        interval: 5000
        onTriggered: {
            switchAnimation.start();
            count++;
            if (count > greetings.length - 1)  {
                count = 0;
            }
        }
    }


    SequentialAnimation {
        id: switchAnimation
        running: false
        alwaysRunToEnd: true

        NumberAnimation { target: greetText; properties: "opacity"; to: 0; duration: 400;  }
        PropertyAction { target: greetText; property: "text"; value: greetings[count] }
        NumberAnimation { target: greetText; properties: "opacity"; to: 1; duration: 400;  }
    }


    Text {
        id: greetText
        color: colors.offwhite
        text: "Hello"
        width: parent.width
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors { centerIn: parent; verticalCenterOffset: -50 }
        font: fonts.primaryFont(70)
    }

    Text {
        color: colors.light
        text: qsTr("Tap the screen to begin")
        width: parent.width
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors { horizontalCenter: parent.horizontalCenter; top: greetText.bottom }
        font: fonts.secondaryFont(28)
    }

    Components.HapticMouseArea {
        anchors.fill: parent
        onClicked: {
            OnboardingController.nextStep();
        }
    }
}
