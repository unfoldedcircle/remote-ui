// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Entity.Light 1.0

import "qrc:/components" as Components

Item {
    id: colorFeature
    anchors.fill: parent

    property QtObject entityObj
    property alias brightnessSliderColor: brightnessSliderColor

    Components.ButtonNavigation {
        overrideActive: true

        defaultConfig: {
            "DPAD_UP": {
                "pressed": function() {
                    if (entityObj.hasFeature(LightFeatures.Dim)) {
                        colorFeature.brightnessSliderColor.increase();
                        brigthnessChangeTimeOut.restart();
                    }
                }
            },
            "DPAD_DOWN": {
                "pressed": function() {
                    if (entityObj.hasFeature(LightFeatures.Dim)) {
                        colorFeature.brightnessSliderColor.decrease();
                        brigthnessChangeTimeOut.restart();
                    }
                }
            }
        }
    }

    Timer {
        id: brigthnessChangeTimeOut
        running: false
        repeat: false
        interval: 500
        onTriggered: entityObj.setBrightness(colorFeature.brightnessSliderColor.value)
    }

    Item {
        width: 440; height: 440
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 20 }

        ColorWheel {
            id: colorWheel
            width: 440; height: 440

            property bool userChanged: false

            Item {
                width: 440; height: 440
                x: - (picker.width / 2)
                y: - (picker.width / 2)

                Rectangle {
                    id: picker
                    width: 60; height: width
                    color: colorWheel.pickedColor
                    border { color: colors.offwhite; width: 4 }
                    radius: width / 2
                    x: colorWheel.width / 2
                    y: colorWheel.height / 2

                    MouseArea {
                        id: dragMouseArea
                        anchors.fill: parent

                        drag.target: parent
                        drag.minimumX : Math.ceil(220 - Math.sqrt(440 * parent.y - Math.pow(parent.y, 2)))
                        drag.maximumX : Math.floor(Math.sqrt(440 * parent.y - Math.pow(parent.y, 2)) + 220)

                        drag.minimumY : Math.ceil(220 - Math.sqrt(440 * parent.x - Math.pow(parent.x, 2)))
                        drag.maximumY : Math.floor(Math.sqrt(440 * parent.x - Math.pow(parent.x, 2)) + 220)
                        onPositionChanged: {
                            colorWheel.getColor(picker.x, picker.y);
                        }


                        onClicked: {
                            picker.x = mouseX - picker.width / 2;
                            picker.y = mouseY - picker.height / 2;
                            colorWheel.getColor(picker.x, picker.y);
                        }

                        onReleased: {
                            colorWheel.userChanged = true;
                            entityObj.setColor(picker.color);
                            resetUserChange.start();
                        }
                    }
                }
            }
        }

        Timer {
            id: resetUserChange
            running: false
            repeat: false
            interval: 300
            onTriggered: colorWheel.userChanged = false
        }

        Timer {
            running: true
            repeat: false
            interval: 200

            onTriggered: {
                let pos = colorWheel.getPosition(entityObj.color);
                picker.x = pos.x;
                picker.y = pos.y;
                colorWheel.getColor(picker.x, picker.y);
            }
        }

        Connections {
            target: entityObj
            ignoreUnknownSignals: true

            function onColorChanged() {
                if (!colorWheel.userChanged) {
                    let pos = colorWheel.getPosition(entityObj.color);
                    picker.x = pos.x;
                    picker.y = pos.y;
                    colorWheel.getColor(picker.x, picker.y);
                }
            }
        }
    }

    Slider {
        id: brightnessSliderColor
        live: true
        width: parent.width - 60
        height: 140
        snapMode: Slider.SnapAlways
        padding: 0

        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 30 }

        from: 0
        to: 255
        stepSize: 1
        value: entityObj.brightness

        background: Item {
            id: brightnessSliderColorBg
            implicitWidth: brightnessSliderColor.width; implicitHeight: brightnessSliderColor.height
            width: brightnessSliderColor.availableWidth; height: implicitHeight

            Rectangle {
                width: parent.width; height: brightnessSliderColor.height
                anchors.centerIn: parent
                radius: ui.cornerRadiusSmall
                color: colors.dark


                Behavior on height {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutExpo
                    }
                }

                Rectangle {
                    width: brightnessSliderColor.visualPosition * parent.width; height: parent.height
                    color: colors.offwhite
                    radius: ui.cornerRadiusSmall
                }
            }
        }

        handle: Item {
            x: (brightnessSliderColor.visualPosition * brightnessSliderColor.availableWidth) - width/2
            y: brightnessSliderColor.topPadding + brightnessSliderColor.availableHeight / 2 - height / 2
            implicitWidth: 40; implicitHeight: 40
        }

        onValueChanged: Haptic.play(Haptic.Bump)

        onPressedChanged: {
            if (!brightnessSliderColor.pressed) {
                entityObj.setBrightness(value);
            }
        }
    }

    Components.Icon {
        color: colors.light
        icon: "uc:brightness"
        anchors { right: brightnessSliderColor.right; rightMargin: 10; top: brightnessSliderColor.top; topMargin: 10 }
        size: 80
    }

    Text {
        text: Math.round(brightnessSliderColor.value/255*100)
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        elide: Text.ElideRight
        maximumLineCount: 2
        color: colors.offwhite
        anchors { left: parent.left; leftMargin: 30; right: parent.right; rightMargin: 30; bottom: brightnessSliderColor.top }
        font: fonts.primaryFont(100, "Light")
    }
}
