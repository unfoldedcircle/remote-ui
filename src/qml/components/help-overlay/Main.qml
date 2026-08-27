// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/components/help-overlay" as HelpComponents

HelpComponents.Base {
    property alias content: content

    // The tips used to be pre-rendered images with English text baked in, which left every other
    // language with English tips. They are drawn here instead so the texts go through qsTr().
    SwipeView {
        id: content
        anchors.fill: parent

        HelpComponents.Tip {
            title: qsTr("Useful tips")
            text: qsTr("Check out a few useful tips on where to find important elements of the UI.")
                  + "\n\n"
                  //: Second paragraph of the first tip; the URL is not translated
                  + qsTr("For more tips visit unfoldedcircle.com/support")
        }

        HelpComponents.Tip {
            //: Tip pointing at the round button in the top right corner of the main screen
            title: qsTr("Tap here to open settings")
            titleAlignment: Text.AlignRight
            titleRightMargin: 60

            // the settings button sits in the top right corner of the main screen
            Rectangle {
                width: 40; height: 40
                radius: 20
                color: colors.offwhite
                anchors { top: parent.top; right: parent.right }
            }
        }

        HelpComponents.Tip {
            title: qsTr("Status bar")
            text: qsTr("Battery level, WiFi connection problem, software update indicator and the page title may appear here.")
                  + "\n\n"
                  + qsTr("You can also tap the status bar to scroll to the top of a page.")
            topMargin: 60

            // the status bar spans the top of the main screen
            Rectangle {
                height: 30
                radius: 15
                color: colors.offwhite
                anchors { top: parent.top; topMargin: 5; left: parent.left; leftMargin: 5; right: parent.right; rightMargin: 5 }
            }
        }
    }
}
