// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import SoftwareUpdate 1.0

import "qrc:/components" as Components
import "qrc:/settings" as Settings

Settings.Page {
    id: releaseNotes

    buttonNavigation.defaultConfig: {
        "DPAD_DOWN": {
            "pressed": function() {
                flickable.contentY += 100;
            }
        },
        "DPAD_UP": {
            "pressed": function() {
                flickable.contentY -= 100;
            }
        }
    }

    Flickable {
        id: flickable
        width: parent.width
        anchors { top: topNavigation.bottom; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        contentWidth: parent.width - 20; contentHeight: content.implicitHeight
        clip: true
        flickableDirection: Flickable.VerticalFlick

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        Text {
            id: content
            width: parent.width
            wrapMode: Text.WordWrap
            color: colors.light
            textFormat: Text.MarkdownText
            text: SoftwareUpdate.releaseNotes
            font: fonts.secondaryFont(24)
            x: 10
        }

        ScrollBar.vertical: ScrollBar {
            opacity: 0.5
        }
    }
}
