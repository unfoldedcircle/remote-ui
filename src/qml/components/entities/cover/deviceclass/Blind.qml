// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import Entity.Cover 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents
import "qrc:/components/entities/cover" as CoverComponents

EntityComponents.BaseDetail {
    id: coverBase

    overrideConfig: {
        "DPAD_UP": {
            "pressed": function() {
                repeatStarterTimer.action = function () {
                    if (entityObj.hasFeature(CoverFeatures.Position)) {
                        repeatPressTimer.up = true;
                        repeatPressTimer.start();
                    }
                }

                repeatStarterTimer.start()
            },
            "released": function() {
                if (repeatStarterTimer.running) {
                    repeatStarterTimer.stop();
                }

                if (repeatPressTimer.running) {
                    repeatPressTimer.stop();
                    positionChangeTimeOut.restart();
                } else {
                    entityObj.open();
                }
            }
        },
        "DPAD_DOWN": {
            "pressed": function() {
                repeatStarterTimer.action = function () {
                    if (entityObj.hasFeature(CoverFeatures.Position)) {
                        repeatPressTimer.up = false;
                        repeatPressTimer.start();
                    }
                }

                repeatStarterTimer.start()
            },
            "released": function() {
                if (repeatStarterTimer.running) {
                    repeatStarterTimer.stop();
                }

                if (repeatPressTimer.running) {
                    repeatPressTimer.stop();
                    positionChangeTimeOut.restart();
                } else {
                    entityObj.close();
                }
            }
        },
        "DPAD_MIDDLE": {
            "released": function() {
                if (entityObj.hasFeature(CoverFeatures.Stop)) {
                    entityObj.stop();
                }
            }
        }
    }

    Timer {
        id: repeatStarterTimer
        interval: 300
        repeat: false
        running: false

        property var action: function() {}

        onTriggered: {
            repeatStarterTimer.action();
        }
    }

    Timer {
        id: repeatPressTimer
        interval: changeInterval
        repeat: true
        running: false
        triggeredOnStart: true

        property int count: 0
        property int changeInterval: 150
        property bool up: false

        onRunningChanged: {
            if (!running) {
                count = 0;
                changeInterval = 150;
            }
        }

        onTriggered: {
            count++;

            if (count >= 5) {
                changeInterval = 40;
            }

            if (up) {
                positionSlider.increase();
            } else {
                positionSlider.decrease();
            }
        }
    }

    Timer {
        id: positionChangeTimeOut
        running: false
        repeat: false
        interval: 500

        onTriggered: {
            entityObj.setPosition(positionSlider.value);
        }
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        suffix: entityObj.stateAsString
        title: entityObj.name
    }

    ColumnLayout {
        id: coverFeatures
        width: parent.width
        height: parent.height - title.height
        anchors { top: title.bottom }

        CoverComponents.OpenClose {
            id: openCloseFeature

            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: parent.width - 60
            Layout.fillHeight: true

            visible: !entityObj.hasFeature(CoverFeatures.Position)
        }

        Item {
            id: positionFeatureTitle

            Layout.fillWidth: true
            Layout.leftMargin: 30
            Layout.rightMargin: 30
            implicitHeight: childrenRect.height
            visible: positionFeature.visible

            Text {
                id: positionText
                text: positionSlider.value
                elide: Text.ElideRight
                maximumLineCount: 1
                color: colors.offwhite
                font: fonts.primaryFont(180,  "Light")
            }

            Text {
                text: "%"
                color: colors.offwhite
                font: fonts.primaryFont(90)
                anchors { left: positionText.right; baseline: positionText.baseline }
            }
        }

        Item {
            id: positionFeature

            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: parent.width - 60
            Layout.fillWidth: true
            Layout.fillHeight: true

            visible: entityObj.hasFeature(CoverFeatures.Position)

            Slider {
                id: positionSlider
                visible: parent.visible
                enabled: visible
                width: parent.height + 30
                height: parent.width
                rotation: 90
                transformOrigin: Item.Center
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

                snapMode: Slider.SnapAlways
                from: 100
                to: 0
                stepSize: 1
                touchDragThreshold: 1

                value: entityObj.position

                background: Rectangle {
                    id: sliderBG
                    x: positionSlider.leftPadding
                    y: positionSlider.topPadding + positionSlider.availableHeight / 2 - height / 2
                    implicitWidth: positionSlider.width; implicitHeight: positionSlider.height
                    width: positionSlider.availableWidth; height: implicitHeight
                    radius: ui.cornerRadiusSmall
                    color: colors.dark

                    Behavior on height {
                        NumberAnimation {
                            duration: 200
                            easing.type: Easing.OutExpo
                        }
                    }

                    Flow {
                        id: blindBars
                        anchors.fill: parent
                        spacing: 10

                        layer.enabled: true
                        layer.effect: OpacityMask {
                            maskSource: sliderBG
                        }

                        Repeater {
                            id: blindBarsRepeater
                            model: Math.round((positionSlider.width / 20) * positionSlider.visualPosition)

                            Rectangle {
                                width: 10
                                height: positionSlider.height
                                color: Qt.darker(colors.light)
                            }
                        }
                    }
                }

                handle: Item {
                    x: (positionSlider.visualPosition * positionSlider.availableWidth) - width/2
                    y: positionSlider.topPadding + positionSlider.availableHeight / 2 - height / 2
                    implicitWidth: 80; implicitHeight: 80
                }

                onValueChanged: {
                    Haptic.play(Haptic.Bump);
                }

                onPressedChanged: {
                    if (!pressed) {
                        entityObj.setPosition(positionSlider.value);
                    }
                }
            }

            Rectangle {
                width: 10
                height: 20
                color: Qt.darker(colors.light)
                y: blindBars.x + (blindBarsRepeater.model * 20) - 15 + (blindBarsRepeater.model === 0 ? 2 : -10)
                anchors.horizontalCenter: parent.horizontalCenter
                visible: positionSlider.value !== 0 && positionSlider.visible

                Rectangle {
                    width: 30; height: 30
                    radius: 15
                    color: colors.transparent
                    border { width: 10; color: Qt.darker(colors.light) }
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.bottom; topMargin: -10 }
                }
            }
        }

        RowLayout {
            id: bottomMenu

            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.preferredHeight: 80
            visible: entityObj.hasFeature(CoverFeatures.Stop)

            Components.HapticMouseArea {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter

                visible: parent.visible
                enabled: visible

                Text {
                    //: Button caption to stop window blinds motion
                    text: qsTr("Stop")
                    verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                    color: colors.offwhite
                    font: fonts.secondaryFont(30,  "Bold")
                    anchors.centerIn: parent
                }

                onClicked: {
                    entityObj.stop();
                }
            }
        }
    }

    Components.PopupMenu {
        id: popupMenu
    }
}
