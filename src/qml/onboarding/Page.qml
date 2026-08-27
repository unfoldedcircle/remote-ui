// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 ONBOARDING PAGE

 Base of every onboarding step. It owns the key input while its step is on screen, the same way
 Settings.Page does for a settings page: the step takes control of the input controller when the
 swipe view shows it and releases it again when it leaves, so popups opened on top of a step
 (language list, profile switch, dock setup, ...) stack above it and get the keys instead.

 A step navigates through the QML focus chain (KeyNavigation on its controls, Keys handlers on
 Components.Button / Components.Switch); set initialFocusItem for the control that should be
 selected on entry and scrollTarget for the step's Flickable to keep the selection on screen.
 Extend buttonNavigation with extendDefaultConfig() for keys that do not go through the focus
 chain (BACK is already bound to the previous step).
**/

import QtQuick 2.15
import QtQuick.Controls 2.15

import Onboarding 1.0

import "qrc:/components" as Components

Item {
    id: onboardingPage

    property alias buttonNavigation: buttonNavigation
    property alias initialFocusItem: buttonNavigation.initialFocusItem
    property alias scrollTarget: buttonNavigation.scrollTarget

    // Emitted once the step owns the input: the swipe view moved to this step. Steps use this to
    // start their work (network scan, showing their selection list, ...) instead of listening to
    // OnboardingController.currentStepChanged themselves, so the input ownership is settled first.
    // Not named entered()/left(): a signal called "left" shadows the anchor line, and every
    // "anchors.left: parent.left" of a child then fails and leaves the child without a width.
    signal stepEntered()
    signal stepLeft()

    // SwipeView.isCurrentItem, not OnboardingController.currentStep: the step must be attached to
    // the window before it can own the input, and the swipe view only guarantees that for its
    // current item.
    readonly property bool isCurrentStep: SwipeView.isCurrentItem

    function activate() {
        if (isCurrentStep) {
            // a step always opens on its initial control, never on the control it was left on
            buttonNavigation.lastFocusItem = null;
            buttonNavigation.takeControl();
            onboardingPage.stepEntered();
        } else {
            buttonNavigation.releaseControl();
            onboardingPage.stepLeft();
        }
    }

    // Deferred on purpose. OnboardingController emits the step change synchronously, so a step
    // change triggered by a key press would otherwise take the input and focus the next step's
    // control while that very key press is still being delivered: the input controller already
    // fired the handler that moved the step, and the same event then continues into the QML
    // focus chain and activates the freshly focused control of the next step (OK on the start
    // screen ended up agreeing to the terms). Handing over after the event loop turn keeps one
    // key press on one step.
    onIsCurrentStepChanged: Qt.callLater(activate)

    Component.onCompleted: {
        if (isCurrentStep) {
            activate();
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        manageFocus: true
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    OnboardingController.previousStep();
                }
            }
        }
    }
}
