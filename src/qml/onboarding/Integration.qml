// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Onboarding 1.0
import Integration.Controller 1.0

import Haptic 1.0

import "qrc:/components" as Components
import "qrc:/components/integrations" as Integrations

Item {
    id: integrationSetup

    property bool integrationHasBeenSetup: false

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
        anchors { top: integrationSetupTitle.bottom; bottom: skipButton.top; bottomMargin: 20; left: parent.left; right: parent.right }
        anchors.fill: undefined
    }

    Components.Button {
        id: skipButton
        width: parent.width - 40
        text: integrationSetup.integrationHasBeenSetup ? qsTr("Next") : qsTr("Skip")
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
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

        Components.ButtonNavigation {
            id: integrationSetupPopupButtonNavigation
            defaultConfig: {
                "HOME": {
                    "released": function() {
                        integrationSetupPopup.close();
                        goHome();
                    }
                },
                "BACK": {
                    "released": function() {
                        integrationSetupPopup.close();
                    }
                }
            }
        }
    }

    Connections {
        target: OnboardingController
        ignoreUnknownSignals: true

        function onCurrentStepChanged() {
            if (OnboardingController.currentStep == OnboardingController.Integration) {
                IntegrationController.startDriverDiscovery();
            }
            IntegrationController.startDriverDiscovery();  // remove
        }
    }
}
