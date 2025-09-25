// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Onboarding 1.0
import Haptic 1.0
import ResourceTypes 1.0

import "qrc:/components" as Components
import "qrc:/settings/about" as AboutComponent

Item {
    Components.ButtonNavigation {
        overrideActive: OnboardingController.currentStep === OnboardingController.Terms
        defaultConfig: {
            "DPAD_MIDDLE": {
                "pressed": function() {
                    OnboardingController.nextStep();
                }
            },
            "BACK": {
                "pressed": function() {
                    OnboardingController.previousStep();
                }
            },
        }
    }

    Item {
        id: title
        width: parent.width
        height: 60

        Text {
            text: qsTr("Terms & conditions")
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
        text: qsTr("By using Unfolded Circle products you agree to the Terms & conditions.\n\nYou can read them on\nunfoldedcircle.com/legal\nor by scanning this QR code.\nTap the QR code to show it on the screen.")
        anchors { horizontalCenter: parent.horizontalCenter; top: title.bottom }
        font: fonts.secondaryFont(24)
    }

    Image {
        width: 300
        height: width
        fillMode: Image.PreserveAspectFit
        antialiasing: false
        source: "data:image/png;base64," + ui.createQrCode("https://unfoldedcircle.com/legal")
        anchors { top: description.bottom; horizontalCenter: parent.horizontalCenter; bottom: buttons.top }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                if (termsPopup.closed) {
                    termsPopup.open();
                }
            }
        }
    }

    Rectangle {
        id: buttons
        width: parent.width
        height: 80
        color: colors.black
        anchors.bottom: parent.bottom

        Components.Button {
            id: buttonCancel
            text: qsTr("Cancel")
            color: colors.secondaryButton
            width: (parent.width - 30 ) / 2
            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            trigger: function() {
                OnboardingController.previousStep();
            }
        }

        Components.Button {
            //: Agree to terms and conditions
            text: qsTr("Agree")
            width: (parent.width - 30 ) / 2
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
            trigger: function() {
                OnboardingController.nextStep();
            }
        }
    }

    Popup {
        id: termsPopup
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        background: Rectangle {
            color: colors.black
        }

        Item {
            width: parent.width
            height: 60

            Text {
                text: qsTr("Terms & conditions")
                width: parent.width
                elide: Text.ElideRight
                color: colors.offwhite
                verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                anchors.centerIn: parent
                font: fonts.primaryFont(24)
            }

            Components.Icon {
                color: colors.offwhite
                icon: "uc:xmark"
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }

                Components.HapticMouseArea {
                    width: parent.width + 20; height: parent.height + 20
                    anchors.centerIn: parent
                    enabled: termsPopup.opened
                    onClicked: {
                        termsPopup.close();
                    }
                }
            }
        }

        AboutComponent.AboutPage {
            id: terms
            topNavigation.visible: false
            type: ResourceTypes.Terms
        }
    }
}
