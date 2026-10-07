// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import SoftwareUpdate 1.0
import Haptic 1.0
import Wifi 1.0
import Config 1.0
import Integration.Controller 1.0
import Dock.Controller 1.0

import "qrc:/components" as Components

Item {
    id: profileRoot
    width: parent.width; height: parent.height
    anchors.centerIn: parent

    signal closed

    property alias profileRoot: profileRoot
    property alias buttonNavigation: buttonNavigation
    property alias settingsSwipeView: settingsSwipeView
    property alias closeAnimation: closeAnimation
    property alias thirdPage: thirdPage

    function open() {
        openAnimation.start();
        buttonNavigation.takeControl();
    }

    function close() {
        closeAnimation.start();
    }

    function loadPage(page) {
        if (page === "") {
            return;
        }

        let p = menuModel.get(page).page;
        let url;

        switch (p) {
        case "software":
            url = "qrc:/settings/SoftwareUpdate.qml";
            break;
        case "settings":
            url = "qrc:/settings/Settings.qml";
            break;
        case "integration":
            url = "qrc:/settings/Integrations.qml";
            break;
        case "docks":
            url = "qrc:/settings/Docks.qml";
            break;
        case "activities":
            url = "qrc:/settings/Activities.qml";
            break;
        case "remotes":
            url = "qrc:/settings/Remotes.qml";
            break;
        case "about":
            url = "qrc:/settings/About.qml";
            break;
        }

        secondPage.setSource(url, { parentSwipeView: profileRoot, topNavigationText: Qt.binding(function(){ return qsTr(menuModel.get(page).name); }) });

        secondPage.active = true;
        settingsSwipeView.currentIndex = 1;
    }

    function goBack() {
        settingsSwipeView.decrementCurrentIndex();
    }

    function goHome() {
        closeAnimation.start();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        // keeps the keyboard focus inside the settings overlay, so the screen behind it stays quiet
        manageFocus: true
        defaultConfig: {
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
            },
            "BACK": {
                "pressed": function() {
                    if (profileRoot.state == "showLargeQr") {
                        profileRoot.state = "";
                    } else {
                        goHome();
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    if (profileRoot.state == "showLargeQr") {
                        profileRoot.state = "";
                    } else {
                        goHome();
                    }
                }
            }
        }
    }

    ListModel {
        id: menuModel

        ListElement {
            pos: 0
            name: QT_TR_NOOP("Software update")
            page: "software"
        }

        ListElement {
            pos: 1
            name: QT_TR_NOOP("Settings")
            page: "settings"
        }

        ListElement {
            pos: 2
            name: QT_TR_NOOP("Integrations")
            page: "integration"
        }

        ListElement {
            pos: 3
            name: QT_TR_NOOP("Docks")
            page: "docks"
        }

        //        ListElement {
        //            pos: 4
        //            name: QT_TR_NOOP("Activities & macros")
        //            page: "activities"
        //        }

        //        ListElement {
        //            pos: 5
        //            name: QT_TR_NOOP("Remotes")
        //            page: "docks"
        //        }

        ListElement {
            pos: 6
            name: QT_TR_NOOP("About")
            page: "about"
        }
    }

    Rectangle {
        id: iconBg
        width: 28; height: 28
        radius: 14
        color: colors.black
        anchors { top: parent.top; topMargin: 6; right: parent.right }
    }

    Text {
        id: iconText
        color: colors.offwhite
        text: ui.profile.name.substring(0,1)
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: iconBg
        font: fonts.caption()
    }

    MouseArea {
        anchors.fill: parent
    }

    SwipeView {
        id: settingsSwipeView
        width: parent.width; height: parent.height
        anchors.centerIn: parent
        interactive: false
        opacity: 0

        onCurrentIndexChanged: if (settingsSwipeView.currentIndex == 0) {
                                   profileRoot.buttonNavigation.takeControl();
                               } else if (settingsSwipeView.currentIndex == 1) {
                                   if (secondPage.item) {
                                       secondPage.item.buttonNavigation.takeControl();
                                   }
                               }

        // PAGE 1
        // Settings level 0
        ColumnLayout {
            spacing: 0

            Components.TitleBar {
                text: qsTr("Settings")
                goBack: function() {
                    closeAnimation.start();
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 20

                color: colors.transparent
                border { color: colors.divider; width: 2 }
                radius: ui.cornerRadiusSmall
                visible: ui.profile.restricted

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    // restricted
                    Item {
                        Layout.alignment: Qt.AlignBottom
                        Layout.preferredHeight: 30
                        Layout.margins: 20

                        visible: ui.profile.restricted

                        Components.Icon {
                            id: lockIcon
                            icon: "uc:lock"
                            color: colors.textSecondary
                            anchors { left: parent.left }
                            size: 30
                        }

                        Text {
                            color: colors.textSecondary
                            //: Text explaining that the profile has restricted access
                            text: qsTr("Restricted")
                            anchors { left: lockIcon.right; leftMargin: 10; verticalCenter: lockIcon.verticalCenter }
                            font: fonts.help()
                        }
                    }
                }
            }

            ListView {
                id: menu

                Layout.fillWidth: true
                Layout.fillHeight: !ui.profile.restricted
                Layout.preferredHeight: 80

                maximumFlickVelocity: 6000
                flickDeceleration: 1000
                highlightMoveDuration: 200
                clip: true
                interactive: !ui.profile.restricted
                pressDelay: 200

                model: menuModel
                delegate: menuItem

                onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

                Components.ScrollIndicator {
                    parent: menu
                    parentObj: menu
                }
            }
        }

        // PAGE 2
        // Settings level 1
        Item {
            Loader {
                id: secondPage
                width: parent.width; height: settingsSwipeView.height;
                anchors.centerIn: parent
                asynchronous: true
                active: false
                onLoaded: secondPage.item.buttonNavigation.takeControl()
            }
        }

        // PAGE 3
        // Settings level 2
        Item {
            Loader {
                id: thirdPage
                width: parent.width; height: settingsSwipeView.height;
                anchors.centerIn: parent
                asynchronous: true
                active: false
                onLoaded: thirdPage.item.buttonNavigation.takeControl()
            }
        }
    }

    Rectangle {
        id: largeQrContainer
        anchors.fill: parent
        color: colors.black
        opacity: 0
        enabled: opacity == 1

        MouseArea {
            anchors.fill: parent
            onClicked: profileRoot.state = "";
        }

        Text {
            width: parent.width - 40
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            horizontalAlignment: Text.AlignHCenter
            color: colors.textSecondary
            text: qsTr("Scan to open\nthe Web Configurator")
            font: fonts.help()
            anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }
        }

        Text {
            width: parent.width - 40
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            horizontalAlignment: Text.AlignHCenter
            color: colors.textSecondary
            text: qsTr("Tap to close")
            font: fonts.help()
            anchors { bottom: parent.bottom; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }
        }
    }

    Components.ProfileSwitch {
        id: profileSwitch
    }

    Component {
        id: menuItem

        Components.MenuRow {
            width: ListView.view.width
            height: visible ? 80 : 0
            // a restricted profile only sees About
            visible: pos === 6 || !ui.profile.restricted
            text: qsTr(name)
            chevron: true
            selected: ListView.isCurrentItem
            badgeAlert: pos === 0
            badge: {
                switch (pos) {
                case 0:
                    return SoftwareUpdate.updateAvailable ? "1" : "";
                case 2:
                    return IntegrationController.integrationsModel.count > 0
                            ? String(IntegrationController.integrationsModel.count) : "";
                case 3:
                    return DockController.configuredDocks.count > 0 ? String(DockController.configuredDocks.count) : "";
                default:
                    return "";
                }
            }

            onClicked: {
                menu.currentIndex = index;
                loadPage(menu.currentIndex);
            }
        }
    }

    SequentialAnimation {
        id: openAnimation
        running: false

        ParallelAnimation {
            PropertyAnimation { target: iconText; properties: "opacity"; to: 0; easing.type: Easing.InExpo; duration: 200 }
            PropertyAnimation { target: iconBg; properties: "scale"; to: 150; easing.type: Easing.InExpo; duration: 200 }
        }
        PropertyAnimation { target: settingsSwipeView; properties: "opacity"; to: 1; easing.type: Easing.OutExpo; duration: 200 }
    }

    ParallelAnimation {
        id: closeAnimation
        running: false

        PropertyAnimation { target: settingsSwipeView; properties: "opacity"; to: 0; easing.type: Easing.OutExpo; duration: 200 }
        PropertyAnimation { target: iconText; properties: "opacity"; to: 1; easing.type: Easing.OutExpo; duration: 200 }
        PropertyAnimation { target: iconBg; properties: "scale"; to: 1; easing.type: Easing.OutExpo; duration: 200 }
    }

    states: State {
        name: "showLargeQr"

        PropertyChanges { target: largeQrContainer; opacity: 1 }
        ParentChange { target: qrCode; parent: largeQrContainer; x: 20; y: (ui.height - ui.width - 40) / 2 + 40; width: ui.width - 40; height: ui.width - 40 }
    }

    transitions: Transition {
        to: "showLargeQr"
        from: ""
        reversible: true

        ParallelAnimation {
            ParentAnimation {
                NumberAnimation { properties: "x, y, width, height"; easing.type: Easing.OutExpo; duration: 200 }
            }
            PropertyAnimation { target: largeQrContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 200 }
        }
    }

    Connections {
        target: closeAnimation

        function onFinished() {
            buttonNavigation.releaseControl();
            closed();
        }
    }

    Component.onCompleted: {
        Wifi.getWifiStatus();

        if (ui.profile.restricted) {
            menuModel.remove(0);
            menuModel.remove(0);
            menuModel.remove(0);
            menuModel.remove(0);
            //            menuModel.remove(0);
            //            menuModel.remove(0);
        }
    }
}
