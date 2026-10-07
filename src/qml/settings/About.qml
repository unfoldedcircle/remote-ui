// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
 
import HwInfo 1.0
import Wifi 1.0
import Haptic 1.0
import SoftwareUpdate 1.0
import Config 1.0
import ResourceTypes 1.0

import "qrc:/components" as Components
import "qrc:/settings" as Settings

Settings.Page {
    id: aboutPage
    scrollTarget: flickable

    function loadPage(page) {
        parentSwipeView.thirdPage.setSource(menu.model[page].page === ResourceTypes.Licenses ? "qrc:/settings/about/LicensePage.qml" : "qrc:/settings/about/AboutPage.qml", { parentSwipeView: profileRoot, topNavigationText: qsTr(menu.model[page].itemTitle), type: menu.model[page].page });
        parentSwipeView.thirdPage.active = true;
        settingsSwipeView.incrementCurrentIndex();
    }

    Component.onCompleted: {
        Config.getConfig();

        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         menu.incrementCurrentIndex();
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         menu.decrementCurrentIndex();
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         loadPage(menu.currentIndex);
                                                     }
                                                 }
                                             });
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: width; contentHeight: aboutColumn.implicitHeight
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        boundsBehavior: Flickable.StopAtBounds

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        Column {
            id: aboutColumn
            width: parent.width

            Components.KeyValueRow {
                key: qsTr("Model number")
                value: HwInfo.modelNumber
            }

            Components.KeyValueRow {
                key: qsTr("Serial number")
                value: HwInfo.serialNumber
            }

            Components.KeyValueRow {
                key: qsTr("Revision")
                value: HwInfo.revision
            }

            Components.KeyValueRow {
                key: qsTr("Wi-Fi address")
                value: Wifi.macAddress
            }

            Components.KeyValueRow {
                key: qsTr("Bluetooth address")
                value: Config.bluetoothMac
            }

            Components.KeyValueRow {
                key: qsTr("UI version")
                value: SoftwareUpdate.uiVersion
            }

            Components.KeyValueRow {
                key: qsTr("Core version")
                value: SoftwareUpdate.coreVersion
            }

            Components.KeyValueRow {
                key: qsTr("System version")
                value: SoftwareUpdate.currentVersion
                showDivider: false
            }

            Item {
                width: parent.width
                height: 20
            }

            ListView {
                id: menu
                width: parent.width; height: contentHeight

                interactive: false
                highlightMoveDuration: 200
                pressDelay: 200

                // the list is laid out at full height inside the page Flickable, so it never scrolls
                // itself - keep the keypad selection visible by scrolling the page instead
                onCurrentIndexChanged: aboutPage.ensureVisible(menu.currentItem)

                model: [
                    {
                        itemTitle: qsTr("Regulatory"),
                        page: ResourceTypes.Regulatory
                    },
                    {
                        itemTitle: qsTr("Terms & conditions"),
                        page: ResourceTypes.Terms
                    },
                    {
                        itemTitle: qsTr("Warranty information"),
                        page: ResourceTypes.Warranty
                    },
                    {
                        itemTitle: qsTr("Licenses"),
                        page: ResourceTypes.Licenses
                    }
                ]

                delegate: Components.MenuRow {
                    width: ListView.view.width
                    text: modelData.itemTitle
                    chevron: true
                    selected: ListView.isCurrentItem

                    onClicked: {
                        menu.currentIndex = index;
                        loadPage(index);
                    }
                }
            }
        }
    }
}
