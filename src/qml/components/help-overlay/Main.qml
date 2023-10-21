// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/components/help-overlay" as HelpComponents

HelpComponents.Base {
    property alias content: content

    SwipeView {
        id: content
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: navigation.top; bottomMargin: 10 }

        Image {
            antialiasing: true
            asynchronous: true
            source: "qrc:/images/help-overlay/Main1.png"
        }

        Image {
            antialiasing: true
            asynchronous: true
            source: "qrc:/images/help-overlay/Main2.png"
        }

        Image {
            antialiasing: true
            asynchronous: true
            source: "qrc:/images/help-overlay/Main3.png"
        }
    }
}
