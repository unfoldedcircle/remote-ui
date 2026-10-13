// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import SoftwareUpdate 1.0

import "qrc:/components" as Components
import "qrc:/settings" as Settings

Settings.Page {
    id: releaseNotes

    // extend, never assign: assigning buttonNavigation.defaultConfig replaces the whole object and
    // drops the BACK / HOME handlers declared in Settings.Page, leaving the page impossible to exit
    // with the keypad.
    Component.onCompleted: {
        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         flickable.scrollStep(1);
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         flickable.scrollStep(-1);
                                                     }
                                                 }
                                             });
    }

    Components.Prose {
        id: flickable
        width: parent.width
        anchors { top: topNavigation.bottom; bottom: parent.bottom }
        textFormat: Text.MarkdownText
        text: SoftwareUpdate.releaseNotes
    }
}
