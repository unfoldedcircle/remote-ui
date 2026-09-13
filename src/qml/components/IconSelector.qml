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

    // The grids are driven through the button navigation only (no keyboard focus): LEFT / RIGHT
    // and UP / DOWN walk the cells, UP from the first row reaches the tabs (LEFT / RIGHT switch
    // them), DOWN past the last row reaches Close.
    enum Zone { Tabs, Grid, Close }
    property int zone: IconSelector.Zone.Grid

    function grid() {
        return iconGridSwipeView.currentIndex === 0 ? iconGrid : iconGridCustom;
    }

    function columns() {
        return Math.max(1, Math.floor(grid().width / grid().cellWidth));
    }

    function moveVertical(delta) {
        const view = iconSelectorPopup.grid();

        switch (iconSelectorPopup.zone) {
        case IconSelector.Zone.Tabs:
            if (delta > 0) {
                iconSelectorPopup.zone = view.count > 0 ? IconSelector.Zone.Grid : IconSelector.Zone.Close;
            }
            break;
        case IconSelector.Zone.Close:
            if (delta < 0) {
                iconSelectorPopup.zone = view.count > 0 ? IconSelector.Zone.Grid : IconSelector.Zone.Tabs;
            }
            break;
        default:
            const next = view.currentIndex + delta * iconSelectorPopup.columns();
            if (next < 0) {
                iconSelectorPopup.zone = IconSelector.Zone.Tabs;
            } else if (next >= view.count) {
                iconSelectorPopup.zone = IconSelector.Zone.Close;
            } else {
                view.currentIndex = next;
            }
        }
    }

    function moveHorizontal(delta) {
        const view = iconSelectorPopup.grid();

        switch (iconSelectorPopup.zone) {
        case IconSelector.Zone.Tabs:
            tabBar.currentIndex = delta > 0 ? 1 : 0;
            iconSelectorPopup.grid().currentIndex = 0;
            break;
        case IconSelector.Zone.Close:
            break;
        default:
            const next = view.currentIndex + delta;
            if (next >= 0 && next < view.count) {
                view.currentIndex = next;
            }
        }
    }

    onOpened: {
        // PopupMenu hands the input back before it runs the callback that opens this popup, so
        // it can be taken right away
        buttonNavigation.takeControl();
        iconGrid.currentIndex = 0;
        iconGridCustom.currentIndex = 0;
        tabBar.currentIndex = 0;
        iconSelectorPopup.zone = IconSelector.Zone.Grid;
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    signal iconSelected(string icon)

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "HOME": {
                "pressed": function() {
                    iconSelectorPopup.close();
                }
            },
            "BACK": {
                "pressed": function() {
                    iconSelectorPopup.close();
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (iconSelectorPopup.zone === IconSelector.Zone.Close) {
                        iconSelectorPopup.close();
                        return;
                    }

                    if (iconSelectorPopup.zone !== IconSelector.Zone.Grid) {
                        return;
                    }

                    const item = iconSelectorPopup.grid().currentItem;
                    if (!item) {
                        return;
                    }

                    iconSelectorPopup.iconSelected(item.icon);
                    iconSelectorPopup.close();
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    iconSelectorPopup.moveVertical(-1);
                }
            },
            "DPAD_DOWN": {
                "pressed": function() {
                    iconSelectorPopup.moveVertical(1);
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    iconSelectorPopup.moveHorizontal(-1);
                }
            },
            "DPAD_RIGHT": {
                "pressed": function() {
                    iconSelectorPopup.moveHorizontal(1);
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
                    //: "Unfolded" is the brand name (Unfolded Circle) — do not translate it.
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
                        border {
                            width: 2
                            color: tabBar.currentIndex == 0 && iconSelectorPopup.zone === IconSelector.Zone.Tabs
                                   && ui.keyNavigationActive ? colors.highlight : colors.transparent
                        }
                    }
                }

                TabButton {
                    id: customTabButton
                    //: Icons the user has uploaded themselves.
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
                        border {
                            width: 2
                            color: tabBar.currentIndex == 1 && iconSelectorPopup.zone === IconSelector.Zone.Tabs
                                   && ui.keyNavigationActive ? colors.highlight : colors.transparent
                        }
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


                GridView {
                    id: iconGrid

                    cellWidth: 120; cellHeight: 120

                    model: resource.getIconList()
                    clip: true
                    pressDelay: 100
                    keyNavigationEnabled: false

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
                                color: currentItem && iconSelectorPopup.zone === IconSelector.Zone.Grid && ui.keyNavigationActive ? colors.offwhite : colors.transparent
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
                    keyNavigationEnabled: false

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
                                color: currentItem && iconSelectorPopup.zone === IconSelector.Zone.Grid && ui.keyNavigationActive ? colors.offwhite : colors.transparent
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
            highlight: iconSelectorPopup.zone === IconSelector.Zone.Close && ui.keyNavigationActive
            trigger: function() {
                iconSelectorPopup.close();
            }
        }
    }
}
