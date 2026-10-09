// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 ACTIONABLE NOTIFICATION COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - key
 - title
 - message
 - icon
 - warning
 - actionlabel
 - notificationObj
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Haptic 1.0

import "qrc:/components" as Components

Popup {
    id: actionableNotification
    x: 0; y:0
    width: parent.width
    height: parent.height
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    // keypad selection: the action is preselected, LEFT moves to Cancel
    property bool cancelSelected: false

    onOpened: {
        actionableNotification.cancelSelected = false;
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    function clearAll() {
        actionableNotification.close();

        for (let i = 0; i < notificationList.depth; i++) {
            notificationList.get(i).destroy();
        }

        notificationList.clear();
    }

    Connections {
        target: ui.notification
        ignoreUnknownSignals: true

        function onActionableNotificationCreated(notificationObj) {
            // the same message is not stacked twice, but two different problems are: the title alone
            // is shared by every entity that fails to answer a command
            for (let i = 0; i < notificationList.depth; i++) {
                const shown = notificationList.get(i);
                if (shown && shown.notificationObj.isDuplicateOf(notificationObj)) {
                    return;
                }
            }

            actionableNotification.open();
            notificationList.push(notificationComponent.createObject(notificationList, {notificationObj: notificationObj}));

            // a notification pushed on top of an open one is a new layer taking the input: it starts
            // on the same control as a freshly opened one, not where the selection was left
            actionableNotification.cancelSelected = false;
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    notificationList.currentItem.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    actionableNotification.clearAll();

                    if (notificationList.depth == 1) {
                        actionableNotification.close();
                    }
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (actionableNotification.cancelSelected) {
                        notificationList.currentItem.close();
                        return;
                    }

                    notificationList.currentItem.notificationObj.action();
                    // close() the item, not just the popup: an item left on the stack keeps matching the
                    // duplicate check above and silently swallows every later notification with that title
                    notificationList.currentItem.close();
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    if (notificationList.currentItem && notificationList.currentItem.hasCancel) {
                        actionableNotification.cancelSelected = true;
                    }
                }
            },
            "DPAD_RIGHT": {
                "pressed": function() {
                    actionableNotification.cancelSelected = false;
                }
            },
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 200 }
    }

    background: Item {
        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, parent.height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: colors.transparent }
                GradientStop { position: 0.6; color: colors.black }
                GradientStop { position: 1.0; color: colors.black }
            }
        }

        Rectangle {
            id: colorBar
            width: parent.width - 20
            height: 4
            radius: ui.cornerRadiusSmall
            color: notificationList.currentItem && notificationList.currentItem.notificationObj.itemWarning() ? colors.red : colors.textPrimary
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        }
    }

    SequentialAnimation {
        loops: Animation.Infinite
        running: actionableNotification.opened

        NumberAnimation { target: colorBar; properties: "opacity"; from: 1; to: 0; duration: 1000 }
        NumberAnimation { target: colorBar; properties: "opacity"; from: 0; to: 1; duration: 1000 }
        PauseAnimation { duration: 2000 }
    }

    contentItem: StackView {
        id: notificationList
    }

    Component {
        id: notificationComponent

        MouseArea {
            id: notificationComponentContent

            property QtObject notificationObj
            readonly property bool hasCancel: actionableNotificationAction.text !== ""

            Component.onDestruction: notificationComponentContent.notificationObj.destroy()

            function close() {
                if (notificationList.depth < 2) {
                    actionableNotification.close();
                    notificationList.clear();
                } else {
                    notificationList.pop();
                }

                notificationComponentContent.destroy();
            }
            
            onClicked: notificationComponentContent.close()

            // Cancel and the action are buttons, Cancel first (docs/design-system.md, I-09); a notification
            // without an action label has neither and is dismissed with a tap
            Components.Button {
                id: actionableNotificationAction
                text: notificationObj.itemActionLabel()
                visible: text !== ""
                width: (parent.width - 60) / 2
                height: visible ? 80 : 0
                anchors { right: parent.right; rightMargin: 20; bottom: parent.bottom; bottomMargin: 30 }
                highlight: !actionableNotification.cancelSelected && ui.keyNavigationActive
                trigger: function() {
                    notificationObj.action();
                    notificationComponentContent.close();
                }
            }

            Components.Button {
                text: qsTr("Cancel")
                variant: "secondary"
                visible: actionableNotificationAction.visible
                width: (parent.width - 60) / 2
                anchors { left: parent.left; leftMargin: 20; bottom: parent.bottom; bottomMargin: 30 }
                highlight: actionableNotification.cancelSelected && ui.keyNavigationActive
                trigger: function() {
                    notificationComponentContent.close();
                }
            }

            Text {
                id: actionableNotificationMessage
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                color: colors.textPrimary
                text: notificationObj.itemMessage()
                anchors { left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20; bottom: actionableNotificationAction.top; bottomMargin: 40  }
                font: fonts.prose()
                lineHeight: fonts.proseLineHeight
            }

            Text {
                id: actionableNotificationTitle
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                color: colors.offwhite
                text: notificationObj.itemTitle()
                anchors { left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20; bottom: actionableNotificationMessage.top; bottomMargin: 20  }
                font: fonts.primaryFont(40)
                lineHeight: 0.8
            }

            Components.Icon {
                color: notificationObj.itemWarning() ? colors.red : colors.offwhite
                icon: notificationObj.itemIcon() === "" ? "uc:triangle-exclamation" : notificationObj.itemIcon()
                anchors { left: parent.left; bottom: actionableNotificationTitle.top }
                size: 140
            }
        }
    }
}
