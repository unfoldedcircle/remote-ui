// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

Popup {
    id: sourceListPopup
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
    property var closeCallback: function() {}
    property var items: []

    onOpened: {
        buttonNavigation.takeControl();
        iconGrid.forceActiveFocus();
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_MIDDLE": {
                "pressed": function() {
                    iconGrid.currentItem.callBack();
                }
            },
            "BACK": {
                "pressed": function() {
                    sourceListPopup.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    sourceListPopup.close();
                }
            }
        }
    }

    background: Rectangle {
        color: colors.black
        opacity: 0.8
    }

    contentItem: Rectangle {
        color: colors.black
        radius: ui.cornerRadiusLarge

        Components.Icon {
            id: iconClose
            color: colors.offwhite
            icon: "uc:xmark"
            anchors { right: parent.right; top: parent.top; topMargin: 5 }
            size: 70

            Components.HapticMouseArea {
                width: parent.width + 20; height: parent.height + 20
                anchors.centerIn: parent
                onClicked: {
                    sourceListPopup.close();
                }
            }
        }

        Text {
            text: sourceListPopup.title
            color: colors.offwhite
            font: fonts.primaryFont(24, "Medium")
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            maximumLineCount: 2
            lineHeight: 0.8
            anchors { left: parent.left; leftMargin: 20; right: iconClose.left; rightMargin: 20; verticalCenter: iconClose.verticalCenter; }
        }

        GridView {
            id: iconGrid

            cellWidth: 230; cellHeight: 100
            clip: true
            pressDelay: 100
            keyNavigationEnabled: true
            focus: true
            model: sourceListPopup.items
            anchors { left: parent.left; leftMargin: 10; right: parent.right; rightMargin: 10; top: iconClose.bottom; topMargin: 20; bottom: parent.bottom; bottomMargin: 20 }

            delegate: Components.HapticMouseArea {
                width: GridView.view.cellWidth
                height: GridView.view.cellHeight

                property bool currentItem: GridView.isCurrentItem

                function callBack() {
                    let f = sourceListPopup.items[index].callback();
                    closeCallback = function() { f(); };
                    sourceListPopup.close();
                }

                onClicked: callBack()

                Rectangle {
                    width: parent.width - 20
                    height: parent.height - 20
                    anchors.centerIn: parent
                    color: colors.dark
                    radius: ui.cornerRadiusSmall
                    border {
                        width: 2
                        color: currentItem ? Qt.lighter(colors.dark, 2) : colors.transparent
                    }

                    Text {
                        text: sourceListPopup.items[index].title
                        color: colors.offwhite
                        font: fonts.primaryFont(26)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        padding: 10
                    }
                }
            }
        }

        Components.ScrollIndicator {
            parentObj: iconGrid
            hideOverride: iconGrid.atYEnd
        }
    }
}
