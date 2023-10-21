// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Dock.Controller 1.0

import "qrc:/components" as Components

Item {
    id: dockSetupFinish

    property bool success: true
    property string dockName
    property alias errorString: errorString.text

    signal done()
    signal failed()

    ColumnLayout {
        visible: dockSetupFinish.success
        anchors.fill: parent
        spacing: 10

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 40
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            text: qsTr("You're all set")
            maximumLineCount: 2
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            color: colors.offwhite
            font: fonts.primaryFont(30)
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            maximumLineCount: 2
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            color: colors.light
            text: qsTr("The dock has been added successfully.")
            font: fonts.secondaryFont(24)
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Image {
                width: parent.width
                height: parent.height * 0.7
                anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: -80 }
                fillMode: Image.PreserveAspectFit
                antialiasing: true
                asynchronous: true
                cache: true
                source: "qrc:/images/dock_setup.png"
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            maximumLineCount: 2
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            color: colors.offwhite
            text: qsTr("%1 is ready to blast IR codes.").arg(dockSetupFinish.dockName)
            font: fonts.secondaryFont(24)
        }

        Components.Button {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 20
            Layout.alignment: Qt.AlignBottom

            text: qsTr("Done")
            trigger: function() {
                dockSetupFinish.done();
            }
        }
    }

    ColumnLayout {
        visible: !dockSetupFinish.success
        anchors.fill: parent
        spacing: 10

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 40
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            text: qsTr("Oops")
            maximumLineCount: 2
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            color: colors.red
            font: fonts.primaryFont(30)
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            maximumLineCount: 2
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            color: colors.light
            text: qsTr("Something went wrong while setting up the dock.")
            font: fonts.secondaryFont(24)
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 60
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            maximumLineCount: 2
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            color: colors.offwhite
            text: qsTr("ERROR:")
            font: fonts.secondaryFont(24)
        }

        Text {
            id: errorString

            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            color: colors.red
            font: fonts.secondaryFont(24)
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Image {
                width: parent.width
                height: parent.height * 0.7
                anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: -80 }
                fillMode: Image.PreserveAspectFit
                antialiasing: true
                asynchronous: true
                cache: true
                source: "qrc:/images/dock_setup.png"
                opacity: 0.3
            }
        }

        Components.Button {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 20
            Layout.alignment: Qt.AlignBottom

            text: qsTr("Try again")
            trigger: function() {
                dockSetupFinish.failed();
            }
        }
    }
}
