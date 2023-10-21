// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Integration.Controller 1.0

import "qrc:/components" as Components

Item {
    id: integrationSetupFinish

    property bool success: true
    property alias errorString: errorString.text

    signal done()
    signal failed()

    ColumnLayout {
        visible: integrationSetupFinish.success
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
            text: qsTr("The integration has been added successfully.")
            font: fonts.secondaryFont(24)
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 20

            Rectangle {
                Layout.preferredWidth: 60
                Layout.preferredHeight: 60

                radius: 30
                color: colors.offwhite

                Components.Icon {
                    icon: IntegrationController.integrationDriverTosetup.icon
                    size: 60
                    color: colors.black
                    anchors.centerIn: parent
                }
            }

            Text {
                Layout.fillWidth: true

                color: colors.offwhite
                text: IntegrationController.integrationDriverTosetup.name
                maximumLineCount: 1
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                font: fonts.primaryFont(30)
            }
        }

        Components.AboutInfo {
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            key: qsTr("Version")
            value: IntegrationController.integrationDriverTosetup.version
            lineTop: true
        }

        Components.AboutInfo {
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            key: qsTr("Developer")
            value: IntegrationController.integrationDriverTosetup.developerName
            multiline: true
            lineBottom: IntegrationController.integrationDriverTosetup.homepage !== ""
        }

        Components.AboutInfo {
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 20

            visible: IntegrationController.integrationDriverTosetup.homepage !== ""
            key: qsTr("Website")
            value: IntegrationController.integrationDriverTosetup.homepage
            multiline: true
            lineBottom: false
        }

        Components.Button {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 20
            Layout.alignment: Qt.AlignBottom

            text: qsTr("Done")
            trigger: function() {
                integrationSetupFinish.done();
            }
        }
    }

    ColumnLayout {
        visible: !integrationSetupFinish.success
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
            text: qsTr("Something went wrong while setting up the integration.")
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
        }

        Components.Button {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 20
            Layout.alignment: Qt.AlignBottom

            text: qsTr("Try again")
            trigger: function() {
                integrationSetupFinish.failed()
            }
        }
    }
}
