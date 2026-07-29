// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import Config 1.0
import TouchSlider 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: touchSliderPageContent

    // the use case reflected by the test popup — the focused / last-adjusted sensitivity
    property real testGain: Config.touchSliderGainVolume
    property string testLabel: qsTr("Volume")
    property string testIcon: "uc:volume"

    // adjustable range for the sweep gain, see the touch slider components
    readonly property real gainMin: 0.1
    readonly property real gainMax: 2.0

    function selectTest(value, label, icon) {
        touchSliderPageContent.testGain = value;
        touchSliderPageContent.testLabel = label;
        touchSliderPageContent.testIcon = icon;
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 20
            width: flickable.width
            anchors.horizontalCenter: parent.horizontalCenter

            /** MASTER ENABLE SWITCH **/
            RowLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                Layout.topMargin: 10
                Layout.preferredWidth: content.width - 20
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Touch slider")
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: enabledSwitch
                    icon: "uc:check"
                    checked: Config.touchSliderEnabled
                    trigger: function() {
                        Config.touchSliderEnabled = !Config.touchSliderEnabled;
                    }

                    KeyNavigation.down: volumeSlider
                    highlight: activeFocus && ui.keyNavigationEnabled

                    Component.onCompleted: enabledSwitch.forceActiveFocus()
                }
            }

            Text {
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: content.width - 20
                wrapMode: Text.WordWrap
                color: colors.light
                text: qsTr("When off, the touch slider is disabled everywhere and swiping it does nothing.")
                font: fonts.primaryFont(24)
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: content.width - 20; height: 2
                color: colors.medium
            }

            Text {
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: content.width - 20
                wrapMode: Text.WordWrap
                color: colors.light
                text: qsTr("Adjust how far the touch slider moves a value for a full-length swipe. Higher is more sensitive; 1.0 means one full swipe covers the whole range.")
                font: fonts.primaryFont(24)
            }

            /** TEST HINT **/
            Text {
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: content.width - 20
                wrapMode: Text.WordWrap
                color: colors.offwhite
                text: qsTr("Slide the hardware slider to test the highlighted setting.")
                font: fonts.primaryFont(24)
            }

            /** VOLUME **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: content.width - 20
                height: childrenRect.height

                Text {
                    id: volumeLabel
                    width: parent.width - 100
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Volume")
                    anchors { left: parent.left; top: parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    color: colors.offwhite
                    text: volumeSlider.value.toFixed(1) + "x"
                    anchors { right: parent.right; verticalCenter: volumeLabel.verticalCenter }
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    id: volumeSlider
                    height: 60
                    from: touchSliderPageContent.gainMin
                    to: touchSliderPageContent.gainMax
                    stepSize: 0.1
                    live: true
                    showLiveValue: false
                    anchors { top: volumeLabel.bottom; topMargin: 10 }

                    Component.onCompleted: value = Config.touchSliderGainVolume
                    onActiveFocusChanged: if (activeFocus) touchSliderPageContent.selectTest(value, qsTr("Volume"), "uc:volume")
                    onUserInteractionStarted: touchSliderPageContent.selectTest(value, qsTr("Volume"), "uc:volume")
                    onMoved: touchSliderPageContent.selectTest(value, qsTr("Volume"), "uc:volume")
                    onUserInteractionEnded: {
                        Config.touchSliderGainVolume = value;
                        touchSliderPageContent.selectTest(value, qsTr("Volume"), "uc:volume");
                        Haptic.play(Haptic.Bump);
                    }

                    KeyNavigation.up: enabledSwitch
                    KeyNavigation.down: brightnessSlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            /** BRIGHTNESS **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: content.width - 20
                height: childrenRect.height

                Text {
                    id: brightnessLabel
                    width: parent.width - 100
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Brightness")
                    anchors { left: parent.left; top: parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    color: colors.offwhite
                    text: brightnessSlider.value.toFixed(1) + "x"
                    anchors { right: parent.right; verticalCenter: brightnessLabel.verticalCenter }
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    id: brightnessSlider
                    height: 60
                    from: touchSliderPageContent.gainMin
                    to: touchSliderPageContent.gainMax
                    stepSize: 0.1
                    live: true
                    showLiveValue: false
                    anchors { top: brightnessLabel.bottom; topMargin: 10 }

                    Component.onCompleted: value = Config.touchSliderGainBrightness
                    onActiveFocusChanged: if (activeFocus) touchSliderPageContent.selectTest(value, qsTr("Brightness"), "uc:brightness")
                    onUserInteractionStarted: touchSliderPageContent.selectTest(value, qsTr("Brightness"), "uc:brightness")
                    onMoved: touchSliderPageContent.selectTest(value, qsTr("Brightness"), "uc:brightness")
                    onUserInteractionEnded: {
                        Config.touchSliderGainBrightness = value;
                        touchSliderPageContent.selectTest(value, qsTr("Brightness"), "uc:brightness");
                        Haptic.play(Haptic.Bump);
                    }

                    KeyNavigation.up: volumeSlider
                    KeyNavigation.down: positionSlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            /** COVER POSITION **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: content.width - 20
                height: childrenRect.height

                Text {
                    id: positionLabel
                    width: parent.width - 100
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Cover position")
                    anchors { left: parent.left; top: parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    color: colors.offwhite
                    text: positionSlider.value.toFixed(1) + "x"
                    anchors { right: parent.right; verticalCenter: positionLabel.verticalCenter }
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    id: positionSlider
                    height: 60
                    from: touchSliderPageContent.gainMin
                    to: touchSliderPageContent.gainMax
                    stepSize: 0.1
                    live: true
                    showLiveValue: false
                    anchors { top: positionLabel.bottom; topMargin: 10 }

                    Component.onCompleted: value = Config.touchSliderGainPosition
                    onActiveFocusChanged: if (activeFocus) touchSliderPageContent.selectTest(value, qsTr("Cover position"), "uc:blind")
                    onUserInteractionStarted: touchSliderPageContent.selectTest(value, qsTr("Cover position"), "uc:blind")
                    onMoved: touchSliderPageContent.selectTest(value, qsTr("Cover position"), "uc:blind")
                    onUserInteractionEnded: {
                        Config.touchSliderGainPosition = value;
                        touchSliderPageContent.selectTest(value, qsTr("Cover position"), "uc:blind");
                        Haptic.play(Haptic.Bump);
                    }

                    KeyNavigation.up: brightnessSlider
                    KeyNavigation.down: seekSlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            /** SEEK **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: content.width - 20
                height: childrenRect.height
                Layout.bottomMargin: 20

                Text {
                    id: seekLabel
                    width: parent.width - 100
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Seek")
                    anchors { left: parent.left; top: parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    color: colors.offwhite
                    text: seekSlider.value.toFixed(1) + "x"
                    anchors { right: parent.right; verticalCenter: seekLabel.verticalCenter }
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    id: seekSlider
                    height: 60
                    from: touchSliderPageContent.gainMin
                    to: touchSliderPageContent.gainMax
                    stepSize: 0.1
                    live: true
                    showLiveValue: false
                    anchors { top: seekLabel.bottom; topMargin: 10 }

                    Component.onCompleted: value = Config.touchSliderGainSeek
                    onActiveFocusChanged: if (activeFocus) touchSliderPageContent.selectTest(value, qsTr("Seek"), "uc:forward")
                    onUserInteractionStarted: touchSliderPageContent.selectTest(value, qsTr("Seek"), "uc:forward")
                    onMoved: touchSliderPageContent.selectTest(value, qsTr("Seek"), "uc:forward")
                    onUserInteractionEnded: {
                        Config.touchSliderGainSeek = value;
                        touchSliderPageContent.selectTest(value, qsTr("Seek"), "uc:forward");
                        Haptic.play(Haptic.Bump);
                    }

                    KeyNavigation.up: positionSlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }
        }
    }

    /** LIVE TEST: drives the same style of popup as normal use, titled "Test – <use case>" **/
    Item {
        id: testArea

        property int testValue: 50
        property real prevTouchX: 0
        property real deltaAcc: 0
        property real touchScale: 0

        Timer {
            id: testCloseTimer
            interval: 1000
            onTriggered: testPopup.close()
        }

        Connections {
            target: TouchSliderProcessor
            ignoreUnknownSignals: true
            enabled: Config.touchSliderEnabled

            function onTouchPressed() {
                testArea.prevTouchX = TouchSliderProcessor.touchX;
                testArea.deltaAcc = 0;
                let axisRange = TouchSliderProcessor.touchXMax - TouchSliderProcessor.touchXMin;
                if (axisRange <= 0) {
                    axisRange = 300;
                }
                testArea.touchScale = 100 * touchSliderPageContent.testGain / axisRange;

                testCloseTimer.stop();
                if (!testPopup.visible) {
                    testPopup.open();
                }
            }

            function onTouchXChanged(x) {
                testArea.deltaAcc += (x - testArea.prevTouchX) * testArea.touchScale;
                testArea.prevTouchX = x;

                const step = testArea.deltaAcc > 0 ? Math.floor(testArea.deltaAcc)
                                                   : Math.ceil(testArea.deltaAcc);
                if (step === 0) {
                    return;
                }
                testArea.deltaAcc -= step;

                const newValue = Math.max(0, Math.min(100, testArea.testValue + step));
                if (newValue === testArea.testValue) {
                    return;
                }
                testArea.testValue = newValue;

                Haptic.play(Haptic.Bump);
                testCloseTimer.restart();
            }

            function onTouchReleased() {
                testCloseTimer.restart();
            }
        }
    }

    /** TEST POPUP — mirrors the normal-use slider overlay, marked as a test **/
    Popup {
        id: testPopup
        width: ui.width; height: ui.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        y: 500
        parent: Overlay.overlay
        background: Item {}

        enter: Transition {
            PropertyAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            PropertyAnimation { properties: "y"; from: 500; to: 0; easing.type: Easing.OutExpo; duration: 300 }
        }

        exit: Transition {
            PropertyAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 300 }
            PropertyAnimation { properties: "y"; from: 0; to: 500; easing.type: Easing.InExpo; duration: 300 }
        }

        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, parent.height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 0.5; color: colors.black }
                GradientStop { position: 1.0; color: colors.black }
            }
        }

        Text {
            text: qsTr("Test") + " – " + touchSliderPageContent.testLabel
            width: parent.width - 20
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: 1
            elide: Text.ElideRight
            color: colors.white
            font: fonts.primaryFont(30)
            anchors { bottom: testBar.top; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }
        }

        Rectangle {
            id: testBar
            width: parent.width - 20
            height: 100
            color: colors.transparent
            border { width: 1; color: Qt.hsla(colors.white.hslHue, colors.white.hslSaturation, colors.white.hslLightness, 0.3) }
            radius: height / 2
            anchors { bottom: parent.bottom; bottomMargin: 10; horizontalCenter: parent.horizontalCenter }

            Item {
                id: testBarIcon
                width: 100
                height: 100
                anchors { left: parent.left }

                Components.Icon {
                    color: colors.white
                    anchors.centerIn: parent
                    icon: touchSliderPageContent.testIcon
                    size: 80
                }
            }

            Rectangle {
                id: testBarVisual
                color: colors.transparent
                border { width: 1; color: colors.white }
                radius: 6
                height: 12
                anchors { verticalCenter: parent.verticalCenter; left: testBarIcon.right; right: testBarValue.left }

                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource:
                        Rectangle {
                        width: testBarVisual.width
                        height: testBarVisual.height
                        radius: testBarVisual.radius
                    }
                }

                Rectangle {
                    color: colors.white
                    x: 0
                    width: testBarVisual.width * testArea.testValue / 100
                    height: 12

                    Behavior on width {
                        NumberAnimation { duration: 200; easing.type: Easing.OutExpo }
                    }
                }
            }

            Item {
                id: testBarValue
                width: 100
                height: 100
                anchors { right: parent.right }

                Text {
                    text: testArea.testValue
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.NoWrap
                    maximumLineCount: 1
                    color: colors.white
                    font: fonts.primaryFont(40)
                    anchors.fill: parent
                }
            }
        }
    }
}
