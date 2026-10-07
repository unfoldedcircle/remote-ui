// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0
import SoftwareUpdate 1.0
import Battery 1.0

import "qrc:/components" as Components
import "qrc:/settings" as Settings
import "qrc:/settings/softwareupdate" as Softwareupdate

Settings.Page {
    id: softwareUpdatePage
    scrollTarget: flickable
    // the release notes row and the install button only exist while an update is offered
    initialFocusItem: SoftwareUpdate.updateAvailable ? releaseNotesRow : checkForUpdateButton

    // Navigation on this page runs through the QML focus chain below (release notes -> buttons ->
    // switches). It must not also scroll the Flickable from a DPAD_UP/DOWN handler: the input
    // controller does not consume key events, so both would act on the same key press.
    Component.onCompleted: {
        SoftwareUpdate.checkForUpdate(false);
    }

    Timer {
        repeat: true
        interval: 3000
        running: true

        onTriggered: SoftwareUpdate.checkForUpdate(false, true)
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        boundsBehavior: Flickable.StopAtBounds

        Behavior on contentY {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 500 }
        }

        ColumnLayout {
            id: content
            spacing: 0
            width: parent.width

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 20
                Layout.bottomMargin: 20
                wrapMode: Text.WordWrap
                color: colors.textPrimary
                text: SoftwareUpdate.updateAvailable ? qsTr("New software version is available") : qsTr("Your software is up to date")
                horizontalAlignment: Text.AlignHCenter
                font: fonts.label()
            }

            Components.KeyValueRow {
                id: currentVersion
                //: Current software version
                key: qsTr("Current version")
                value: SoftwareUpdate.currentVersion
                showDivider: SoftwareUpdate.updateAvailable
            }

            Components.KeyValueRow {
                id: newVersion
                visible: SoftwareUpdate.updateAvailable
                //: New software version
                key: qsTr("New version")
                value: SoftwareUpdate.newVersion
                showDivider: false
            }

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                visible: SoftwareUpdate.updateAvailable
                wrapMode: Text.WordWrap
                color: colors.textSecondary
                //: Software update download state
                text: {
                    switch (SoftwareUpdate.updateDownloadState) {
                    case SoftwareUpdate.Pending:
                        return qsTr("Pending");
                    case SoftwareUpdate.Downloading:
                        return qsTr("Downloading");
                    case SoftwareUpdate.Downloaded:
                        return qsTr("Downloaded");
                    case SoftwareUpdate.Error:
                        return qsTr("Error");
                    }
                }

                font: fonts.help()
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 10
                Layout.preferredHeight: 10
                visible: SoftwareUpdate.updateDownloadState === SoftwareUpdate.Downloading
                color: colors.surfaceRaised
                radius: height / 2

                Rectangle {
                    width: parent.width * SoftwareUpdate.downloadProgress / 100
                    height: parent.height
                    color: colors.textPrimary
                    radius: parent.radius
                    anchors.left: parent.left

                    Behavior on width {
                        NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 10
                horizontalAlignment: Text.AlignHCenter
                color: colors.textPrimary
                font: fonts.value()
                text: SoftwareUpdate.downloadProgress + "%"
                visible: SoftwareUpdate.updateDownloadState === SoftwareUpdate.Downloading
            }

            Components.MenuRow {
                id: releaseNotesRow
                Layout.topMargin: 10
                visible: SoftwareUpdate.updateAvailable
                text: qsTr("Release notes")
                chevron: true
                selected: activeFocus

                function openReleaseNotes() {
                    parentSwipeView.thirdPage.setSource("qrc:/settings/softwareupdate/ReleaseNotes.qml", { parentSwipeView: profileRoot, topNavigationText: qsTr("Release Notes") });

                    parentSwipeView.thirdPage.active = true;
                    settingsSwipeView.incrementCurrentIndex();
                }

                onClicked: {
                    releaseNotesRow.openReleaseNotes();
                }

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.down: installButton
                Keys.onReturnPressed: {
                    releaseNotesRow.openReleaseNotes();
                    event.accepted = true;
                }
            }

            Components.Button {
                id: installButton
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 20
                text: SoftwareUpdate.updateDownloadState === SoftwareUpdate.Downloaded ? qsTr("Install") : qsTr("Download")
                visible: SoftwareUpdate.updateAvailable
                enabled: SoftwareUpdate.updateDownloadState !== SoftwareUpdate.Downloading

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: releaseNotesRow
                KeyNavigation.down: checkForUpdateButton

                trigger: function() {
                    if (Battery.level > 50) {
                        SoftwareUpdate.startUpdate();
                        SoftwareUpdate.checkForUpdate(false);
                    } else {
                        ui.createActionableWarningNotification(qsTr("Low battery"), qsTr("Minimum 50% battery charge is required to install software updates"), "uc:battery-low");
                    }
                }
            }

            Components.Button {
                id: checkForUpdateButton
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.topMargin: 20
                Layout.bottomMargin: 20
                // next to the install button this is the alternative action
                variant: SoftwareUpdate.updateAvailable ? "secondary" : "primary"
                text: qsTr("Check for update")
                visible: SoftwareUpdate.updateDownloadState !== SoftwareUpdate.Downloading

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: installButton
                KeyNavigation.down: checkForUpdatesSwitch

                trigger: function() {
                    SoftwareUpdate.checkForUpdate(true);
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 20
                spacing: 10
                visible: Config.updateChannel == "TESTING";

                Text {
                    Layout.alignment: Qt.AlignLeft
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                    color: colors.textSecondary
                    text: qsTr("Beta updates")
                    font: fonts.help()
                }

                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    wrapMode: Text.NoWrap
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                    color: colors.textSecondary
                    text: qsTr("Enabled")
                    font: fonts.help()
                }
            }

            Components.Divider {}

            Components.SettingRow {
                //: Title for indication of checking for software updates are enabled
                title: qsTr("Check for updates")
                help: qsTr("Automatically check for updates.")
                showDivider: Config.checkForUpdates

                Components.Switch {
                    id: checkForUpdatesSwitch
                    icon: "uc:check"
                    checked: Config.checkForUpdates
                    trigger: function() {
                        Config.checkForUpdates = !Config.checkForUpdates;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: checkForUpdateButton
                    KeyNavigation.down: autoUpdateSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                visible: Config.checkForUpdates
                //: Title for indication of automatic software update is enabled
                title: qsTr("Auto update")
                help: qsTr("Automatically update the remote when new software is available. Updates are installed between %1 and %2").arg(Config.otaWindowStart).arg(Config.otaWindowEnd)
                showDivider: false

                Components.Switch {
                    id: autoUpdateSwitch
                    icon: "uc:check"
                    checked: Config.autoUpdate
                    trigger: function() {
                        Config.autoUpdate = !Config.autoUpdate;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: checkForUpdatesSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }
        }
    }
}
