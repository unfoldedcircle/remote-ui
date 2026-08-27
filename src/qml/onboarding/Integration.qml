// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Onboarding 1.0
import Integration.Controller 1.0

import Haptic 1.0

import "qrc:/components" as Components
import "qrc:/components/integrations" as Integrations
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: integrationSetup

    property bool integrationHasBeenSetup: false

    // The keypad selection walks the found integrations and ends on the Skip/Next button below
    // the list. It is driven through the button navigation only - no keyboard focus is involved,
    // so one key press can not act on two controls.
    property bool skipSelected: false

    onStepEntered: {
        integrationSetup.skipSelected = false;
        IntegrationController.startDriverDiscovery();
    }

    Component.onCompleted: {
        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         if (!integrationSetup.skipSelected && !integrationDiscovery.moveSelection(1)) {
                                                             integrationSetup.skipSelected = true;
                                                         }
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         if (integrationSetup.skipSelected) {
                                                             integrationSetup.skipSelected = false;
                                                             integrationDiscovery.selectLast();
                                                         } else {
                                                             integrationDiscovery.moveSelection(-1);
                                                         }
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         if (integrationSetup.skipSelected) {
                                                             skipButton.activate();
                                                         } else {
                                                             integrationDiscovery.activateSelection();
                                                         }
                                                     }
                                                 },
                                                 "BACK": {
                                                     "pressed": function() {
                                                         IntegrationController.stopDriverDiscovery();
                                                         OnboardingController.previousStep();
                                                     }
                                                 }
                                             });
    }

    Item {
        id: integrationSetupTitle
        width: parent.width
        height: 60

        Text {
            text: qsTr("Integration setup")
            width: parent.width - 20
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }
    }

    Integrations.Discovery {
        id: integrationDiscovery
        anchors { top: integrationSetupTitle.bottom; bottom: skipButton.top; bottomMargin: 20; left: parent.left; right: parent.right }
        anchors.fill: undefined
        keypadSelected: !integrationSetup.skipSelected
    }

    Components.Button {
        id: skipButton
        width: parent.width - 40
        text: integrationSetup.integrationHasBeenSetup ? qsTr("Next") : qsTr("Skip")
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        highlight: integrationSetup.skipSelected && ui.keyNavigationActive
        trigger: function() {
            OnboardingController.nextStep();
        }
    }

    Popup {
        id: integrationSetupPopup
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        parent: Overlay.overlay

        onOpened: {
            integrationSetupPopupButtonNavigation.takeControl();
        }

        onClosed: {
            integrationSetupPopupButtonNavigation.releaseControl();
        }

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.7; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 400 }
        }

        exit: Transition {
            SequentialAnimation {
                ParallelAnimation {
                    NumberAnimation { property: "scale"; from: 1.0; to: 0.7; easing.type: Easing.InExpo; duration: 300 }
                    NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 400 }
                }
                ScriptAction { script: integrationSetupLoader.active = false; }
            }
        }

        background: Rectangle { color: colors.black }

        contentItem: Loader {
            id: integrationSetupLoader
            active: false
            asynchronous: true
            source: "qrc:/components/integrations/Setup.qml"

            Behavior on y {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
            }

            Connections {
                target: integrationSetupLoader.item
                ignoreUnknownSignals: true

                function onDone() {
                    integrationSetupPopup.close();
                }
            }
        }

        Connections {
            target: IntegrationController
            ignoreUnknownSignals: true

            // if an integration is selected for setup, we open the popup
            function onIntegrationDriverToSetupChanged() {
                if (IntegrationController.integrationDriverTosetup) {
                    integrationSetupLoader.active = true;
                    integrationSetupPopup.open();
                }
            }

            function onIntegrationSetupChange(driverId, state, error, data) {
                if (state === IntegrationControllerEnums.Ok) {
                    integrationSetup.integrationHasBeenSetup = true;
                }
            }
        }

        // the setup form is touch only; the keypad can only leave it
        Components.ButtonNavigation {
            id: integrationSetupPopupButtonNavigation
            defaultConfig: {
                "HOME": {
                    "pressed": function() {
                        integrationSetupPopup.close();
                    }
                },
                "BACK": {
                    "pressed": function() {
                        integrationSetupPopup.close();
                    }
                }
            }
        }
    }
}
