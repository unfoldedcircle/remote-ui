// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 NOTIFICATION DRAWER COMPONENT
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
 
import Config 1.0
import Haptic 1.0

import "qrc:/components" as Components

Drawer {
    id: notifications
    width: parent.width
    height: parent.height
    edge: ui.rotateScreen ? Qt.LeftEdge : Qt.TopEdge
    dim: false
    closePolicy: Popup.NoAutoClose
    modal: true

    onOpened: {
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    notifications.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    notifications.close();
                }
            }
        }
    }

    background: Rectangle {
        color: colors.black
    }

    Item {
        anchors.fill: parent

        Text {
            id: displayBrightnessText
            width: parent.width - 40
            wrapMode: Text.WordWrap
            color: colors.offwhite
            text: qsTr("Display brightness")
            anchors { horizontalCenter: parent.horizontalCenter; top:parent.top; topMargin: 20 }
            font: fonts.primaryFont(30)
        }

        Components.Slider {
            id: displayBrightnessSlider
            width: parent.width - 40
            height: 60
            from: 10
            to: 100
            stepSize: 1
            value: Config.displayBrightness
            live: true
            anchors { horizontalCenter: parent.horizontalCenter; top: displayBrightnessText.bottom; topMargin: 10 }

            onValueChanged: {
                Config.displayBrightness = value;
            }

            onUserInteractionEnded: {
                Config.displayBrightness = value;
            }
        }

        Rectangle {
            id: notificationsSeparatorLine
            width: parent.width
            height: 2
            color: colors.medium
            anchors { top: displayBrightnessSlider.bottom; topMargin: 20 }
        }

        ListView {
            id: notificationsListView
            width: parent.width - 20
            anchors { top: notificationsSeparatorLine.bottom; topMargin: 20; bottom: parent.bottom; bottomMargin: 30; horizontalCenter: parent.horizontalCenter }
            maximumFlickVelocity: 6000
            flickDeceleration: 1000
            model: ui.notification.model
            spacing: 10
            clip: true

            delegate: Rectangle {
                width: notifications.width - 20
                height: 120
                color: colors.dark
                radius: ui.cornerRadiusSmall

                Components.Icon {
                    id: notificationIcon
                    color: itemWarning ? colors.red : colors.offwhite
                    icon: itemIcon === "" ? "uc:triangle-exclamation" : itemIcon
                    anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 30 }
                    size: 60
                }

                Item {
                    height: childrenRect.height
                    anchors { left: notificationIcon.right; leftMargin: 30; right: parent.right; rightMargin: 20; verticalCenter: notificationIcon.verticalCenter }

                    Text {
                        id: notificationTitle
                        color: itemWarning ? colors.red : colors.offwhite
                        text: itemTitle
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        width: parent.width
                        anchors { top: parent.top }
                        font: fonts.primaryFont(28)
                        lineHeight: 0.8
                    }

                    Text {
                        color: colors.light
                        text: itemTimettamp
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        width: parent.width
                        anchors { top: notificationTitle.bottom }
                        font: fonts.secondaryFont(22)
                        lineHeight: 0.8
                    }
                }
            }

            footerPositioning: ListView.PullBackFooter
            footer: Components.HapticMouseArea {
                width: notifications.width
                height: 100
                z: 1000
                enabled: notificationsListView.count > 0

                onClicked: {
                    ui.notification.clearAll();
                }

                Rectangle {
                    anchors.fill: parent
                    color: colors.black
                }

                Text {
                    text: notificationsListView.count > 0 ? qsTr("Clear all") : qsTr("No notifications")
                    verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                    color: colors.offwhite
                    font: fonts.secondaryFont(26)
                    anchors.centerIn: parent
                }
            }
        }

        Rectangle {
            width: 80
            height: 6
            radius: 3
            color: colors.offwhite
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 10 }
        }
    }
}
