// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


import Haptic 1.0
import Config 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: colorPageContent

    property bool start: true
    property int colorR: rSlider.value
    property int colorG: gSlider.value
    property int colorB: bSlider.value
    property color primary: Qt.rgba(colorR/255, colorG/255, colorB/255, 1)

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        onContentYChanged: {
            if (contentY < 0) {
                contentY = 0;
            }
            if (contentY > 1100) {
                contentY = 1100;
            }
        }

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 10
            width: ui.width
            anchors.horizontalCenter: parent.horizontalCenter

            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Adjust the color tone of the user interface. Using the sliders, choose a color. The user interface colors will be generated based on that color.")
                    font: fonts.secondaryFont(24)
                }
            }

            Components.Slider {
                id: rSlider
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: 60
                from: 0
                to: 255
                stepSize: 1
                live: true
                fillColor: "red"

                onValueChanged: {
                    if (!start) {
                        colors.generateColorPalette(primary);
                    }

                    gSlider.value = colors.base.g * 255;
                    bSlider.value = colors.base.b * 255;
                }
            }

            Components.Slider {
                id: gSlider
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: 60
                from: 0
                to: 255
                stepSize: 1
                live: true
                fillColor: "green"

                onValueChanged: {
                    if (!start) {
                        colors.generateColorPalette(primary);
                    }

                    rSlider.value = colors.base.r * 255;
                    bSlider.value = colors.base.b * 255;
                }
            }

            Components.Slider {
                id: bSlider
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: 60
                from: 0
                to: 255
                stepSize: 1
                live: true
                fillColor: "blue"

                onValueChanged: {
                    if (!start) {
                        colors.generateColorPalette(primary);
                    }

                    rSlider.value = colors.base.r * 255;
                    gSlider.value = colors.base.g * 255;
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: 380
                radius: ui.cornerRadiusSmall
                color: colors.dark

                Rectangle {
                    width: 200
                    radius: ui.cornerRadiusSmall
                    color: colors.medium
                    anchors { top: parent.top; topMargin: 20; left: parent.left; leftMargin: 20; bottom: parent.bottom; bottomMargin: 20 }

                    Text {
                        anchors.fill: parent
                        padding: 20
                        wrapMode: Text.WordWrap
                        color: colors.light
                        text: qsTr("This is a darker text, in a darker container")
                        font: fonts.secondaryFont(24)
                    }
                }

                Components.Button {
                    id: button
                    //: Caption for a sample button
                    text: qsTr("Button")
                    width: parent.width - 260
                    anchors { right: parent.right; rightMargin: 20; top: parent.top; topMargin: 20 }
                }

                Rectangle {
                    width: button.width
                    radius: ui.cornerRadiusSmall
                    color: colors.light
                    anchors { right: parent.right; rightMargin: 20; top: button.bottom; topMargin: 20; bottom: parent.bottom; bottomMargin: 20 }

                    Rectangle {
                        width: parent.width - 40
                        height: 60
                        radius: ui.cornerRadiusSmall
                        color: colors.highlight
                        anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }
                    }

                    Text {
                        anchors { left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20; bottom: parent.bottom; bottomMargin: 20 }
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Main text color")
                        font: fonts.secondaryFont(24)
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        rSlider.value = colors.base.r * 255;
        gSlider.value = colors.base.g * 255;
        bSlider.value = colors.base.b * 255;
        start = false;
    }
}
