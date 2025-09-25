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

    buttonNavigation.enabled: ui.showRegulatoryInfo

    Flow {
        width: parent.width
        anchors { top: topNavigation.bottom }

        Loader {
            sourceComponent: aboutInfo
            onLoaded: {
                item.title = qsTr("Model number");
                item.value = HwInfo.modelNumber
            }
        }

        Loader {
            sourceComponent: aboutInfo
            onLoaded: {
                item.title = qsTr("Serial number");
                item.value = HwInfo.serialNumber
            }
        }

        Loader {
            sourceComponent: aboutInfo
            onLoaded: {
                item.title = qsTr("Revision");
                item.value = HwInfo.revision
            }
        }

        Loader {
            sourceComponent: aboutInfo
            onLoaded: {
                item.title = qsTr("Wi-Fi address");
                item.value = Wifi.macAddress
            }
        }

        Loader {
            sourceComponent: aboutInfo
            onLoaded: {
                item.title = qsTr("Bluetooth address");
                item.value = Config.bluetoothMac;
            }
        }

        Loader {
            sourceComponent: aboutInfo
            onLoaded: {
                item.title = qsTr("UI version")
                item.value = SoftwareUpdate.uiVersion
            }
        }

        Loader {
            sourceComponent: aboutInfo
            onLoaded: {
                item.title = qsTr("Core version")
                item.value = SoftwareUpdate.coreVersion
            }
        }

        Loader {
            sourceComponent: aboutInfo
            onLoaded: {
                item.title = qsTr("System version")
                item.value = SoftwareUpdate.currentVersion
                item.bottomLine.visible = false;
            }
        }

        Item {
            width: ui.width
            height: 40
        }

        ListView {
            id: menu
            width: parent.width; height: childrenRect.height

            interactive: false
            highlightMoveDuration: 200
            pressDelay: 200

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

            delegate: menuItem
        }
    }

    Component {
        id: aboutInfo

        ColumnLayout {
            width: ui.width - 20
            spacing: 10
            x: 10

            property alias title: title.text
            property alias value: value.text
            property alias bottomLine: bottomLine

            RowLayout {
                Layout.topMargin: 10
                width: parent.width
                spacing: 20

                Text {
                    id: title
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                    color: colors.offwhite
                    font: fonts.primaryFont(20)
                }

                Text {
                    id: value
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignRight
                    wrapMode: Text.NoWrap
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                    color: colors.offwhite
                    opacity: 0.7
                    font: fonts.secondaryFont(20)
                }
            }

            Rectangle {
                id: bottomLine
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: colors.medium
            }
        }
    }

    Component {
        id: menuItem

        Rectangle {
            width: ui.width
            height: 80
            color: ListView.isCurrentItem && ui.keyNavigationEnabled ? colors.dark : colors.transparent
            radius: ui.cornerRadiusSmall

            Text {
                id: menuItemText
                color: colors.offwhite
                text: modelData.itemTitle
                anchors { left: parent.left; leftMargin: 10; right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter; }
                font: fonts.primaryFont(30)
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    menu.currentIndex = index;
                    loadPage(index);
                }
            }
        }
    }
}
