// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import Entity.MediaPlayer 1.0
import HwInfo 1.0

import "qrc:/components" as Components
import "qrc:/components/entities/media_player" as MediaPlayerComponents
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: mediaPlayerBase

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

    function startIconAnimation() {
        imageIconAnimation.start();
    }

    Component.onCompleted: {
        if (entityObj.hasFeature(MediaPlayerFeatures.Home)) {
            if (!overrideConfig["HOME"]) {
                overrideConfig["HOME"] = ({});
            }

            overrideConfig["HOME"]["pressed"] = function() {
                entityObj.home();
            }
        }

        if (entityObj.hasAnyFeature([MediaPlayerFeatures.Home, MediaPlayerFeatures.Menu])) {
            if (!overrideConfig["BACK"]) {
                overrideConfig["BACK"] = ({});
            }

            overrideConfig["BACK"]["pressed"] = function() {
                entityObj.back();
            }
        }

        if (entityObj.hasFeature(MediaPlayerFeatures.Color_buttons)) {
            if (!overrideConfig["RED"]) {
                overrideConfig["RED"] = ({});
            }

            overrideConfig["RED"]["pressed"] = function() {
                entityObj.functionRed();
            }

            if (!overrideConfig["GREEN"]) {
                overrideConfig["GREEN"] = ({});
            }

            overrideConfig["GREEN"]["pressed"] = function() {
                entityObj.functionGreen();
            }

            if (!overrideConfig["YELLOW"]) {
                overrideConfig["YELLOW"] = ({});
            }

            overrideConfig["YELLOW"]["pressed"] = function() {
                entityObj.functionYellow();
            }

            if (!overrideConfig["BLUE"]) {
                overrideConfig["BLUE"] = ({});
            }

            overrideConfig["BLUE"]["pressed"] = function() {
                entityObj.functionBlue();
            }
        }

        if (entityObj.hasFeature(MediaPlayerFeatures.Dpad)) {
            if (!overrideConfig["DPAD_UP"]) {
                overrideConfig["DPAD_UP"] = ({});
            }

            overrideConfig["DPAD_UP"]["pressed"] = function() {
                entityObj.cursorUp();
            }

            if (!overrideConfig["DPAD_DOWN"]) {
                overrideConfig["DPAD_DOWN"] = ({});
            }

            overrideConfig["DPAD_DOWN"]["pressed"] = function() {
                entityObj.cursorDown();
            }

            if (!overrideConfig["DPAD_LEFT"]) {
                overrideConfig["DPAD_LEFT"] = ({});
            }

            overrideConfig["DPAD_LEFT"]["pressed"] = function() {
                entityObj.cursorLeft();
            }

            if (!overrideConfig["DPAD_RIGHT"]) {
                overrideConfig["DPAD_RIGHT"] = ({});
            }

            overrideConfig["DPAD_RIGHT"]["pressed"] = function() {
                entityObj.cursorRight();
            }

            if (!overrideConfig["DPAD_MIDDLE"]) {
                overrideConfig["DPAD_MIDDLE"] = ({});
            }

            overrideConfig["DPAD_MIDDLE"]["pressed"] = function() {
                entityObj.cursorEnter();
            }
        }

        if (entityObj.hasFeature(MediaPlayerFeatures.Context_menu)) {
            if (!overrideConfig["DPAD_MIDDLE"]) {
                overrideConfig["DPAD_MIDDLE"] = ({});
            }

            overrideConfig["DPAD_MIDDLE"]["long_press"] = function() {
                entityObj.contextMenu();
            }
        }

        if (entityObj.hasFeature(MediaPlayerFeatures.Channel_switcher)) {
            if (!overrideConfig["CHANNEL_UP"]) {
                overrideConfig["CHANNEL_UP"] = ({});
            }

            overrideConfig["CHANNEL_UP"]["pressed"] = function() {
                entityObj.channelUp();
            }

            if (!overrideConfig["CHANNEL_DOWN"]) {
                overrideConfig["CHANNEL_DOWN"] = ({});
            }

            overrideConfig["CHANNEL_DOWN"]["pressed"] = function() {
                entityObj.channelDown();
            }
        }

        if (entityObj.hasFeature(MediaPlayerFeatures.Fast_forward)) {
            if (!overrideConfig["NEXT"]) {
                overrideConfig["NEXT"] = ({});
            }

            overrideConfig["NEXT"]["pressed"] = function() {
                entityObj.fastForward();
            }
        } else {
            if (!overrideConfig["NEXT"]) {
                overrideConfig["NEXT"] = ({});
            }

            overrideConfig["NEXT"]["pressed"] = function() {
                entityObj.next();
            }
        }

        if (entityObj.hasFeature(MediaPlayerFeatures.Rewind)) {
            if (!overrideConfig["PREV"]) {
                overrideConfig["PREV"] = ({});
            }

            overrideConfig["PREV"]["pressed"] = function() {
                entityObj.rewind();
            }
        } else {
            if (!overrideConfig["PREV"]) {
                overrideConfig["PREV"] = ({});
            }

            overrideConfig["PREV"]["pressed"] = function() {
                entityObj.previous();
            }
        }
    }

    overrideConfig: {
        "VOLUME_UP": {
            "pressed": function() {
                entityObj.volumeUp();
                volume.start(entityObj);
            }
        },
        "VOLUME_DOWN": {
            "pressed": function() {
                entityObj.volumeDown();
                volume.start(entityObj, false);
            }
        },
        "MUTE": {
            "pressed": function() {
                entityObj.muteToggle();
            }
        },
        "PLAY": {
            "pressed": function() {
                entityObj.playPause();
                mediaPlayerBase.startIconAnimation();
            }
        },
        "POWER": {
            "pressed": function() {
                if (entityObj.state === MediaPlayerStates.Off) {
                    entityObj.turnOn();
                } else {
                    entityObj.turnOff();
                }
            }
        }
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    Item {
        width: parent.width
        height: parent.height - title.height
        anchors { top: title.bottom }

        property alias imageIconAnimation: imageIconAnimation

        Item {
            id: mediaImageContainer
            width: parent.width
            height: parent.height
            anchors.horizontalCenter: parent.horizontalCenter

            MediaPlayerComponents.ImageLoader {
                id: mediaImage
                width: parent.width
                height: HwInfo.modelNumber == "UCR2" ? width : width - 60
                url: entityObj.mediaImage
                aspectFit: true
                anchors { top: parent.top }
                scale: entityObj.state === MediaPlayerStates.Playing ? 1 : 0.8

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

                Rectangle {
                    width: sourceText.implicitWidth + ui.cornerRadiusSmall * 2
                    height: sourceText.implicitHeight
                    radius: ui.cornerRadiusSmall
                    color: colors.dark
                    opacity: 0.9
                    layer.enabled: true
                    visible: entityObj.source !== "" && entityObj.state === MediaPlayerStates.Playing
                    anchors { top: parent.top; topMargin: 10; right: parent.right; rightMargin: 10 }

                    Text {
                        id: sourceText
                        text: entityObj.source
                        color: colors.offwhite
                        font: fonts.secondaryFont(26)
                        anchors.centerIn: parent
                    }
                }

                MouseArea {
                    anchors.fill: parent

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

                            mediaPlayerBase.startIconAnimation();
                        }
                    }
                }
            }

            Rectangle {
                width: mediaTypeText.implicitWidth + ui.cornerRadiusSmall * 2
                height: mediaTypeText.implicitHeight
                radius: ui.cornerRadiusSmall
                color: colors.dark
                opacity: 0.9
                layer.enabled: true
                visible: entityObj.state === MediaPlayerStates.Playing && entityObj.mediaType !== ""
                anchors { bottom: mediaTitle.top; bottomMargin: 30; left: mediaImageContainer.left; leftMargin: 10 }

                Text {
                    id: mediaTypeText
                    text: entityObj.mediaType
                    color: colors.offwhite
                    font: fonts.secondaryFontCapitalized(26)
                    anchors.centerIn: parent
                }
            }

            Text {
                id: nothingPlayingText
                width: parent.width - 40
                text: qsTr("Nothing is playing")
                color: colors.offwhite
                font: fonts.secondaryFont(26)
                horizontalAlignment: Text.AlignHCenter
                anchors { top: parent.top; topMargin: 200; horizontalCenter: parent.horizontalCenter }
                visible: ((entityObj.mediaTitle === "" && entityObj.mediaArtist === "" && entityObj.mediaAlbum === "") || entityObj.state === MediaPlayerStates.Off) && entityObj.mediaImage == ""
            }

            Text {
                width: parent.width - 40
                text: qsTr("Open an app or use the directional keys to navigate.")
                color: colors.light
                font: fonts.secondaryFont(26)
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                anchors { top: nothingPlayingText.bottom; topMargin: 10; horizontalCenter: nothingPlayingText.horizontalCenter }
                visible: nothingPlayingText.visible
            }

            Item {
                id: mediaTitle
                anchors { top: mediaImage.bottom; topMargin: 20 }
                width: parent.width
                height: mediaTitleText.implicitHeight
                clip: true

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
                    font: fonts.primaryFont(46, "Bold")
                    maximumLineCount: 1
                    x: 0; y: 0
                }

            }

            Text {
                id: mediaArtist
                text: entityObj.mediaArtist
                color: colors.offwhite
                font: fonts.secondaryFont(26)
                width: parent.width
                height: entityObj.mediaArtist !== "" ? undefined : 0
                elide: Text.ElideRight
                maximumLineCount: 1
                anchors { top: mediaTitle.bottom }
                visible: entityObj.mediaArtist !== ""
            }

            Item {
                id: progressContainer
                width: parent.width
                implicitHeight: childrenRect.height
                anchors { top: mediaArtist.bottom; topMargin: 20 }
                visible: entityObj.hasAllFeatures([MediaPlayerFeatures.Media_duration, MediaPlayerFeatures.Media_position]) && entityObj.state !== MediaPlayerStates.Off;
                enabled: visible
                opacity: entityObj.mediaDuration === 0 ? 0.6 : 1

                Text {
                    id: mediaPositionText
                    text: entityObj.mediaDuration === 0 ? qsTr("Live") : mediaPlayerBase.formatTime(entityObj.mediaPosition)
                    color: colors.offwhite
                    horizontalAlignment: Text.AlignLeft
                    font: fonts.secondaryFont(20)
                    anchors { top: parent.top; left: entityObj.mediaDuration === 0 ? undefined : parent.left; horizontalCenter: entityObj.mediaDuration === 0 ? parent.horizontalCenter : undefined }
                }

                Text {
                    text: "-" + mediaPlayerBase.formatTime(entityObj.mediaDuration-entityObj.mediaPosition)
                    color: colors.offwhite
                    horizontalAlignment: Text.AlignRight
                    font: fonts.secondaryFont(20)
                    anchors { top: parent.top; right: parent.right }
                    visible: entityObj.mediaDuration !== 0
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

        RowLayout {
            id: controlsContainer

            width: parent.width
            height: 80
            anchors { bottom: parent.bottom }

            Components.Icon {
                id: shuffleIcon

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                size: 80
                color: entityObj.shuffleIsOn ? colors.offwhite : colors.light
                icon: "uc:shuffle"
                visible: entityObj.hasFeature(MediaPlayerFeatures.Shuffle)

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        entityObj.shuffle();
                    }
                }
            }

            Components.Icon {
                id: repeatIcon

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                size: 80
                color: entityObj.repeatMode === MediaPlayerRepeatMode.OFF ? colors.light : colors.offwhite
                icon: "uc:repeat"
                visible: entityObj.hasFeature(MediaPlayerFeatures.Repeat)

                Rectangle {
                    width: repeatText.implicitWidth + 20
                    height: repeatText.implicitHeight + 2
                    radius: ui.cornerRadiusSmall
                    color: colors.medium
                    anchors { centerIn: parent; horizontalCenterOffset: 40; verticalCenterOffset: -16 }
                    visible: entityObj.repeatMode !== MediaPlayerRepeatMode.OFF

                    Text {
                        id: repeatText
                        color: colors.white
                        text: {
                            switch (entityObj.repeatMode) {
                            case MediaPlayerRepeatMode.OFF:
                                return "";
                            case MediaPlayerRepeatMode.ALL:
                                return qsTr("All");
                            case MediaPlayerRepeatMode.ONE:
                                return qsTr("One");
                            }
                        }
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: fonts.secondaryFontCapitalized(16)
                        anchors.centerIn: parent
                    }
                }

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        entityObj.repeat();
                    }
                }
            }

            Components.Icon {
                id: sourceListIcon

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                size: 80
                color: colors.offwhite
                icon: "uc:grid-2"
                visible: entityObj.hasFeature(MediaPlayerFeatures.Select_source) && entityObj.sourceList.length !== 0

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        sourceList.title = qsTr("Apps")
                        let items = [];

                        for (const source of entityObj.sourceList) {
                            items.push({
                                           title: source,
                                           callback: function() {
                                               entityObj.selectSource(source);
                                           }
                                       });
                        }

                        sourceList.items = items;
                        sourceList.open();
                    }
                }
            }
        }
    }

    MediaPlayerComponents.SourceList {
        id: sourceList
        parent: Overlay.overlay
    }

    Components.TouchSlider {
        id: touchSlider
        entityObj: mediaPlayerBase.entityObj
        active: HwInfo.modelNumber == "UCR3" || HwInfo.modelNumber == "DEV"
        parent: Overlay.overlay
    }
}
