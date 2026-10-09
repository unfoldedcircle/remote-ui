// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0
import Config 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: settingsPage

    function loadPage(page) {
        parentSwipeView.thirdPage.setSource("qrc:/settings/settings/" + menu.model[page].page + ".qml", { parentSwipeView: profileRoot, topNavigationText: Qt.binding(function(){ return qsTr(menu.model[page].itemTitle); }) });

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

    Flow {
        width: parent.width
        anchors { top: topNavigation.bottom; bottom: parent.bottom }

        ListView {
            id: menu
            width: parent.width; height: parent.height
            clip: true

            interactive: true
            boundsBehavior: Flickable.StopAtBounds
            highlightMoveDuration: 200
            // keep the wheel-selected item within the viewport so the list scrolls with navigation
            highlightRangeMode: ListView.ApplyRange
            preferredHighlightBegin: height * 0.15
            preferredHighlightEnd: height * 0.85

            model: [
                {
                    itemTitle: QT_TR_NOOP("Display & Brightness"),
                    page: "Display",
                    icon: "uc:tv"
                },
                {
                    itemTitle: QT_TR_NOOP("User interface"),
                    page: "Ui",
                    icon: "uc:list"
                },
                {
                    //: Settings page for the touch-sensitive strip on the side of the remote.
                    itemTitle: QT_TR_NOOP("Touch Slider"),
                    page: "TouchSlider",
                    icon: "uc:sliders"
                },
                //                {
                //                    itemTitle: QT_TR_NOOP("Colors"),
                //                    page: "Color",
                //                    icon: "uc:color"
                //                },
                {
                    itemTitle: QT_TR_NOOP("Sound & Haptic"),
                    page: "Sound",
                    icon: "uc:volume"
                },
                {
                    itemTitle: QT_TR_NOOP("Voice Control"),
                    page: "Voice",
                    icon: "uc:microphone"
                },
                {
                    itemTitle: QT_TR_NOOP("Power Saving"),
                    page: "Power",
                    icon: "uc:battery-full"
                },
                {
                    itemTitle: QT_TR_NOOP("Wifi & Bluetooth"),
                    page: "Wifi",
                    icon: "uc:wifi"
                },
                {
                    //: Settings page for the language, country, time zone, units and clock format of the remote.
                    itemTitle: QT_TR_NOOP("Localization"),
                    page: "Localisation",
                    icon: "uc:language"
                },
                {
                    itemTitle: QT_TR_NOOP("Administrator PIN"),
                    page: "AdminPin",
                    icon: "uc:lock"
                },
                {
                    itemTitle: QT_TR_NOOP("Factory reset"),
                    page: "Reset",
                    icon: "uc:triangle-exclamation"
                }
            ]

            delegate: menuItem

            Components.ScrollIndicator {
                parent: menu
                parentObj: menu
            }
        }
    }

    Component {
        id: menuItem

        Components.MenuRow {
            width: ListView.view.width
            icon: menu.model[index].icon
            text: qsTr(menu.model[index].itemTitle)
            chevron: true
            selected: ListView.isCurrentItem

            onClicked: {
                menu.currentIndex = index;
                loadPage(index);
            }
        }
    }
}
