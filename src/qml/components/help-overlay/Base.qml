// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

MouseArea {
    id: showHelpBase
    anchors.fill: parent

    property alias navigation: navigation

    Component.onCompleted: buttonNavigation.takeControl()

    function close() {
        ui.showHelp = false;
        buttonNavigation.releaseControl();
    }

    Rectangle {
        anchors.fill: parent
        color: colors.black
        opacity: 0.3
    }

    RowLayout {
        id: navigation
        width: parent.width
        anchors { bottom: parent.bottom; bottomMargin: 10 }

        Components.Icon {
            Layout.leftMargin: 20
            Layout.alignment: Qt.AlignLeft

            opacity: content.currentIndex > 0
            enabled: opacity === 1
            icon: "uc:left-arrow-alt"
            size: 60
            color: colors.offwhite

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    content.decrementCurrentIndex();
                }
            }
        }

        Components.Button {
            Layout.alignment: Qt.AlignHCenter

            text: qsTr("Close")
            trigger: function() {
                showHelpBase.close();
            }
        }

        Components.Icon {
            Layout.rightMargin: 20
            Layout.alignment: Qt.AlignRight

            opacity: content.currentIndex !== content.count - 1
            enabled: opacity === 1
            icon: "uc:right-arrow-alt"
            size: 60
            color: colors.offwhite

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    content.incrementCurrentIndex();
                }
            }
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    showHelpBase.close();
                }
            },
            "HOME": {
                "released": function() {
                    showHelpBase.close();
                }
            },
            "DPAD_MIDDLE": {
                "released": function() {
                    showHelpBase.close();
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    content.decrementCurrentIndex();
                }
            },
            "DPAD_RIGHT": {
                "pressed": function() {
                    content.incrementCurrentIndex();
                }
            }
        }
    }
}
