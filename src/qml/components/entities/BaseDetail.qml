// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Entity.Controller 1.0
import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: entityBaseDetailContainer
    color: colors.black
    width: parent.width
    height: parent.height

    signal closed()

    Connections {
        target: entityObj

        function onStateChanged(entityId, newState) {
            if (entityObj.state === 0 && entityBaseDetailContainer.state == "open") {
                entityBaseDetailContainer.close();
            }
        }
    }

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
//                ScriptAction { script: ui.inputController.blockInput(false); }
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
    property bool skipAnimation: false
    property var overrideConfig: ([])

    property alias iconClose: iconClose
    property alias buttonNavigation: buttonNavigation

    function open(skipAnimation = false) {
        buttonNavigation.takeControl();
//        ui.inputController.blockInput(true);
        entityBaseDetailContainer.skipAnimation = skipAnimation;
        entityBaseDetailContainer.state = "open";
    }

    function close() {
        entityBaseDetailContainer.state = "closed";

        if (entityBaseDetailContainer.skipAnimation) {
            entityBaseDetailContainer.closed();
            buttonNavigation.releaseControl();
        }

//        ui.inputController.blockInput(false);
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    if (entityBaseDetailContainer.state == "open") {
                        entityBaseDetailContainer.close();
                    }
                }
            },
            "HOME": {
                "released": function() {
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
        icon: "uc:close"
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
}
