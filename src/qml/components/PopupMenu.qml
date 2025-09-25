// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Haptic 1.0

import "qrc:/components" as Components

Popup {
    id: popupMenu
    width: parent.width; height: parent.height
    y: 500
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    enter: Transition {
        SequentialAnimation {
            ParallelAnimation {
                PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { properties: "y"; from: 500; to: 0; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    }

    exit: Transition {
        SequentialAnimation {
            PropertyAnimation { properties: "y"; from: 0; to: 500; easing.type: Easing.InExpo; duration: 300 }
            PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0 }
        }
    }

    property string title
    property var menuItems: []
    property bool footerSelected: false
    property var closeCallback: function() {}

    onOpened: {
        ui.setTimeOut(1500, () => { buttonNavigation.takeControl(); });
    }

    onClosed: {
        menuItemsListView.currentIndex = 0;
        footerSelected = false;
        buttonNavigation.releaseControl();
        closeCallback();
        closeCallback = function() {};
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    if (menuItemsListView.currentIndex == menuItemsListView.count-1 && !footerSelected) {
                        footerSelected = true;
                    } else {
                        menuItemsListView.incrementCurrentIndex();
                    }
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    if (menuItemsListView.currentIndex == menuItemsListView.count-1 && footerSelected) {
                        footerSelected = false;
                    } else {
                        menuItemsListView.decrementCurrentIndex();
                    }
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (footerSelected) {
                        close();
                    } else {
                       menuItemsListView.currentItem.callBack();
                    }
                }
            },
            "BACK": {
                "pressed": function() {
                    popupMenu.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    popupMenu.close();
                }
            }
        }
    }

    background: Item {}

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: popupMenu.opened

        onClicked: popupMenu.close()
    }

    Item {
        id: gradient
        width: parent.width; height: parent.height
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }

        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, parent.height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 0.5; color: colors.black }
                GradientStop { position: 1.0; color: colors.black }
            }
        }
    }

    ListView {
        id: menuItemsListView
        width: parent.width
        height: 80 * (menuItems.length + 2) > parent.height ? parent.height : 80 * (menuItems.length + 2)
        anchors { bottom: parent.bottom }

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        pressDelay: 200

        interactive: false

        model: menuItems
        header: headerItem
        delegate: menuItem
        footer: footerItem
    }

    Component {
        id: headerItem

        Rectangle {
            width: parent.width
            height: 80
            color: colors.black

            Text {
                id: title
                width: parent.width
                color: colors.offwhite
                text: popupMenu.title
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
                font: fonts.primaryFont(24, "Bold")
            }
        }
    }

    Component {
        id: menuItem

        Rectangle {
            width: parent.width - 20
            height: 80
            color: ListView.isCurrentItem && !footerSelected ? colors.dark : colors.black
            radius: ui.cornerRadiusSmall
            border {
                color: ListView.isCurrentItem && !footerSelected ? colors.medium : colors.transparent
                width: 1
            }
            anchors.horizontalCenter: parent.horizontalCenter

            function callBack() {
                closeCallback = function() { menuItems[index].callback(); };
                close();
            }

            Components.Icon {
                id: icon
                color: colors.offwhite
                icon: menuItems[index].icon
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter; }
                size: 60
            }

            Text {
                id: title
                width: parent.width - icon.width - 40;
                color: colors.offwhite
                text: menuItems[index].title
                elide: Text.ElideRight
                anchors { left: icon.right; leftMargin: 10; verticalCenter: parent.verticalCenter }
                font: fonts.primaryFont(28)
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    callBack();
                }
            }
        }
    }

    Component {
        id: footerItem

        Rectangle {
            width: parent.width - 20
            height: 80
            color: footerSelected ? colors.dark : colors.black
            radius: ui.cornerRadiusSmall
            border {
                color: footerSelected ? colors.medium : colors.transparent
                width: 1
            }
            anchors.horizontalCenter: parent.horizontalCenter

            Components.Icon {
                id: icon
                color: colors.offwhite
                icon: "uc:xmark"
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter; }
                size: 60
            }

            Text {
                id: title
                color: colors.offwhite
                //: As in close the menu
                text: qsTr("Close")
                anchors { left: icon.right; leftMargin: 10; verticalCenter: parent.verticalCenter }
                font: fonts.primaryFont(30)
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    Haptic.play(Haptic.Click);
                    close();
                }
            }
        }
    }
}
