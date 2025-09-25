// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 BOTTOM SHEET COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - title open
 - title close
 - open item qml component source
**/

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Item {
    id: bottomSheetContainer
    width: parent.width
    height: 100
    anchors.bottom: parent.bottom
    state: "closed"

    property string parentController
    property string titleClosed
    property string titleOpened
    property alias openItemSource: openItemLoader.source
    property alias openItem: openItemLoader.item
    property alias buttonNavigation: buttonNavigation

    signal opened
    signal closed

    onStateChanged: {
        if (state == "opened") {
            buttonNavigation.takeControl();
        } else {
            buttonNavigation.releaseControl(bottomSheetContainer.parentController);
        }
    }

    states: [
        State {
            name: "closed"
            PropertyChanges { target: bottomSheetContainerContent; y: 0; height: 100 + ui.cornerRadiusLarge }
            PropertyChanges { target: blockOutOverlay; opacity: 0 }
        },
        State {
            name: "opened"
            PropertyChanges { target: bottomSheetContainerContent; y: -bottomSheetContainerContent.height + bottomSheetContainer.height + ui.cornerRadiusLarge; height: ui.height - topNavigation.height + ui.cornerRadiusLarge }
            PropertyChanges { target: blockOutOverlay; opacity: 0.8 }
        }
    ]

    transitions: [
        Transition {
            to: "closed"
            ScriptAction { script: openItemLoader.active = false }
            ParallelAnimation {
                PropertyAnimation { target: bottomSheetContainerContent; properties: "y, height"; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { target: blockOutOverlay; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
            ScriptAction { script: bottomSheetContainer.closed() }
        },
        Transition {
            to: "opened"

            ParallelAnimation {
                PropertyAnimation { target: bottomSheetContainerContent; properties: "y, height"; easing.type: Easing.OutExpo; duration: 300 }
                SequentialAnimation {
                    PauseAnimation { duration: 100 }
                    PropertyAnimation { target: blockOutOverlay; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                }
                ScriptAction { script: openItemLoader.active = true }
            }
        }
    ]


    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "HOME": {
                "pressed": function() {
                    bottomSheetContainer.state = "closed";
                }
            },
            "BACK": {
                "pressed": function() {
                    bottomSheetContainer.state = "closed";
                }
            }
        }
    }

    Rectangle {
        id: blockOutOverlay

        width: parent.width
        height: ui.height - bottomSheetContainerContent.height + ui.cornerRadiusLarge
        color: colors.black
        anchors.bottom: bottomSheetContainerContent.top
        enabled: opacity != 0

        MouseArea {
            anchors.fill: parent
            onClicked: bottomSheetContainer.state = "closed"
        }
    }

    Rectangle {
        id: bottomSheetContainerContent
        width: parent.width
        color: Qt.darker(colors.dark, 1.5)
        radius: ui.cornerRadiusLarge

        Item {
            id: titleBar
            width: parent.width
            height: 100
            anchors.top: parent.top

            Text {
                text: bottomSheetContainer.state == "closed" ? bottomSheetContainer.titleClosed : bottomSheetContainer.titleOpened

                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 2
                color: bottomSheetContainer.state == "closed" ? colors.light : colors.offwhite
                anchors { left: parent.left; leftMargin: 20; right: footerIcon.left; rightMargin: 10; verticalCenter: parent.verticalCenter }
                font: fonts.secondaryFont(28)
            }

            Components.Icon {
                id: footerIcon
                icon: "uc:plus"
                size: 100
                color: colors.light
                anchors { verticalCenter: parent.verticalCenter; right: parent.right }

                transformOrigin: Item.Center
                rotation: bottomSheetContainer.state == "closed" ? 0 : 315

                Behavior on rotation {
                    RotationAnimation  { easing.type: Easing.OutExpo; duration: 300 }
                }
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    if (bottomSheetContainer.state == "closed") {
                        bottomSheetContainer.state = "opened";
                    } else {
                        bottomSheetContainer.state = "closed";
                    }
                }
            }
        }

        Loader {
            id: openItemLoader
            width: parent.width
            opacity: active ? 1 : 0
            asynchronous: true
            active: false
            anchors { top: titleBar.bottom; bottom: parent.bottom; bottomMargin: ui.cornerRadiusLarge }

            onStatusChanged: {
                if (status == Loader.Ready) {
                    bottomSheetContainer.opened();
                }
            }

            Behavior on opacity {
                OpacityAnimator { duration: 300 }
            }
        }
    }
}
