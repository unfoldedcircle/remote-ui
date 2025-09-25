// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 MEDIA COMPONENT
**/

import QtQuick 2.15
import QtQuick.Controls 2.15

import Entity.Controller 1.0
import Entity.MediaPlayer 1.0
import Haptic 1.0

import "qrc:/components" as Components
import "qrc:/components/entities/media_player" as MediaPlayerComponents

Rectangle {
    id: mediaComponent
    width: 80; height: 80
    color: colors.transparent
    radius: ui.cornerRadiusSmall
    clip: true

    function formatTime(time) {
        var hrs = ~~(time / 3600);
        var mins = ~~((time % 3600) / 60);
        var secs = ~~time % 60;

        // Output like "1:01" or "4:03:59" or "123:03:59"
        var ret = "";
        if (hrs > 0) {
            ret += "" + hrs + ":" + (mins < 10 ? "0" : "");
        }
        ret += "" + mins + ":" + (secs < 10 ? "0" : "");
        ret += "" + secs;
        return ret;
    }

    readonly property int requiredTextArea: 120
    property int gridWidth: 4
    property int gridHeight: 6
    property int baseFontSize: 24

    property string entityId
    property QtObject entityObj

    property double mediaInfoHeight: mediaTitle.implicitHeight + mediaArtist.implicitHeight + progressContainer.implicitHeight + 60

    property bool isComponentHorizontal: mediaComponent.height < 260
    property bool isSpaceForMediaInfo: {
        return mediaComponent.height >= 240;
    }

    property alias mediaImage: mediaImage
    property alias aspectFit: mediaImage.aspectFit

    Component.onCompleted: {
        entityObj = EntityController.get(entityId);
    }

    onEntityIdChanged: entityObj = EntityController.get(entityId)

    Connections {
        target: entityObj
        ignoreUnknownSignals: true

        function onStateChanged() {
            if (entityObj.state === MediaPlayerStates.Playing) {
                mediaImageScaleChanger.stop();
                mediaImage.scale = 1;
            } else {
                mediaImageScaleChanger.start();
            }
        }
    }

    Timer {
        id:  mediaImageScaleChanger
        running: false
        repeat: false
        interval: 1000
        onTriggered: mediaImage.scale = 0.8
    }

    MediaPlayerComponents.ImageLoader {
        id: mediaImage
        width: mediaComponent.isComponentHorizontal ? parent.width / 3 : parent.width
        height: mediaComponent.isSpaceForMediaInfo ? (parent.height - mediaInfoHeight) : parent.height
        aspectFit: true
        alignCentered: true
        url: entityObj.mediaImage
        anchors { top: parent.top; horizontalCenter: mediaComponent.isComponentHorizontal ? undefined : parent.horizontalCenter; left: mediaComponent.isComponentHorizontal ? parent.left : undefined }
        scale: 1

        Behavior on scale {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
        }

        Components.Icon {
            id: imageIcon
            color: colors.offwhite
            opacity: 0
            anchors.centerIn: mediaImage
            size: 200
            scale: 0.5

            SequentialAnimation {
                id: imageIconAnimation
                running: false

                PropertyAnimation { target: imageIcon; property: "color"; to: entityObj.mediaImageColor.hslLightness < 0.3 ? colors.white : colors.black; duration: 0 }
                PropertyAnimation { target: imageIcon; property: "icon"; to: entityObj.state === MediaPlayerStates.Playing ? "uc:pause" : "uc:play"; duration: 0 }

                PropertyAnimation { target: imageIcon; property: "scale"; to: 1; easing.type: Easing.OutExpo; duration: 300 }

                ParallelAnimation {
                    PropertyAnimation { target: imageIcon; property: "scale"; to: 1; easing.type: Easing.OutExpo; duration: 300 }
                    PropertyAnimation { target: imageIcon; property: "opacity"; to: 1; easing.type: Easing.OutExpo; duration: 300 }
                }
                ParallelAnimation {
                    PropertyAnimation { target: imageIcon; property: "scale"; to: 1.5; easing.type: Easing.OutExpo; duration: 300 }
                    PropertyAnimation { target: imageIcon; property: "opacity"; to: 0; easing.type: Easing.OutExpo; duration: 300 }
                }
                PropertyAnimation { target: imageIcon; property: "scale"; to: 0.5; duration: 0 }
            }
        }
    }

    MouseArea {
        anchors.fill: mediaImage

        property real velocity: 0.0
        property int xStart: 0
        property int xPrev: 0
        property bool tracking: false
        property int treshold: 4

        onPressed: {
            xStart = mouseX;
            xPrev = mouseX;
            velocity = 0;
            tracking = true;
        }

        onCanceled: tracking = false


        onPositionChanged: {
            let currentVelocity = (mouseX - xPrev);
            velocity = (velocity + currentVelocity) / 2.0
            xPrev = mouseX
        }

        onReleased: {
            tracking = false;

            if (velocity > treshold) {
                entityObj.previous();
            } else if (velocity < -treshold) {
                entityObj.next();
            } else {
                Haptic.play(Haptic.Click);
                entityObj.playPause();

                imageIconAnimation.start();
            }
        }
    }

    Item {
        id: mediaTitle
        anchors {
            top: mediaComponent.isComponentHorizontal ? mediaImage.top : mediaImage.bottom
            topMargin: mediaComponent.isComponentHorizontal ? 5 : 10
            left: mediaComponent.isComponentHorizontal ? mediaImage.right : undefined
            leftMargin: mediaComponent.isComponentHorizontal ? 20 : 0
        }
        width: mediaComponent.isComponentHorizontal ? mediaComponent.width - mediaImage.width - 40 : mediaComponent.width
        height: mediaTitleText.implicitHeight
        clip: true
        visible: mediaComponent.isSpaceForMediaInfo

        property int scrollingWidth: mediaTitleText.implicitWidth - mediaTitle.width
        property int scrollDuration: mediaTitle.scrollingWidth < 0 ? 0 : mediaTitle.scrollingWidth * 25

        onStateChanged: {
            scrollingToLeftAnimation.to = -mediaTitle.scrollingWidth;
        }

        states: [
            State {
                name: "running"
                when: mediaTitle.scrollingWidth > 0
            },
            State {
                name: "notRunning"
                when: mediaTitle.scrollingWidth < 0
                PropertyChanges { target: mediaTitleText; x: 0 }
            }
        ]

        transitions: [
            Transition {
                to: "running"
                SequentialAnimation {
                    loops: Animation.Infinite

                    PropertyAnimation { target: mediaTitleText; property: "x"; to: 0; duration: 0 }
                    PauseAnimation { duration: 2000 }
                    PropertyAnimation { id: scrollingToLeftAnimation; target: mediaTitleText; property: "x"; duration: mediaTitle.scrollDuration }
                    PauseAnimation { duration: 500 }
                    PropertyAnimation { target: mediaTitleText; property: "x"; to: 0; duration: mediaTitle.scrollDuration }

                }
            }
        ]

        Text {
            id: mediaTitleText
            text: entityObj.mediaTitle
            color: colors.offwhite
            font: fonts.primaryFont(mediaComponent.baseFontSize, "Bold")
            maximumLineCount: 1
            x: 0; y: 0
        }
    }

    Text {
        id: mediaArtist
        text: entityObj.mediaArtist
        color: colors.offwhite
        font: fonts.secondaryFont(mediaComponent.baseFontSize * 0.8)
        width: mediaTitle.width
        elide: Text.ElideRight
        maximumLineCount: 1
        anchors {
            top: mediaTitle.bottom
            left: mediaTitle.left
        }
        visible: mediaTitle.visible
    }

    Item {
        id: progressContainer
        width: mediaTitle.width
        implicitHeight: childrenRect.height
        anchors { top: mediaComponent.isComponentHorizontal ? undefined : mediaArtist.bottom; topMargin: 5; bottom: mediaComponent.isComponentHorizontal ? mediaImage.bottom : undefined; bottomMargin: 5; left: mediaTitle.left }
        visible: entityObj.hasAllFeatures([MediaPlayerFeatures.Media_duration, MediaPlayerFeatures.Media_position]) && entityObj.mediaDuration !== 0 && mediaTitle.visible
        enabled: visible

        Text {
            id: mediaPositionText
            text: mediaComponent.formatTime(entityObj.mediaPosition)
            color: colors.offwhite
            horizontalAlignment: Text.AlignLeft
            font: fonts.secondaryFont(20)
            anchors { top: parent.top; left: parent.left }
        }

        Text {
            text: "-" + mediaComponent.formatTime(entityObj.mediaDuration-entityObj.mediaPosition)
            color: colors.offwhite
            horizontalAlignment: Text.AlignRight
            font: fonts.secondaryFont(20)
            anchors { top: parent.top; right: parent.right }
        }

        Slider {
            id: progressSlider
            enabled: entityObj.hasFeature(MediaPlayerFeatures.Seek)
            live: false
            width: parent.width
            height: 8
            snapMode: Slider.SnapAlways
            leftPadding: 0
            rightPadding: 0
            anchors.top: mediaPositionText.bottom

            from: 0
            to: 100
            stepSize: 1
            value: 100 * entityObj.mediaPosition / entityObj.mediaDuration

            onPressedChanged: {
                if (!pressed) {
                    entityObj.seek(progressSlider.value / 100 * entityObj.mediaDuration);
                }
            }

            background: Item {
                x: progressSlider.leftPadding
                y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                implicitWidth: progressSlider.width; implicitHeight: progressSlider.height
                width: progressSlider.availableWidth; height: implicitHeight

                Rectangle {
                    id: bg
                    width: parent.width; height: progressSlider.height
                    anchors.centerIn: parent
                    radius: progressSlider.height/2
                    color: colors.dark


                    Behavior on height {
                        NumberAnimation {
                            duration: 200
                            easing.type: Easing.OutExpo
                        }
                    }

                    Rectangle {
                        width: progressSlider.visualPosition * parent.width; height: parent.height
                        color: Qt.lighter(entityObj.mediaImageColor,3)//colors.offwhite
                        radius: progressSlider.height/2

                        Behavior on color {
                            ColorAnimation { duration: 200 }
                        }
                    }
                }
            }

            handle: Item {
                x: (progressSlider.visualPosition * progressSlider.availableWidth) - width/2
                y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                implicitWidth: 40; implicitHeight: 40
            }
        }
    }
}
