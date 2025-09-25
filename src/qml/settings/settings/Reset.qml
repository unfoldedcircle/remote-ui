// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


import Haptic 1.0
import Config 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: resetPageContent

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        onContentYChanged: {
            if (contentY < 0) {
                contentY = 0;
            }
            if (contentY > 1100) {
                contentY = 1100;
            }
        }

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 20
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height


                Text {
                    id: descriptionText
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Resetting will delete all settings, configuration and any information saved on the remote. Data cannot be recovered. Continue?")
                    font: fonts.secondaryFont(24)
                }

                Components.Button {
                    id: eraseButton
                    width: parent.width
                    anchors { top: descriptionText.bottom; topMargin: 30 }
                    color: colors.red
                    text: qsTr("Erase everything")
                    trigger: function() {
                        ui.getFactoryResetToken();
                        confirmationPopup.open();
                    }
                    // keypad control is not enabled for earese button to avoid accidental presses
                }
            }
        }
    }

    Popup {
        id: confirmationPopup
        x: 0; y:0
        width: parent.width
        height: parent.height
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        padding: 0

        onOpened: {
            confirmationPopupButtonNavigation.takeControl();
        }

        onClosed: {
            confirmationPopupButtonNavigation.releaseControl();
            ui.cancelFactoryReset();
        }

        Components.ButtonNavigation {
            id: confirmationPopupButtonNavigation
            defaultConfig: {
                "BACK": {
                    "pressed": function() {
                        confirmationPopup.close();
                    }
                },
                "HOME": {
                    "pressed": function() {
                        confirmationPopup.close();
                    }
                }
            }
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 300 }
        }

        background: Rectangle {
            color: colors.red
        }

        contentItem: Item {
            Text {
                id: title
                wrapMode: Text.WordWrap
                color: colors.offwhite
                //: Factory reset, after this step, everything is deleted
                text: qsTr("Point of\nno return")
                anchors { left: parent.left; leftMargin: 10; right: parent.right; rightMargin: 10; top: parent.top; topMargin: 10 }
                font: fonts.primaryFont(60)
                lineHeight: 0.8
            }

            Text {
                id: description
                wrapMode: Text.WordWrap
                color: colors.offwhite
                text: qsTr("Confirming factory reset will erase all configuration and data. Data cannot be recovered.")
                anchors { left: parent.left; leftMargin: 10; right: parent.right; rightMargin: 10; top: title.bottom; topMargin: 20 }
                font: fonts.secondaryFont(24)
                lineHeight: 0.8
            }

            Components.Button {
                text: qsTr("Confirm")
                color: colors.offwhite
                textColor: colors.black
                width: parent.width - 20
                anchors { horizontalCenter: parent.horizontalCenter; top: description.bottom; topMargin: 40 }
                trigger: function() {
                    ui.factoryReset();
                }
            }

            Components.Button {
                text: qsTr("Cancel")
                width: parent.width - 20
                anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 10 }
                trigger: function() {
                    confirmationPopup.close();
                }
            }
        }
    }
}
