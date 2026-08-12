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
            Layout.alignment: Qt.AlignCenter
            spacing: 10
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            Item {
                height: 20
            }

            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height + 20

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: SoftwareUpdate.updateAvailable ? qsTr("New software version is available") : qsTr("Your software is up to date")
                    horizontalAlignment: Text.AlignHCenter
                    font: fonts.primaryFont(30)
                }
            }

            Item {
                height: 20
            }

            RowLayout {
                id: currentVersion
                Layout.alignment: Qt.AlignCenter
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                Text {
                    Layout.alignment: Qt.AlignLeft
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                    color: colors.light
                    //: Current software version
                    text: qsTr("Current version")
                    font: fonts.primaryFont(20)
                }

                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    wrapMode: Text.NoWrap
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                    color: colors.light
                    text: SoftwareUpdate.currentVersion
                    font: fonts.secondaryFont(20)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
                visible: SoftwareUpdate.updateAvailable
            }

            RowLayout {
                id: newVersion
                Layout.alignment: Qt.AlignCenter
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10
                visible: SoftwareUpdate.updateAvailable

                Text {
                    Layout.alignment: Qt.AlignLeft
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                    color: colors.offwhite
                    //: New software version
                    text: qsTr("New version")
                    font: fonts.primaryFont(20)
                }

                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    wrapMode: Text.NoWrap
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                    color: colors.offwhite
                    text: SoftwareUpdate.newVersion
                    font: fonts.secondaryFont(20)
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10
                visible: SoftwareUpdate.updateAvailable

                Text {
                    Layout.alignment: Qt.AlignLeft
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                    color: colors.light
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

                    font: fonts.primaryFont(20)
                }
            }

            Item {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                width: parent.width - 20
                height: 10
                visible: SoftwareUpdate.updateDownloadState === SoftwareUpdate.Downloading

                Rectangle {
                    width: parent.width
                    height: parent.height
                    color: colors.dark
                    radius: 5

                    Rectangle {
                        width: parent.width * SoftwareUpdate.downloadProgress / 100
                        height: parent.height
                        color: colors.offwhite
                        radius: parent.radius
                        anchors.left: parent.left

                        Behavior on width {
                            NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
                        }
                    }
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                horizontalAlignment: Text.AlignHCenter
                color: colors.light
                font: fonts.secondaryFont(20)
                text: SoftwareUpdate.downloadProgress + "%"
                visible: SoftwareUpdate.updateDownloadState === SoftwareUpdate.Downloading
            }

            Item {
                height: 20
            }

            Components.HapticMouseArea {
                id: releaseNotesRow
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: releaseNoteText.height
                visible: SoftwareUpdate.updateAvailable

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

                Rectangle {
                    anchors { fill: parent; margins: -6 }
                    color: colors.transparent
                    radius: ui.cornerRadiusSmall
                    border {
                        width: 2
                        color: releaseNotesRow.activeFocus && ui.keyNavigationEnabled ? colors.highlight
                                                                                      : colors.transparent
                    }
                }

                Text {
                    id: releaseNoteText
                    width: parent.width / 2
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Release notes")
                    anchors { left: parent.left }
                    font: fonts.primaryFont(20)
                }

                Components.Icon {
                    icon: "uc:arrow-right"
                    size: 40
                    color: colors.offwhite
                    anchors { right: parent.right; verticalCenter: releaseNoteText.verticalCenter }
                }
            }

            Item {
                height: 30
            }

            Components.Button {
                id: installButton
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                text: SoftwareUpdate.updateDownloadState === SoftwareUpdate.Downloaded ? qsTr("Install") : qsTr("Download")
                visible: SoftwareUpdate.updateAvailable
                enabled: SoftwareUpdate.updateDownloadState !== SoftwareUpdate.Downloading
                opacity: enabled ? 1 : 0.3

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

            Item {
                height: 20
            }

            Components.Button {
                id: checkForUpdateButton
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                text: qsTr("Check for update")
                visible: SoftwareUpdate.updateDownloadState !== SoftwareUpdate.Downloading

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: installButton
                KeyNavigation.down: checkForUpdatesSwitch

                trigger: function() {
                    SoftwareUpdate.checkForUpdate(true);
                }
            }

            Item {
                height: 10
            }

            RowLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10
                visible: Config.updateChannel == "TESTING";

                Text {
                    Layout.alignment: Qt.AlignLeft
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                    color: colors.light
                    text: qsTr("Beta updates")
                    font: fonts.primaryFont(20)
                }

                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    wrapMode: Text.NoWrap
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                    color: colors.light
                    text: qsTr("Enabled")
                    font: fonts.secondaryFont(20)
                }
            }

            Item {
                height: 30
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: checkForUpdatesText

                        Layout.fillWidth: true

                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        //: Title for indication of checking for software updates are enabled
                        text: qsTr("Check for updates")
                        font: fonts.primaryFont(30)
                    }

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
                        highlight: activeFocus && ui.keyNavigationEnabled
                    }
                }

                Text {
                    Layout.fillWidth: true

                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Automatically check for updates.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.topMargin: 20
                Layout.bottomMargin: 20
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
                visible: Config.checkForUpdates
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                visible: Config.checkForUpdates
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: autoUpdateText

                        Layout.fillWidth: true

                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        //: Title for indication of automatic software update is enabled
                        text: qsTr("Auto update")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: autoUpdateSwitch
                        icon: "uc:check"
                        checked: Config.autoUpdate
                        trigger: function() {
                            Config.autoUpdate = !Config.autoUpdate;
                        }

                        /** KEYBOARD NAVIGATION **/
                        KeyNavigation.up: checkForUpdatesSwitch
                        highlight: activeFocus && ui.keyNavigationEnabled
                    }
                }

                Text {
                    Layout.fillWidth: true

                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Automatically update the remote when new software is available. Updates are installed between %1 and %2").arg(Config.otaWindowStart).arg(Config.otaWindowEnd)
                    font: fonts.secondaryFont(24)
                }
            }

            Item {
                height: 20
            }
        }
    }
}
