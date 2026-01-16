// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Integration.Controller 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components
import "qrc:/components/integrations" as Integrations

Settings.Page {
    id: integrationsPage

    function loadIntegrationInfo(integrationId) {
        integrationDetailPopupInfo.integrationId = integrationId;
        integrationDetailPopup.open();
    }

    Component.onCompleted: {
        IntegrationController.getAllIntegrationDrivers();
        IntegrationController.getAllIntegrations();

        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         itemList.incrementCurrentIndex();
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         itemList.decrementCurrentIndex();
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         loadIntegrationInfo(itemList.currentItem.key);
                                                     }
                                                 }
                                             });
    }

    ListView {
        id: itemList
        width: parent.width
        anchors { horizontalCenter: parent.horizontalCenter; top: topNavigation.bottom; topMargin: 20; bottom: addIntegrationSheet.top; bottomMargin: 20 }
        clip: true
        spacing: 20

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        pressDelay: 200

        model: IntegrationController.integrationsModel

        delegate: listItem
        currentIndex: 0
    }

    Components.ScrollIndicator {
        parentObj: itemList
        hideOverride: itemList.atYEnd
    }

    Components.BottomSheet {
        id: addIntegrationSheet
        titleOpened: qsTr("Add an integration")
        titleClosed: qsTr("Add an integration")
        openItemSource: "qrc:/components/integrations/Discovery.qml"

        onOpened: {
            IntegrationController.startDriverDiscovery();
            addIntegrationSheet.openItem.buttonNavigation.overrideActive = true;
        }

        onClosed: {
            IntegrationController.stopDriverDiscovery();
            integrationsPage.buttonNavigation.takeControl();
        }

        Connections {
            target: IntegrationController
            ignoreUnknownSignals: true

            function onIntegrationSetupChange(driverId, state, error, data) {
                if (state === IntegrationControllerEnums.Ok) {
                    addIntegrationSheet.state = "closed";
                }
            }
        }
    }

    Popup {
        id: integrationDetailPopup
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0

        onOpened: {
            integrationDetailPopupInfo.buttonNavigation.takeControl();
        }

        onClosed: {
            integrationDetailPopupInfo.reset();
            integrationDetailPopupInfo.buttonNavigation.releaseControl()
        }

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.7; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        exit: Transition {
            NumberAnimation { property: "scale"; from: 1.0; to: 0.7; easing.type: Easing.InExpo; duration: 300 }
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 300 }
        }

        background: Rectangle { color: colors.black; }
        contentItem: Integrations.Info {
            id: integrationDetailPopupInfo
            popup: integrationDetailPopup
        }
    }

    Popup {
        id: integrationSetupPopup
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0
        parent: Overlay.overlay

        onOpened: {
            integrationSetupPopupButtonNavigation.takeControl();
        }

        onClosed: {
            integrationSetupPopupButtonNavigation.releaseControl();
        }

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.7; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 400 }
        }

        exit: Transition {
            SequentialAnimation {
                ParallelAnimation {
                    NumberAnimation { property: "scale"; from: 1.0; to: 0.7; easing.type: Easing.InExpo; duration: 300 }
                    NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 400 }
                }
                ScriptAction { script: integrationSetupLoader.active = false; }
            }
        }

        background: Rectangle { color: colors.black }

        contentItem: Loader {
            id: integrationSetupLoader
            active: false
            asynchronous: true
            source: "qrc:/components/integrations/Setup.qml"

            Connections {
                target: integrationSetupLoader.item
                ignoreUnknownSignals: true

                function onDone() {
                    integrationSetupPopup.close();
                }
            }

            Behavior on y {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
            }
        }

        Connections {
            target: IntegrationController
            ignoreUnknownSignals: true

            // if an integration is selected for setup, we open the popup
            function onIntegrationDriverToSetupChanged() {
                if (IntegrationController.integrationDriverTosetup) {
                    integrationSetupLoader.active = true;
                    integrationSetupPopup.open();
                }
            }
        }

        Components.ButtonNavigation {
            id: integrationSetupPopupButtonNavigation
            defaultConfig: {
                "HOME": {
                    "pressed": function() {
                        integrationSetupPopup.close();
                        goHome();
                    }
                },
                "BACK": {
                    "pressed": function() {
                        integrationSetupPopup.close();
                    }
                }
            }
        }
    }

    Component {
        id: listItem

        Rectangle {
            width: ListView.view.width
            height: mainColumnLayout.height
            color: isCurrentItem && ui.keyNavigationEnabled ? Qt.darker(colors.dark, 1.5) : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: isCurrentItem && ui.keyNavigationEnabled ? colors.medium : colors.transparent
                width: 1
            }

            property bool isCurrentItem: ListView.isCurrentItem
            property string key: integrationId
            property bool selected: selected

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    itemList.currentIndex = index;
                    integrationsPage.loadIntegrationInfo(key);
                }
            }

            ColumnLayout {
                id: mainColumnLayout
                width: parent.width
                spacing: 0

                RowLayout {
                    spacing: 20
                    Layout.topMargin: 20
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20

                    Rectangle {
                        Layout.alignment: Qt.AlignTop

                        width: 80
                        height: width
                        radius: 40
                        color: integrationIcon.includes("uc") ? colors.offwhite : colors.transparent

                        Components.Icon {
                            color: colors.black
                            icon: integrationIcon === "" ? "uc:puzzle" : integrationIcon
                            size: 80
                            anchors.centerIn: parent
                        }
                    }

                    ColumnLayout {
                        spacing: 0

                        Text {
                            Layout.fillWidth: true

                            text: integrationName
                            wrapMode: Text.WordWrap
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            color: colors.offwhite
                            font: fonts.primaryFont(30)
                        }

                        Text {
                            Layout.fillWidth: true

                            text: integrationState
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            color: colors.light
                            font: fonts.secondaryFont(24)
                        }
                    }
                }

                RowLayout {
                    spacing: 10
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 10
                    Layout.bottomMargin: 20

                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft

                        text: qsTr("Version: ") + IntegrationController.getDriversModelItem(driverId).version
                        wrapMode: Text.WordWrap
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        color: colors.light
                        font: fonts.secondaryFont(20)
                    }

                    Components.Icon {
                        icon: "uc:globe"
                        size: 40
                        color: colors.light
                        visible: IntegrationController.getDriversModelItem(driverId).external
                    }

                    Components.Switch {
                        Layout.alignment: Qt.AlignRight

                        icon: "uc:check"
                        checked: integrationState == "connected"
                        trigger: function() {
                            if (integrationState == "connected") {
                                IntegrationController.integrationDisconnect(key);
                            } else if (integrationState == "disconnected" || integrationState == "error") {
                                IntegrationController.integrationConnect(key);
                            }
                        }
                        enabled: integrationState == "connected" || integrationState == "disconnected" || integrationState == "error"
                        opacity: enabled ? 1 : 0.3
                    }
                }
            }
        }
    }
}
