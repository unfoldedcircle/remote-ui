// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Onboarding 1.0
import Wifi 1.0

import "qrc:/components" as Components
import "qrc:/onboarding" as OnboardingComponents

Item {
    id: onboardingContainerRoot
    width: parent.width; height: parent.height

    SwipeView {
        id: swipeView
        anchors.fill: parent
        interactive: false
        currentIndex: OnboardingController.currentStep

        OnboardingComponents.Start {}
        OnboardingComponents.Terms {}
        OnboardingComponents.Language {}
        OnboardingComponents.Country {}
        OnboardingComponents.Timezone {}
        OnboardingComponents.Pin {}
        OnboardingComponents.RemoteName {}
        OnboardingComponents.Profile {}
        OnboardingComponents.Wifi {}
        OnboardingComponents.Dock {}
        OnboardingComponents.Integration {}
        OnboardingComponents.Finish {}
    }

    Rectangle {
        width: parent.width
        height: 6
        color: colors.medium
        anchors.top: parent.top
    }

    Rectangle {
        width: parent.width * (swipeView.currentIndex + ( OnboardingController.currentStep == OnboardingController.Start ? 0 : 1 ))/swipeView.count
        height: 6
        color: colors.offwhite
        anchors.top: parent.top

        Behavior on width {
            NumberAnimation { duration: 300; easing.type: Easing.OutExpo }
        }
    }

    Component.onCompleted: keyboard.hide()
}
