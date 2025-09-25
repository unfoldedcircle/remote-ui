// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Entity.Light 1.0

import "qrc:/components" as Components

Item {
    id: brightnessFeature
    anchors.fill: parent

    property QtObject entityObj
    property alias brightnessSlider: brightnessSlider
    property alias colorTempSlider: colorTempSlider

    Components.ButtonNavigation {
        overrideActive: true

        defaultConfig: {
            "VOLUME_UP": {
                "pressed": function() {
                    if (entityObj.hasAllFeatures([LightFeatures.Dim, LightFeatures.Color_temperature])) {
                        brightnessFeature.brightnessSlider.increase();
                        brigthnessChangeTimeOut.restart();
                    }
                }
            },
            "VOLUME_DOWN": {
                "pressed": function() {
                    if (entityObj.hasAllFeatures([LightFeatures.Dim, LightFeatures.Color_temperature])) {
                        brightnessFeature.brightnessSlider.decrease();
                        brigthnessChangeTimeOut.restart();
                    }
                }
            },
            "CHANNEL_UP": {
                "pressed": function() {
                    if (entityObj.hasAllFeatures([LightFeatures.Dim, LightFeatures.Color_temperature])) {
                        brightnessFeature.colorTempSlider.increase();
                        colorTempChangeTimeOut.restart();
                    }
                }
            },
            "CHANNEL_DOWN": {
                "pressed": function() {
                    if (entityObj.hasAllFeatures([LightFeatures.Dim, LightFeatures.Color_temperature])) {
                        brightnessFeature.colorTempSlider.decrease();
                        colorTempChangeTimeOut.restart();
                    }
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    if (entityObj.hasFeature(LightFeatures.Dim)) {
                        brightnessFeature.brightnessSlider.increase();
                        brigthnessChangeTimeOut.restart();
                    }
                }
            },
            "DPAD_DOWN": {
                "pressed": function() {
                    if (entityObj.hasFeature(LightFeatures.Dim)) {
                        brightnessFeature.brightnessSlider.decrease();
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
        onTriggered: entityObj.setBrightness(brightnessFeature.brightnessSlider.value)
    }

    Timer {
        id: colorTempChangeTimeOut
        running: false
        repeat: false
        interval: 500
        onTriggered: entityObj.setColorTemperature(brightnessFeature.colorTempSlider.value)
    }

    Item {
        width: entityObj.hasFeature(LightFeatures.Color_temperature) ? parent.width/2 : parent.width
        height: parent.height
        anchors.left: parent.left
        visible: entityObj.hasFeature(LightFeatures.Dim)

        Text {
            text: Math.round(brightnessSlider.value/255*100)
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            anchors { left: parent.left; leftMargin: 30; right: parent.right; rightMargin: 30; bottom: brightnessSlider.top }
            font: fonts.primaryFont(entityObj.hasFeature(LightFeatures.Color_temperature) ? 100 : 180, "Light")
        }

        Slider {
            id: brightnessSlider
            live: true
            width: parent.width - 60
            height: 500
            snapMode: Slider.SnapAlways
            padding: 0
            orientation: Qt.Vertical

            transformOrigin: Item.Center
            rotation: 180
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 30 }

            from: 255
            to: 0
            stepSize: 1
            value: entityObj.brightness

            background: Item {
                id: brightnessSliderBg
                implicitWidth: brightnessSlider.width; implicitHeight: brightnessSlider.height
                width: brightnessSlider.availableWidth; height: implicitHeight

                Rectangle {
                    width: parent.width; height: brightnessSlider.height
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
                        height: brightnessSlider.visualPosition * parent.height; width: parent.width
                        color: colors.offwhite
                        radius: ui.cornerRadiusSmall
                    }
                }
            }

            handle: Item {
                x: (brightnessSlider.visualPosition * brightnessSlider.availableWidth) - width/2
                y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                implicitWidth: 40; implicitHeight: 40
            }

            onValueChanged: Haptic.play(Haptic.Bump)

            onPressedChanged: {
                if (!brightnessSlider.pressed) {
                    entityObj.setBrightness(value);
                }
            }
        }

        Components.Icon {
            color: Qt.darker(colors.light)
            icon: "uc:plus"
            size: 60
            anchors { right: brightnessSlider.right; rightMargin: 10; top: brightnessSlider.top; topMargin: 20 }
            visible: entityObj.hasFeature(LightFeatures.Color_temperature)
        }

        Components.Icon {
            color: Qt.darker(colors.light)
            icon: "uc:minus"
            size: 60
            anchors { right: brightnessSlider.right; rightMargin: 10; bottom: brightnessSlider.bottom; bottomMargin: 20 }
            visible: entityObj.hasFeature(LightFeatures.Color_temperature)
        }

        Components.Icon {
            color: colors.light
            icon: "uc:brightness"
            anchors { left: brightnessSlider.left; leftMargin: 10; top: brightnessSlider.top; topMargin: 10 }
            size: 80
        }
    }

    Item {
        width: entityObj.hasFeature(LightFeatures.Dim) ? parent.width/2 : parent.width
        height: parent.height
        anchors.right: parent.right
        visible: entityObj.hasFeature(LightFeatures.Color_temperature)

        Text {
            text: colorTempSlider.value
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            anchors { left: parent.left; leftMargin: 30; right: parent.right; rightMargin: 30; bottom: colorTempSlider.top }
            font: fonts.primaryFont(entityObj.hasFeature(LightFeatures.Dim) ? 100 : 180, "Light")
        }

        Slider {
            id: colorTempSlider
            live: true
            width: parent.width - 60
            height: 500
            snapMode: Slider.SnapAlways
            padding: 0
            orientation: Qt.Vertical

            transformOrigin: Item.Center
            rotation: 180
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 30 }

            from: entityObj.colorTempSteps
            to: 0
            stepSize: 1
            value: entityObj.colorTemp

            background: Rectangle {
                id: colorTempSliderBg
                implicitWidth: colorTempSlider.width; implicitHeight: colorTempSlider.height
                width: colorTempSlider.availableWidth; height: implicitHeight
                radius: ui.cornerRadiusSmall

                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#C9ECFF" }
                    GradientStop { position: 1.0; color: "#FFE48B" }
                    orientation: Gradient.Vertical
                }
            }

            handle: Rectangle {
                y: (colorTempSlider.visualPosition * colorTempSlider.availableHeight) - height / 2
                width: colorTempSlider.width; height: 8
                color: colors.offwhite
                radius: 4
            }

            onValueChanged: Haptic.play(Haptic.Bump)

            onPressedChanged: {
                if (!colorTempSlider.pressed) {
                    entityObj.setColorTemperature(value);
                }
            }
        }

        Components.Icon {
            color: Qt.lighter(colors.light)
            icon: "uc:arrow-up"
            size: 60
            anchors { right: colorTempSlider.right; rightMargin: 10; top: colorTempSlider.top; topMargin: 20 }
            visible: entityObj.hasFeature(LightFeatures.Color_temperature)
        }

        Components.Icon {
            color: Qt.lighter(colors.light)
            icon: "uc:arrow-down"
            size: 60
            anchors { right: colorTempSlider.right; rightMargin: 10; bottom: colorTempSlider.bottom; bottomMargin: 20 }
            visible: entityObj.hasFeature(LightFeatures.Color_temperature)
        }

        Components.Icon {
            id: colorTempIcon
            color: colors.light
            icon: "uc:temperature-half"
            anchors { left: colorTempSlider.left; leftMargin: 10; top: colorTempSlider.top; topMargin: 10 }
            size: 80
        }
    }
}
