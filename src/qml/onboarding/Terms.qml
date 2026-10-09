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

    Components.TitleBar {
        id: title
        // onboarding steps have no back target: BACK goes to the previous step
        action: ""
        text: qsTr("Terms & conditions")
    }

    Text {
        id: description
        width: parent.width - 40
        wrapMode: Text.WordWrap
        color: colors.textPrimary
        horizontalAlignment: Text.AlignHCenter
        //: Onboarding terms step; "them" are the Terms & conditions. Selecting the QR code (touch or OK) shows the terms on the remote
        text: qsTr("By using Unfolded Circle products you agree to the Terms & conditions.\n\nYou can read them on\nunfoldedcircle.com/legal\nor by scanning this QR code.\nSelect the QR code to read them on the screen.")
        anchors { horizontalCenter: parent.horizontalCenter; top: title.bottom }
        font: fonts.prose()
        lineHeight: fonts.proseLineHeight
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

        // the code shrinks to the space the text leaves, so a long translation does not run under it
        Image {
            width: Math.max(0, Math.min(300, qrCode.height - 20))
            height: width
            fillMode: Image.PreserveAspectFit
            antialiasing: false
            source: "data:image/png;base64," + ui.createQrCode("https://unfoldedcircle.com/legal")
            anchors.centerIn: parent

            Components.Selectable {
                anchors.margins: -4
                selected: qrCode.activeFocus
            }
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
        width: parent.width - 40
        height: 80
        color: colors.bg
        anchors { bottom: parent.bottom; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }

        Components.Button {
            id: buttonCancel
            text: qsTr("Cancel")
            variant: "secondary"
            width: (parent.width - 20) / 2
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
            width: (parent.width - 20) / 2
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
            color: colors.bg
        }

        Components.TitleBar {
            action: "close"
            text: qsTr("Terms & conditions")
            goBack: function() {
                if (termsPopup.opened) {
                    termsPopup.close();
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
