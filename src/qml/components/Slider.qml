// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SLIDER COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - from
 - to
 - stepSize
 - value
 - live
 - showTicks
 - showLiveValue
 - lowValueText
 - highValueText
 - highlight
**/

import QtQuick 2.15
import QtQuick.Controls 2.15

Slider {
    id: slider
    live: false
    width: parent.width
    height: 28
    snapMode: Slider.SnapAlways
    leftPadding: 0
    rightPadding: 0

    property bool showTicks: false
    property bool showLiveValue: true
    property alias lowValueText: lowValueText.text
    property alias highValueText: highValueText.text
    property alias valueDisplayText: valueDisplayText.text
    property bool highlight: false
    property alias fillColor: fill.color

    signal userInteractionStarted()
    signal userInteractionEnded()

    background: Item {
        id: sliderBG
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        implicitWidth: slider.width; implicitHeight: slider.height
        width: slider.availableWidth; height: implicitHeight

        Rectangle {
            width: bg.width + 4
            height: bg.height + 4
            radius: bg.radius
            color: highlight ? colors.highlight : colors.transparent
            anchors.centerIn: bg
        }

        Rectangle {
            id: bg
            width: parent.width; height: slider.pressed ? slider.height : slider.height/2
            anchors.centerIn: parent
            radius: slider.pressed ? slider.height/2 : slider.height/4
            color: colors.dark


            Behavior on height {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutExpo
                }
            }

            Rectangle {
                id: fill
                width: slider.visualPosition * parent.width; height: parent.height
                color: colors.offwhite
                radius: slider.height/2
            }
        }
    }

    handle: Item {
        x: (slider.visualPosition * slider.availableWidth) - width/2
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        implicitWidth: 40; implicitHeight: 40
    }

    Row {
        anchors { top: sliderBG.bottom; topMargin: slider.pressed ? 10 : -5; horizontalCenter: parent.horizontalCenter }
        spacing: (slider.width-14) / (slider.to - slider.from)
        visible: showTicks
        enabled: showTicks

        Behavior on anchors.topMargin {
            NumberAnimation { duration: 100 }
        }

        Repeater {
            model: slider.to - slider.from + 1

            Rectangle {
                width: 2
                height: 10
                color: colors.offwhite
                opacity: 0.3
            }
        }
    }

    onValueChanged: {
        valueDisplayText.text = value;

    }

    Rectangle {
        id: valueDisplay
        x: {
            if (slider.handle.x + valueDisplay.width > ui.width) {
                return ui.width - valueDisplay.width - 20;
            } else if (slider.handle.x == -20) {
                return slider.handle.x + 20;
            } else {
                return slider.handle.x;
            }
        }

        width: valueDisplayText.implicitWidth + 20; height: 80
        color: colors.dark
        opacity: 0
        visible: showLiveValue
        enabled: visible
        radius: ui.cornerRadiusSmall
        anchors { bottom: slider.handle.top }

        Behavior on x {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutExpo
            }
        }


        Behavior on opacity {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutExpo
            }
        }

        Text {
            id: valueDisplayText
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            font: fonts.secondaryFont(40)
            anchors.centerIn: parent
        }
    }

    onPressedChanged: {
        if (slider.pressed) {
            userInteractionStarted();
            valueDisplay.opacity = 1;
        } else {
            userInteractionEnded();
            valueDisplay.opacity = 0;
        }
    }

    Text {
        id: lowValueText
        color: colors.offwhite
        font: fonts.secondaryFont(20)
        anchors { left: parent.left; top: sliderBG.bottom; topMargin: slider.pressed ? 20 : 5 }
        visible: lowValueText.text != ""

        Behavior on anchors.topMargin {
            NumberAnimation { duration: 100 }
        }
    }

    Text {
        id: highValueText
        color: colors.offwhite
        font: fonts.secondaryFont(20)
        anchors { right: parent.right; top: sliderBG.bottom; topMargin: slider.pressed ? 20 : 5 }
        visible: highValueText.text != ""

        Behavior on anchors.topMargin {
            NumberAnimation { duration: 100 }
        }
    }
}
