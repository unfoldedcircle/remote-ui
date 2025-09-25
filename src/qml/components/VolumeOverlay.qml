// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 VOLUME OVERLAY COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - volumePosition
 - volumeUp
 - changeInterval
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Entity.Controller 1.0
import Entity.Activity 1.0
import Entity.MediaPlayer 1.0

import "qrc:/components" as Components

Popup {
    id: volume
    width: parent.width; height: parent.height
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    property QtObject entity: QtObject {
        property int volume: 0
    }

    property int volumePosition: entity.volume
    property bool supportsVolumeSet: true
    property bool up: true

    signal done

    function start(entity, up = true) {
        volume.up = up;
        volume.entity = entity;
        volume.supportsVolumeSet = true;

        if (!entity.hasFeature(MediaPlayerFeatures.Volume) && entity.type === EntityTypes.Media_player) {
            volume.supportsVolumeSet = false;
        }

        if (entity.type === EntityTypes.Activity) {
            volume.supportsVolumeSet = false;
        }

        if (!visible) {
            visible = true;
            volume.open();
        }

        hideTimer.restart();
    }

    onVolumePositionChanged: {
        hideTimer.restart();
    }

    onOpened: buttonNavigation.overrideActive = true
    onClosed: buttonNavigation.overrideActive = false

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    volume.close();
                    volume.done();
                }
            },
            "HOME": {
                "pressed": function() {
                    volume.close();
                    volume.done();
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    background: Item {}

    Rectangle {
        id: bg
        width: parent.width
        height: volumeBar.height + 40
        color: colors.black
        anchors.bottom: parent.bottom
    }

    Item {
        id: gradient
        width: parent.width; height: parent.height - bg.height
        anchors { bottom: bg.top; horizontalCenter: parent.horizontalCenter }

        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, parent.height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: colors.transparent }
                GradientStop { position: 1.0; color: colors.black }
            }
        }
    }

    Components.Icon {
        id: volumeIcon
        icon: volume.volumePosition == 0 && volume.supportsVolumeSet ? "uc:volume-xmark" : "uc:volume"
        color: colors.offwhite
        anchors { left: parent.left; leftMargin: 40; verticalCenter: volumeBar.verticalCenter; }
        size: 60
    }

    Rectangle {
        id: volumeBar
        width: 6
        height: 470
        anchors { bottom: parent.bottom; bottomMargin: 20; left: volumeIcon.right; leftMargin: 30 }
        color: colors.dark
        radius: ui.cornerRadiusSmall
        visible: volume.supportsVolumeSet

        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource:
                Rectangle {
                width: volumeBar.width
                height: volumeBar.height
                radius: volumeBar.radius
            }
        }

        Rectangle {
            width: parent.width
            height: parent.height * volume.volumePosition / 100
            color: colors.offwhite
            anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }

            Behavior on height {
                NumberAnimation { duration: 200; easing.type: Easing.OutExpo }
            }
        }
    }

    Text {
        color: colors.offwhite
        text: volume.volumePosition
        anchors { left: volumeBar.right; leftMargin: 60; verticalCenter: volumeBar.verticalCenter }
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
        font: fonts.primaryFont(180, "Light")
        visible: volume.supportsVolumeSet
    }

    Text {
        color: colors.offwhite
        text: resource.getIcon(volume.up ? "uc:plus" : "uc:minus")
        anchors { left: volumeBar.right; leftMargin: 60; verticalCenter: volumeBar.verticalCenter }
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
        font { family: "Font Awesome 6 Pro"; pixelSize: 180 }
        visible: !volume.supportsVolumeSet
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            volume.close();
            volume.done();
        }
    }

    Timer {
        id: hideTimer
        running: false
        interval: 2000
        repeat: false

        onTriggered: {
            volume.close();
            volume.done();
        }
    }
}
