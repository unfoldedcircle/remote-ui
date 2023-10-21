// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/components" as Components

Popup {
    id: remoteOpenRoot
    width: parent.width; height: parent.height
    opacity: 0
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0

    property int countdown: 2

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
    }

    background: Rectangle { color: colors.black }

    Components.Icon {
        id: warning
        icon: "uc:ban"
        color: colors.red
        size: 200
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: width }
    }

    Text {
        width: parent.width - 40
        wrapMode: Text.WordWrap
        color: colors.offwhite
        text: qsTr("Do not operate the device disassembled.")
        horizontalAlignment: Text.AlignHCenter
        anchors { horizontalCenter: parent.horizontalCenter; top: warning.bottom; topMargin: 60 }
        font: fonts.secondaryFont(24)
    }

    Text {
        width: parent.width - 40
        wrapMode: Text.WordWrap
        color: colors.offwhite
        opacity: 0.6
        //: \n and %1 must be included
        text: qsTr("The remote will turn off\nin %1 seconds.").arg(countdown)
        horizontalAlignment: Text.AlignHCenter
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 20 }
        font: fonts.secondaryFont(24)
    }

    Timer {
        running: remoteOpenRoot.opened
        repeat: true
        interval: 1000

        onTriggered: {
            countdown--;

            if (countdown == 0) {
                stop();
            }
        }

    }
}
