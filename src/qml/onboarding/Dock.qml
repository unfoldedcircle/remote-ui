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

Item {
    property bool currentItem: SwipeView.isCurrentItem
    property bool dockSelected: false

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
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        anchors.fill: undefined
        onSkip: OnboardingController.nextStep()
    }

    Popup {
        id: dockSetupPopup
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        parent: Overlay.overlay

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

            Connections {
                target: dockSetupLoader.item
                ignoreUnknownSignals: true

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
    }
}
