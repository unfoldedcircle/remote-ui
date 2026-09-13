// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Onboarding 1.0
import Dock.Controller 1.0

import Haptic 1.0
import Wifi 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/components/docks" as Docks
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: dockStep

    // The keypad selection walks the discovery component (start screen controls, then the found
    // docks) and ends on the Skip button below it. It is driven through the button navigation
    // only - no keyboard focus is involved, so one key press can not act on two controls.
    property bool skipSelected: false

    onStepEntered: dockStep.skipSelected = false

    Component.onCompleted: {
        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         if (!dockStep.skipSelected && !dockDiscovery.moveSelection(1)) {
                                                             dockStep.skipSelected = true;
                                                         }
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         if (dockStep.skipSelected) {
                                                             dockStep.skipSelected = false;
                                                             dockDiscovery.selectLast();
                                                         } else {
                                                             dockDiscovery.moveSelection(-1);
                                                         }
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         if (dockStep.skipSelected) {
                                                             skipButton.activate();
                                                         } else {
                                                             dockDiscovery.activateSelection();
                                                         }
                                                     }
                                                 },
                                                 "BACK": {
                                                     "pressed": function() {
                                                         DockController.stopDiscovery();
                                                         OnboardingController.previousStep();
                                                     }
                                                 }
                                             });
    }

    Item {
        id: title
        width: parent.width
        height: 60

        Text {
            id: titleText
            //: Smart charging dock
            text: qsTr("Dock setup")
            width: parent.width
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }
    }

    Docks.Discovery {
        id: dockDiscovery
        anchors {
            top: title.bottom
            topMargin: 20
            bottom: skipButton.top
            bottomMargin: 20
            left: parent.left
            right: parent.right
        }
        anchors.fill: undefined
        keypadSelected: !dockStep.skipSelected
        onSkip: OnboardingController.nextStep()
    }

    Components.Button {
        id: skipButton
        width: parent.width - 20
        text: qsTr("Skip")
        color: colors.secondaryButton
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        highlight: dockStep.skipSelected && ui.keyNavigationActive
        trigger: function() {
            OnboardingController.nextStep();
        }
    }

    Popup {
        id: dockSetupPopup
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        parent: Overlay.overlay

        // the popup's own navigation is the fallback owner; the current setup step takes the input
        // on top of it once the popup is visible
        onOpened: {
            dockSetupPopupButtonNavigation.takeControl();
            if (dockSetupLoader.item) {
                dockSetupLoader.item.activateCurrentStep();
            }
        }

        onClosed: {
            dockSetupPopupButtonNavigation.releaseControl();
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
                ScriptAction { script: dockSetupLoader.active = false; }
            }
        }

        background: Rectangle { color: colors.black }

        contentItem: Loader {
            id: dockSetupLoader
            active: false
            asynchronous: true
            source: "qrc:/components/docks/Setup.qml"

            Behavior on y {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
            }

            onStatusChanged: {
                if (status == Loader.Ready && dockSetupPopup.opened) {
                    dockSetupLoader.item.activateCurrentStep();
                }
            }

            Connections {
                target: dockSetupLoader.item
                ignoreUnknownSignals: true

                function onHome() {
                    dockSetupPopup.close();
                }

                function onDone() {
                    dockSetupPopup.close();
                    OnboardingController.nextStep();
                }

                function onFailed() {
                    dockSetupPopup.close();
                    dockDiscovery.startMessageContainer.opacity = 1;
                }
            }
        }

        Connections {
            target: DockController
            ignoreUnknownSignals: true

            // if a dock is selected for setup, we open the popup
            function onDockToSetupChanged(dockId) {
                if (dockId) {
                    dockSetupLoader.active = true;
                    dockSetupPopup.open();
                }
            }
        }

        // leaving the popup cancels the setup that is running in it; the popup closes on the
        // setup's done / failed signal
        function cancelSetup() {
            if (dockSetupLoader.item) {
                dockSetupLoader.item.cancel();
            } else {
                dockSetupPopup.close();
            }
        }

        Components.ButtonNavigation {
            id: dockSetupPopupButtonNavigation
            defaultConfig: {
                "HOME": {
                    "pressed": function() {
                        dockSetupPopup.cancelSetup();
                    }
                },
                "BACK": {
                    "pressed": function() {
                        dockSetupPopup.cancelSetup();
                    }
                }
            }
        }
    }
}
