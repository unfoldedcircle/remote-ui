// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

Item {
    width: parent.width; height: parent.height

    Image {
        width: 480; height: 460
        source: "qrc:/button-simulator/button-layout.png"
    }


    Flow {
        anchors.fill: parent

        Button {
            width: 96
            height: 90
            key: Qt.Key_Exit
        }

        Button {
            width: 288
            height: 90
            key: Qt.Key_Home
        }

        Button {
            width: 96
            height: 90
            key: Qt.Key_F3
        }

        Flow {
            width: 96
            height: 280

            Button {
                width: 90
                height: 140
                key: Qt.Key_VolumeUp
            }

            Button {
                width: 90
                height: 140
                key: Qt.Key_VolumeDown
            }
        }

        Flow {
            width: 288
            height: 280

            Button {
                width: 96
                height: 93
                key: Qt.Key_Green
            }

            Button {
                width: 96
                height: 93
                key: Qt.Key_Up
            }

            Button {
                width: 96
                height: 93
                key: Qt.Key_Yellow
            }

            Button {
                width: 96
                height: 93
                key: Qt.Key_Left
            }

            Button {
                width: 96
                height: 93
                key: Qt.Key_Return
            }

            Button {
                width: 96
                height: 93
                key: Qt.Key_Right
            }

            Button {
                width: 96
                height: 93
                key: Qt.Key_Red
            }

            Button {
                width: 96
                height: 93
                key: Qt.Key_Down
            }

            Button {
                width: 96
                height: 93
                key: Qt.Key_Blue
            }
        }

        Flow {
            width: 96
            height: 280

            Button {
                width: 90
                height: 140
                key: Qt.Key_ChannelUp
            }

            Button {
                width: 90
                height: 140
                key: Qt.Key_ChannelDown
            }
        }

        Button {
            width: 96
            height: 90
            key: Qt.Key_VolumeMute
        }

        Button {
            width: 96
            height: 90
            key: Qt.Key_AudioRewind
        }

        Button {
            width: 96
            height: 90
            key: Qt.Key_MediaTogglePlayPause
        }

        Button {
            width: 96
            height: 90
            key: Qt.Key_AudioForward
        }

        Button {
            width: 96
            height: 90
            key: Qt.Key_PowerOff
        }
    }
}
