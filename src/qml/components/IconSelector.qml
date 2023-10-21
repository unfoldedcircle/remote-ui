// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import "qrc:/components" as Components

Popup {
    id: iconSelectorPopup
    width: parent.width
    height: parent.height
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    onOpened: {
        ui.setTimeOut(300, ()=>{ buttonNavigation.takeControl(); });
        iconGrid.forceActiveFocus();
        iconGrid.currentIndex = 0;
        tabBar.currentIndex = 0;
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    signal iconSelected(string icon)

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "HOME": {
                "released": function() {
                    iconSelectorPopup.close();
                }
            },
            "BACK": {
                "released": function() {
                    iconSelectorPopup.close();
                }
            },
            "DPAD_MIDDLE": {
                "released": function() {
                    iconSelectorPopup.iconSelected(iconGridSwipeView.currentIndex === 0 ? iconGrid.currentItem.icon : iconGridCustom.currentItem.icon);
                    iconSelectorPopup.close();
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "scale"; from: 0.7; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "scale"; from: 1.0; to: 0.7; easing.type: Easing.InExpo; duration: 300 }
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 300 }
    }

    background: Rectangle { color: colors.black; }
    contentItem: ColumnLayout {
        spacing: 20

        Text {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            text: qsTr("Select icon")
            elide: Text.ElideRight
            maximumLineCount: 1
            color: colors.offwhite
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font: fonts.primaryFont(30)
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TabBar {
                id: tabBar
                width: parent.width - 20
                implicitHeight: 60
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

                background: Rectangle {
                    color: colors.dark
                    radius: ui.cornerRadiusLarge
                    border {
                        color: colors.medium
                        width: 1
                    }
                }

                TabButton {
                    id: unfoldedTabButton
                    text: qsTr("Unfolded Icons")
                    implicitHeight: 60

                    contentItem: Text {
                        text: unfoldedTabButton.text
                        font: fonts.secondaryFont(22)
                        color: colors.offwhite
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        color: tabBar.currentIndex == 0 ? colors.primaryButton : colors.transparent
                        radius: ui.cornerRadiusLarge
                    }
                }

                TabButton {
                    id: customTabButton
                    text: qsTr("Custom Icons")
                    implicitHeight: 60

                    contentItem: Text {
                        text: customTabButton.text
                        font: fonts.secondaryFont(22)
                        color: colors.offwhite
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        color: tabBar.currentIndex == 1 ? colors.primaryButton : colors.transparent
                        radius: ui.cornerRadiusLarge
                    }
                }
            }

            SwipeView {
                id: iconGridSwipeView
                width: parent.width
                anchors { horizontalCenter: parent.horizontalCenter; top: tabBar.bottom; topMargin: 20; bottom: parent.bottom }

                currentIndex: tabBar.currentIndex
                interactive: false
                clip: true

                onCurrentIndexChanged: {
                    switch(currentIndex) {
                    case 0:
                        iconGrid.forceActiveFocus();
                        break;
                    case 1:
                        iconGridCustom.forceActiveFocus();
                        break;
                    }
                }

                GridView {
                    id: iconGrid

                    cellWidth: 120; cellHeight: 120

                    model: resource.getIconList()
                    clip: true
                    pressDelay: 100
                    keyNavigationEnabled: true
                    focus: true

                    delegate: Item {
                        width: GridView.view.cellWidth
                        height: width

                        property bool currentItem: GridView.isCurrentItem
                        property string icon: modelData

                        Rectangle {
                            id: iconContainer
                            width: parent.width - 20
                            height: width
                            anchors.centerIn: parent
                            color: colors.transparent
                            radius: ui.cornerRadiusSmall
                            border {
                                width: 2
                                color: currentItem ? colors.offwhite : colors.transparent
                            }

                            Behavior on color {
                                ColorAnimation { duration: 300 }
                            }

                            Components.Icon {
                                size: parent.width
                                icon: modelData
                                color: colors.offwhite
                            }

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onPressed: iconContainer.color = colors.offwhite
                                onReleased: iconContainer.color = colors.transparent
                                onCanceled: iconContainer.color = colors.transparent
                                onClicked: {
                                    iconContainer.color = colors.offwhite
                                    iconSelectorPopup.iconSelected(modelData);
                                    iconSelectorPopup.close();
                                }
                            }
                        }
                    }
                }

                GridView {
                    id: iconGridCustom

                    cellWidth: 120; cellHeight: 120

                    model: resource.getCustomIconList()
                    clip: true
                    pressDelay: 100
                    keyNavigationEnabled: true
                    focus: true

                    delegate: Item {
                        width: GridView.view.cellWidth
                        height: width

                        property bool currentItem: GridView.isCurrentItem
                        property string icon: modelData

                        Rectangle {
                            id: customIconContainer
                            width: parent.width - 20
                            height: width
                            anchors.centerIn: parent
                            color: colors.transparent
                            radius: ui.cornerRadiusSmall
                            border {
                                width: 2
                                color: currentItem ? colors.offwhite : colors.transparent
                            }

                            Behavior on color {
                                ColorAnimation { duration: 300 }
                            }

                            Components.Icon {
                                size: parent.width
                                icon: modelData
                                color: colors.offwhite
                            }

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onPressed: customIconContainer.color = colors.offwhite
                                onReleased: customIconContainer.color = colors.transparent
                                onCanceled: customIconContainer.color = colors.transparent
                                onClicked: {
                                    customIconContainer.color = colors.offwhite
                                    iconSelectorPopup.iconSelected(modelData);
                                    iconSelectorPopup.close();
                                }
                            }
                        }
                    }
                }
            }
        }

        Components.Button {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 20

            text: qsTr("Close")
            trigger: function() {
                iconSelectorPopup.close();
            }
        }
    }
}
