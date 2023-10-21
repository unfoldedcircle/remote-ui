// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: loadingFirstRoot
    parent: Overlay.overlay
    anchors.fill: parent
    color: colors.black
    z: 3999

    function stop() {
        endAnimation.start();
        ui.inputController.blockInput(false);
    }

    Connections {
        target: ui
        ignoreUnknownSignals: true

        function onConfigLoaded() {
            loadingFirstRoot.stop();
        }
    }

    Component.onCompleted: {
        startAnimation.start();
        ui.inputController.blockInput(true);
    }

    Item {
        id: animationContainer
        width: 105
        height: 70
        anchors.centerIn: parent
        opacity: 0

        Item {
            id: arc1
            x: 70
            y: 0
            transform: Rotation { id: arc1Rot; origin.x: 0; origin.y: 35; axis { x: 1; y: 0; z: 0 } angle: 0 }

            Image {
                source: "qrc:/images/quarter_arc.svg"
            }
        }

        Item {
            id: arc2
            x: 70
            y: 35
            transform: Rotation { id: arc2Rot; origin.x: 0; origin.y: 0; axis { x: 1; y: 0; z: 0 } angle: 0 }

            Image {
                source: "qrc:/images/quarter_arc.svg"
                rotation: 270
            }
        }

        Item {
            id: arc3
            x: 35
            y: 35
            transform: Rotation { id: arc3Rot; origin.x: 0; origin.y: 0; axis { x: 0; y: 1; z: 0 } angle: 0 }

            Image {
                source: "qrc:/images/quarter_arc.svg"
                rotation: 180
            }
        }

        Item {
            id: arc4
            x: 0
            y: 35
            transform: Rotation { id: arc4Rot; origin.x: 0; origin.y: 0; axis { x: 1; y: 0; z: 0 } angle: 0 }

            Image {
                source: "qrc:/images/quarter_arc.svg"
                rotation: 270
            }
        }
    }

    ParallelAnimation {
        id: startAnimation
        running: false

        SequentialAnimation {
            PauseAnimation { duration: 100 }
            NumberAnimation { targets: animationContainer; properties: "opacity"; to: 1; easing.type: Easing.OutExpo; duration: 300 }
        }
    }

    ParallelAnimation {
        id: endAnimation
        running: false

        ParallelAnimation {
            SequentialAnimation {
                NumberAnimation { target: arc2Rot; property: "angle"; from: 0; to: 180; easing.type: Easing.InExpo; duration: 700 }
                NumberAnimation { targets: arc2; properties: "opacity"; to: 0; duration: 0 }
            }
            SequentialAnimation {
                PauseAnimation { duration: 500 }
                ParallelAnimation {
                    NumberAnimation { target: arc1Rot; property: "angle"; from: 0; to: 180; easing.type: Easing.InExpo; duration: 700 }
                    NumberAnimation { targets: arc1; properties: "opacity"; to: 0; easing.type: Easing.InExpo; duration: 700 }
                }
            }
        }

        SequentialAnimation {
            PauseAnimation { duration: 200 }
            ParallelAnimation {
                SequentialAnimation {
                    NumberAnimation { target: arc3Rot; property: "angle"; from: 0; to: 180; easing.type: Easing.InExpo; duration: 700 }
                    NumberAnimation { targets: arc3; properties: "opacity"; to: 0; duration: 0 }
                }
                SequentialAnimation {
                    PauseAnimation { duration: 500 }
                    ParallelAnimation {
                        NumberAnimation { target: arc4Rot; property: "angle"; from: 0; to: 180; easing.type: Easing.InExpo; duration: 700 }
                        NumberAnimation { targets: arc4; properties: "opacity"; to: 0; easing.type: Easing.InExpo; duration: 700 }
                    }
                }
                SequentialAnimation {
                    PauseAnimation { duration: 1200 }
                    NumberAnimation { targets: loadingFirstRoot; properties: "opacity"; to: 0; easing.type: Easing.OutExpo; duration: 300 }
                }
            }
        }
    }
}
