// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Onboarding 1.0
import Haptic 1.0
import ResourceTypes 1.0

import "qrc:/components" as Components
import "qrc:/settings/about" as AboutComponent
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: termsStep

    // the terms must be agreed to deliberately: the selection starts on Cancel
    initialFocusItem: buttonCancel

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

    Item {
        id: qrCode
        anchors { top: description.bottom; horizontalCenter: parent.horizontalCenter; bottom: buttons.top }
        width: 300

        KeyNavigation.down: buttonCancel

        function showTerms() {
            if (termsPopup.closed) {
                termsPopup.open();
            }
        }

        Keys.onReturnPressed: {
            qrCode.showTerms();
            event.accepted = true;
        }

        Image {
            width: 300
            height: width
            fillMode: Image.PreserveAspectFit
            antialiasing: false
            source: "data:image/png;base64," + ui.createQrCode("https://unfoldedcircle.com/legal")
            anchors.centerIn: parent
        }

        Rectangle {
            anchors { fill: parent; margins: -4 }
            radius: ui.cornerRadiusSmall
            color: colors.transparent
            border { width: 2; color: qrCode.activeFocus && ui.keyNavigationActive ? colors.highlight : colors.transparent }
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                qrCode.showTerms();
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
            KeyNavigation.up: qrCode
            KeyNavigation.right: buttonAgree
            trigger: function() {
                OnboardingController.previousStep();
            }
        }

        Components.Button {
            id: buttonAgree
            //: Agree to terms and conditions
            text: qsTr("Agree")
            width: (parent.width - 30 ) / 2
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
            KeyNavigation.up: qrCode
            KeyNavigation.left: buttonCancel
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

        // the about page scrolls with DPAD_UP/DOWN through its own button navigation, so it owns
        // the input while the popup is open; BACK closes the popup again
        onOpened: terms.buttonNavigation.takeControl()
        onClosed: terms.buttonNavigation.releaseControl()

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

            Component.onCompleted: {
                buttonNavigation.extendDefaultConfig({
                                                         "BACK": {
                                                             "pressed": function() {
                                                                 termsPopup.close();
                                                             }
                                                         },
                                                         "HOME": {
                                                             "pressed": function() {
                                                                 termsPopup.close();
                                                             }
                                                         }
                                                     });
            }
        }
    }
}
