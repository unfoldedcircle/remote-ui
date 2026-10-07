// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

Item {
    id: noPageRoot
    width: parent.width; height: parent.height

    property alias statusBar: statusBar

    Component.onCompleted: {
        buttonNavigation.takeControl();
        ui.inputController.setBaseOwner(noPageRoot);
    }

    function addPage() {
        pageAdd.state = "visible";
        keyboard.show();
    }

    // The menu of a HOME long press, as on the main screen. Without a page it only leads to the profile
    // page (the profile icon in the status bar has no key of its own) and the tips.
    function openMenu() {
        popupMenu.title = ui.profile.name;
        popupMenu.menuItems = [
                    {
                        title: ui.profile.restricted
                               //: Page menu entry of a restricted profile: opens the profile page (profile list and About)
                               ? qsTr("Profile")
                               //: Page menu entry: opens the profile page with the profile list, the web configurator and the settings
                               : qsTr("Profile & settings"),
                        icon: "uc:user",
                        callback: function() {
                            loadSecondContainer("qrc:/components/Profile.qml");
                        }
                    },
                    {
                        title: qsTr("Show tips"),
                        icon: "uc:circle-info",
                        callback: function() {
                            ui.showHelp = true;
                        }
                    }
                ];
        popupMenu.open();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (!ui.profile.restricted) {
                        // deferred: the dialog focuses its field, which must not receive this key press
                        Qt.callLater(noPageRoot.addPage);
                    }
                }
            },
            "HOME": {
                "long_press": function() {
                    noPageRoot.openMenu();
                }
            }
        }
    }

    Components.StatusBar {
        id: statusBar
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
    }

    Item {
        id: plusIcon
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -20
        visible: !ui.profile.restricted

        Rectangle {
            width: 60
            height: 1
            color: colors.offwhite
            anchors.centerIn: parent
        }

        Rectangle {
            width: 1
            height: 60
            color: colors.offwhite
            anchors.centerIn: parent
        }
    }

    Text {
        id: smallText
        color: colors.textPrimary
        text: ui.profile.restricted
              ? qsTr("No page found. Ask your administrator to setup pages.")
                //: Shown below a large "+" when the profile has no page yet; the "+" or OK adds one
              : qsTr("Add your first page")
        width: parent.width - 40
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors { horizontalCenter: parent.horizontalCenter; top: plusIcon.bottom; topMargin: 40 }
        font: fonts.prose()
        lineHeight: fonts.proseLineHeight
    }

    Components.HapticMouseArea {
        width: Math.min(smallText.implicitWidth, smallText.width) + 40
        height: plusIcon.height + smallText.height + 100
        anchors { top: plusIcon.top; topMargin: -50; horizontalCenter: parent.horizontalCenter }
        visible: !ui.profile.restricted

        // the screen's only action: OK triggers it, so the ring is on it whenever the keypad is in use
        Components.Selectable {
            selected: pageAdd.state !== "visible"
        }

        onClicked: {
            noPageRoot.addPage();
        }
    }

    Components.PageAdd {
        id: pageAdd
        anchors.centerIn: parent
    }

    Components.PopupMenu {
        id: popupMenu
    }

    Loader {
        anchors.fill: parent
        active: ui.showHelp
        // above the status bar
        z: 10
        asynchronous: true
        source: "qrc:/components/help-overlay/Main.qml"
    }
}
