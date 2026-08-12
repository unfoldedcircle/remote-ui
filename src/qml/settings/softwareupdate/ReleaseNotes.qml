// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import SoftwareUpdate 1.0

import "qrc:/components" as Components
import "qrc:/settings" as Settings

Settings.Page {
    id: releaseNotes

    function scrollDown() {
        flickable.contentY = Math.min(flickable.contentY + 100,
                                      Math.max(0, flickable.contentHeight - flickable.height));
    }

    function scrollUp() {
        flickable.contentY = Math.max(0, flickable.contentY - 100);
    }

    // extend, never assign: assigning buttonNavigation.defaultConfig replaces the whole object and
    // drops the BACK / HOME handlers declared in Settings.Page, leaving the page impossible to exit
    // with the keypad.
    Component.onCompleted: {
        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         releaseNotes.scrollDown();
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         releaseNotes.scrollUp();
                                                     }
                                                 }
                                             });
    }

    Flickable {
        id: flickable
        width: parent.width
        anchors { top: topNavigation.bottom; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        contentWidth: parent.width - 20; contentHeight: content.implicitHeight
        clip: true
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds

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
