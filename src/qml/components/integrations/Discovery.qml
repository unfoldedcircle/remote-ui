// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Integration.Controller 1.0

import "qrc:/components" as Components

ListView {
    id: integrationList

    anchors.fill: parent
    clip: true
    spacing: 20
    model: IntegrationController.discoveredIntegrationDrivers
    delegate: integrationItem
    header: headerItem
    footer: footerItem

    add: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 300; easing.type: Easing.OutExpo }
    }

    remove: Transition {
        NumberAnimation { properties: "x"; to: ui.width; duration: 300; easing.type: Easing.OutBounce }
    }

    populate: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 300; easing.type: Easing.OutExpo }
    }

    displaced: Transition {
        NumberAnimation { properties: "x"; to: ui.width; duration: 300; easing.type: Easing.OutBounce }
    }

    property alias buttonNavigation: buttonNavigation

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    IntegrationController.stopDriverDiscovery();
                }
            },
            "HOME": {
                "pressed": function() {
                    IntegrationController.stopDriverDiscovery();
                }
            }
        }
    }

    Components.ScrollIndicator {
        hideOverride: integrationList.atYEnd
    }

    Component {
        id: headerItem

        Item {
            width: ListView.view.width
            height: 100

            Components.HapticMouseArea {
                width: childrenRect.width
                height: 70
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

                onClicked: {
                    IntegrationController.startDriverDiscovery();
                    console.debug("CLick");
                }

                Text {
                    id: headerTitle

                    color: colors.offwhite
                    opacity: 0.6
                    //: Title for searching for integrations to setup
                    text: qsTr("Discovering")
                    verticalAlignment: Text.AlignVCenter
                    anchors { left: scanLoading.right; leftMargin: 20; verticalCenter: parent.verticalCenter }
                    font: fonts.secondaryFont(24)
                }

                Image {
                    id: scanLoading

                    visible: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/images/loader_small.png"
                    anchors { left: parent.left; leftMargin: scanLoading.visible ? 0 : -scanLoading.width/2 - 10; verticalCenter: parent.verticalCenter }

                    RotationAnimation on rotation {
                        running: visible
                        loops: Animation.Infinite
                        from: 0; to: 360
                        duration: 2000
                    }
                }
            }

            Connections {
                target: IntegrationController
                ignoreUnknownSignals: true

                function onDriverDiscoveryStarted() {
                    scanLoading.visible = true;
                    headerTitle.text = qsTr("Discovering");
                }

                function onDriverDiscoveryStopped() {
                    scanLoading.visible = false;
                    headerTitle.text = qsTr("%1 integration(s) found").arg(IntegrationController.discoveredIntegrationDrivers.count);
                }
            }
        }
    }

    Component {
        id: footerItem

        Item {
            width: ListView.view.width
            height: footerItemText.implicitHeight + 40
            visible: integrationList.count > 0

            Text {
                id: footerItemText
                width: parent.width - 20
                color: colors.offwhite
                opacity: 0.6
                text: qsTr("Integrations may require the Web Configurator for setup.")
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                font: fonts.secondaryFont(24)
                anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }
            }

        }
    }

    Component {
        id: integrationItem

        Rectangle {
            id: integrationItemContainer

            x: 10
            width: ListView.view.width - 20
            height: childrenRect.height
            color: ListView.isCurrentItem ? colors.black : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                width: 1
                color: colors.medium
            }

            Components.Icon {
                icon: "uc:globe"
                size: 40
                color: colors.light
                anchors { top: parent.top; topMargin: 10; right: parent.right; rightMargin: 10 }
                visible: driverExternal
            }

            RowLayout {
                width: parent.width - 60
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 20

                Rectangle {
                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 60
                    Layout.topMargin: 30
                    Layout.bottomMargin: 30

                    radius: 30
                    color: colors.offwhite

                    Components.Icon {
                        icon: driverIcon
                        size: 60
                        color: colors.black
                        anchors.centerIn: parent
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60

                    Text {
                        Layout.fillWidth: true

                        color: colors.offwhite
                        text: driverName
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        font: fonts.primaryFont(30)
                    }

                    Text {
                        Layout.fillWidth: true

                        color: colors.light
                        //: Integration driver developer name
                        text: qsTr("By %1").arg(driverDeveloperName)
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        font: fonts.secondaryFont(22)
                    }
                }
            }

            Components.HapticMouseArea {
                anchors.fill: parent

                onClicked: {
                    IntegrationController.stopDriverDiscovery();                    
                    IntegrationController.selectIntegrationToSetup(driverId);
                }
            }
        }
    }
}
