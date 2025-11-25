// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Entity.Controller 1.0
import Haptic 1.0

import Integration.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: entityBaseDetailContainer
    color: colors.black
    width: parent.width
    height: parent.height

    signal closed()

    state: "closed"

    states: [
        State {
            name: "open"
            PropertyChanges {target: iconClose; opacity: 1 }
            PropertyChanges {target: entityBaseDetailContainer; y: 0 }
        },
        State {
            name: "closed"
            PropertyChanges {target: iconClose; opacity: 0 }
            PropertyChanges {target: entityBaseDetailContainer; y: entityBaseDetailContainer.height }
        }
    ]

    transitions: [
        Transition {
            to: "open"
            SequentialAnimation {
                ParallelAnimation {
                    PropertyAction { target: containerMain.item; property: "state"; value: "hidden" }
                    PropertyAnimation { target: entityBaseDetailContainer; properties: "y"; easing.type: Easing.OutExpo; duration: entityBaseDetailContainer.skipAnimation ? 0 : 300 }
                }
                PropertyAnimation { target: iconClose; properties: "opacity"; easing.type: Easing.OutExpo; duration: entityBaseDetailContainer.skipAnimation ? 0 : 300 }
                PauseAnimation { duration: 500 }
            }
        },
        Transition {
            to: "closed"
            SequentialAnimation {
                PauseAnimation { duration: entityBaseDetailContainer.skipAnimation ? 300 : 0 }
                ParallelAnimation {
                    PropertyAnimation { target: entityBaseDetailContainer; properties: "y"; easing.type: Easing.InExpo; duration: 300 }
                    PropertyAnimation { target: iconClose; properties: "opacity"; easing.type: Easing.InExpo; duration: 200 }
                    PropertyAction { target: containerMain.item; property: "state"; value: entityBaseDetailContainer.skipAnimation ? "hidden" : "visible" }
                }
            }

            onRunningChanged: {
                if ((state == "closed") && (!running))
                    entityBaseDetailContainer.closed();
            }
        }
    ]

    property string entityId
    property QtObject entityObj
    property QtObject integrationObj: QtObject {
        property string state
    }

    property bool skipAnimation: false
    property var overrideConfig: ([])

    property alias iconClose: iconClose
    property alias buttonNavigation: buttonNavigation

    function open(skipAnimation = false) {
        // get the latest entity data from the core
        EntityController.refreshEntity(entityId);

        ui.inputController.takeControl(String(entityBaseDetailContainer));
        entityBaseDetailContainer.skipAnimation = skipAnimation;
        entityBaseDetailContainer.state = "open";
    }

    function close() {
        entityBaseDetailContainer.state = "closed";

        if (entityBaseDetailContainer.skipAnimation) {
            entityBaseDetailContainer.closed();
            ui.inputController.releaseControl();
        }

    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                },
                "long_press": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                },
                "long_press": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                }
            }
        }
        overrideConfig: entityBaseDetailContainer.overrideConfig
    }

    Components.Icon {
        id: iconClose
        color: colors.offwhite
        opacity: 0
        icon: "uc:xmark"
        anchors { right: parent.right; top: parent.top; topMargin: 5 }
        size: 70
        z: 1000

        Components.HapticMouseArea {
            width: parent.width + 20; height: parent.height + 20
            anchors.centerIn: parent
            enabled: entityBaseDetailContainer.state == "open"
            onClicked: {
                entityBaseDetailContainer.close();
            }
        }
    }

    Components.Icon {
        color: colors.red
        icon: "uc:link-slash"
        anchors { right: iconClose.left; verticalCenter: iconClose.verticalCenter }
        size: 40
        visible: integrationObj.state != "connected" && integrationObj.state != ""
        z: 1001
    }

    Rectangle {
        id: unavailableOverlay
        color: colors.black
        opacity: entityObj.state == 0 ? 0.85 : 0
        anchors { top: iconClose.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
        z: 2000

        onOpacityChanged: {
            if (unavailableOverlay.opacity == 0) {
                showUnavailableIconTimer.stop();
                unavailableOverlayIcon.visible = false;
            } else {
                showUnavailableIconTimer.start();
            }
        }

        Timer {
            id: showUnavailableIconTimer
            repeat: false
            running: false
            interval: 1000

            onTriggered: {
                unavailableOverlayIcon.visible = true;
            }
        }

        MouseArea {
            enabled: unavailableOverlay.opacity != 0
            anchors.fill: parent
        }

        Components.Icon {
            id: unavailableOverlayIcon
            color: colors.red
            anchors.centerIn: parent
            icon: "uc:ban"
            size: 120
            visible: false
        }

        Text {
            visible: unavailableOverlayIcon.visible
            text: qsTr("Entity unavailable")
            width: parent.width - 40
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: 2
            elide: Text.ElideRight
            color: colors.red
            font: fonts.primaryFont(30)
            lineHeight: 0.8
            anchors { horizontalCenter: parent.horizontalCenter; top: unavailableOverlayIcon.bottom; topMargin: 20 }
        }
    }
}
