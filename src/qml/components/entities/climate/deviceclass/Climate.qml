// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import Entity.Climate 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: climateBase

    function modeOpen() {
        if (entityObj.hasAnyFeature([ClimateFeatures.On_off, ClimateFeatures.Heat, ClimateFeatures.Cool])) {
            //: Climate device mode
            popupMenu.title = qsTr("Mode")
            let menuItems = [];

            if (entityObj.hasFeature(ClimateFeatures.On_off) && entityObj.state !== ClimateStates.Off) {
                menuItems.push({
                                   //: Climate device state
                                   title: qsTr("Off"),
                                   icon: "uc:power-off",
                                   callback: function() {
                                       entityObj.turnOff();
                                   }
                               });
            }

            if (entityObj.hasFeature(ClimateFeatures.Heat) && entityObj.state !== ClimateStates.Heat) {
                menuItems.push({
                                   //: Climate device state
                                   title: qsTr("Heat"),
                                   icon: "uc:heat",
                                   callback: function() {
                                       entityObj.setHvacMode(ClimateStates.Heat);
                                   }
                               });
            }

            if (entityObj.hasFeature(ClimateFeatures.Cool) && entityObj.state !== ClimateStates.Cool) {
                menuItems.push({
                                   //: Climate device state
                                   title: qsTr("Cool"),
                                   icon: "uc:snowflake",
                                   callback: function() {
                                       entityObj.setHvacMode(ClimateStates.Cool);
                                   }
                               });
            }
            if (entityObj.hasAllFeatures([ClimateFeatures.Cool, ClimateFeatures.Heat])) {
                menuItems.push({
                                   //: Climate device state
                                   title: qsTr("Auto"),
                                   icon: "uc:temperature-half",
                                   callback: function() {
                                       entityObj.setHvacMode(ClimateStates.Auto);
                                   }
                               });
            }

            popupMenu.menuItems = menuItems;
            popupMenu.open();
        }
    }

    function fanOpen() {
        if (entityObj.hasFeature(ClimateFeatures.Fan)) {
            popupMenu.title = qsTr("Fan")
            let menuItems = [];

            popupMenu.menuItems = menuItems;
            popupMenu.open();
        }
    }

    Connections {
        target: entityObj
        ignoreUnknownSignals: true

        function onTargetTemperatureChanged() {
            let index = entityObj.getModelIndexFromTemperature(entityObj.targetTemperature);
            temperatureTumbler.positionViewAtIndex(index - 1, ListView.Center);
            temperatureTumbler.currentIndex = index;
        }
    }

    Timer {
        running: true
        repeat: false
        interval: 200

        onTriggered: {
            let index = entityObj.getModelIndexFromTemperature(entityObj.targetTemperature);
            temperatureTumbler.currentIndex = index;
            temperatureTumbler.positionViewAtIndex(index - 1, ListView.Center);
            temperatureTumbler.opacity = 1;
        }
    }


    overrideConfig: {
        "DPAD_UP": {
            "pressed": function() {
                temperatureTumbler.currentIndex--;
                temperatureTumbler.positionViewAtIndex(temperatureTumbler.currentIndex, ListView.Center);
                targetTempChangeTimeOut.restart();
            }
        },
        "DPAD_DOWN": {
            "pressed": function() {
                temperatureTumbler.currentIndex++;
                temperatureTumbler.positionViewAtIndex(temperatureTumbler.currentIndex, ListView.Center);
                targetTempChangeTimeOut.restart();
            }
        },
        "GREEN": {
            "pressed": function() {
                modeOpen();
            }
        },
        "YELLOW": {
            "pressed": function() {
                fanOpen();
            }
        }
    }

    Timer {
        id: targetTempChangeTimeOut
        running: false
        repeat: false
        interval: 500

        onTriggered: {
            entityObj.setTargetTemperature(entityObj.model[temperatureTumbler.currentIndex]);
        }
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    ColumnLayout {
        id: climateFeatures
        width: parent.width
        height: parent.height - title.height
        anchors { top: title.bottom }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Tumbler {
                id: temperatureTumbler
                anchors.fill: parent
                model: entityObj.model
                visibleItemCount: 3
                wrap: false
                delegate: tumblerComponent
                opacity: 0

                Behavior on opacity {
                    OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
                }

                onMovingChanged: {
                    if (!moving) {
                        entityObj.setTargetTemperature(entityObj.model[temperatureTumbler.currentIndex]);
                    }
                }

                onCurrentIndexChanged: {
                    Haptic.play(Haptic.Bump);
                }
            }

            Item {
                id: topGradient
                width: parent.width; height: 200
                anchors { top: temperatureTumbler.top; horizontalCenter: temperatureTumbler.horizontalCenter }

                LinearGradient {
                    anchors.fill: parent
                    start: Qt.point(0, 0)
                    end: Qt.point(0, parent.height)
                    gradient: Gradient {
                        GradientStop { position: 1.0; color: "#00000000" }
                        GradientStop { position: 0.0; color: colors.black }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: colors.black
                    opacity: temperatureTumbler.moving ? 0 : 1

                    Behavior on opacity {
                        OpacityAnimator { duration: 100 }
                    }

                    Item {
                        anchors.centerIn: parent

                        Rectangle {
                            width: 100
                            height: 4
                            color: colors.offwhite
                            anchors.centerIn: parent
                        }

                        Rectangle {
                            width: 4
                            height: 100
                            color: colors.offwhite
                            anchors.centerIn: parent
                        }
                    }

                    Components.HapticMouseArea  {
                        anchors.fill: parent
                        enabled: !temperatureTumbler.moving

                        onClicked: {
                            temperatureTumbler.currentIndex--;
                            temperatureTumbler.positionViewAtIndex(temperatureTumbler.currentIndex, ListView.Center);
                            targetTempChangeTimeOut.restart();
                        }
                    }
                }
            }

            Item {
                id: bottomGradient
                width: parent.width; height: 200
                anchors { bottom: temperatureTumbler.bottom; horizontalCenter: temperatureTumbler.horizontalCenter }

                LinearGradient {
                    anchors.fill: parent
                    start: Qt.point(0, 0)
                    end: Qt.point(0, parent.height)
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#00000000" }
                        GradientStop { position: 1.0; color: colors.black }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: colors.black
                    opacity: temperatureTumbler.moving ? 0 : 1

                    Behavior on opacity {
                        OpacityAnimator { duration: 100 }
                    }

                    Item {
                        anchors.centerIn: parent

                        Rectangle {
                            width: 100
                            height: 4
                            color: colors.offwhite
                            anchors.centerIn: parent
                        }

                        Item {
                            height: 100
                            anchors.centerIn: parent
                        }
                    }

                    Components.HapticMouseArea  {
                        anchors.fill: parent
                        enabled: !temperatureTumbler.moving

                        onClicked: {
                            temperatureTumbler.currentIndex++;
                            temperatureTumbler.positionViewAtIndex(temperatureTumbler.currentIndex, ListView.Center);
                            targetTempChangeTimeOut.restart();
                        }
                    }
                }
            }


            Text {
                visible: entityObj.hasFeature(ClimateFeatures.Current_temperature)
                //: Current temperature
                text: qsTr("Current %1°").arg(entityObj.currentTemperature.toLocaleString(Qt.locale(), 'f', entityObj.targetTemperatureStep === 1 ? 0 : 1))
                color: colors.light
                opacity: temperatureTumbler.moving ? 0 : 1
                font: fonts.secondaryFont(26)
                anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; verticalCenterOffset: 140 }

                Behavior on opacity {
                    OpacityAnimator { duration: 100 }
                }
            }
        }

        Item {
            id: bottomMenu
            visible: entityObj.hasAnyFeature([ClimateFeatures.On_off, ClimateFeatures.Heat, ClimateFeatures.Cool])

            Layout.fillWidth: true
            height: 80

            Components.HapticMouseArea {
                width: entityObj.hasAnyFeature([ClimateFeatures.On_off, ClimateFeatures.Heat, ClimateFeatures.Cool]) ? (entityObj.hasFeature(ClimateFeatures.Fan) ? parent.width / 2 : parent.width) : 0
                height: parent.height
                anchors.left: parent.left
                visible: width > 0
                enabled: visible

                Text {
                    text: {
                        switch (entityObj.state) {
                        case ClimateStates.Off:
                            //: Climate device state
                            return qsTr("Off");
                        case ClimateStates.Heat:
                            //: Climate device state
                            return qsTr("Heat");
                        case ClimateStates.Cool:
                            //: Climate device state
                            return qsTr("Cool");
                        case ClimateStates.Heat_cool:
                            //: Climate device state
                            return qsTr("Heat/Cool");
                        case ClimateStates.Fan:
                            //: Climate device state
                            return qsTr("Fan");
                        case ClimateStates.Auto:
                            //: Climate device state
                            return qsTr("Auto");
                        default:
                            //: Climate device state
                            return qsTr("Mode");
                        }
                    }

                    verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                    color: colors.offwhite
                    font: fonts.secondaryFont(30,  "Bold")
                    anchors.centerIn: parent
                }

                Rectangle {
                    width: 12; height: 12
                    radius: 6
                    color: colors.remoteGreen
                    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
                }

                onClicked: {
                    modeOpen();
                }
            }

            Components.HapticMouseArea {
                width: entityObj.hasFeature(ClimateFeatures.Fan) ? (entityObj.hasAnyFeature([ClimateFeatures.On_off, ClimateFeatures.Heat, ClimateFeatures.Cool]) ? parent.width / 2 : parent.width) : 0
                height: parent.height
                anchors.right: parent.right
                visible: width > 0
                enabled: visible

                Text {
                    //: Climate fan
                    text: qsTr("Fan")
                    verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                    color: colors.offwhite
                    font: fonts.secondaryFont(30,  "Bold")
                    anchors.centerIn: parent
                }

                Rectangle {
                    width: 12; height: 12
                    radius: 6
                    color: colors.remoteYellow
                    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
                }

                onClicked: {
                    fanOpen();
                }
            }
        }
    }

    Components.PopupMenu {
        id: popupMenu
    }

    Component {
        id: tumblerComponent

        Item {
            width: temperatureTumbler.width
            height: 200

            property bool isCurrentItem: Tumbler.tumbler.currentIndex === index
            property alias selectedTemperature: selectedTemperature

            Text {
                id: selectedTemperature
                text: entityObj.model[index].toLocaleString(Qt.locale(), 'f', entityObj.targetTemperatureStep === 1 ? 0 : 1)
                color: entityObj.currentTemperature > entityObj.targetTemperature && isCurrentItem ? colors.blue : ( (entityObj.currentTemperature === entityObj.targetTemperature && isCurrentItem) || !isCurrentItem ? colors.offwhite : colors.red )
                opacity: isCurrentItem ? 1 : 0.5
                horizontalAlignment: Text.AlignHCenter
                font: isCurrentItem ? fonts.primaryFont(160, "Light") : fonts.primaryFont(120, "ExtraLight")
                anchors.centerIn: parent
            }

            Text {
                visible: isCurrentItem
                text: entityObj.temperatureLabel
                color: selectedTemperature.color
                opacity: selectedTemperature.opacity
                font: fonts.primaryFont(40, "SemiBold")
                anchors { left: selectedTemperature.right; leftMargin: 10; top: selectedTemperature.top; topMargin: 42 }
            }
        }
    }
}
