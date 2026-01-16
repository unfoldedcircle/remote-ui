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
        font: fonts.secondaryFont(18)
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

            Components.Icon {
                id: closeIcon

                color: colors.offwhite
                icon: "uc:xmark"
                size: 80

                Components.HapticMouseArea {
                    width: 120; height: 120
                    anchors.centerIn: parent
                    onClicked: {
                        closeAnimation.start();
                    }
                }
            }

            Text {
                Layout.topMargin: 10
                Layout.leftMargin: 20
                Layout.fillWidth: true

                color: colors.light
                text: qsTr("Your current profile")
                font: fonts.secondaryFont(20)
            }

            Components.HapticMouseArea {
                Layout.fillWidth: true
                Layout.topMargin: 10
                Layout.leftMargin: 20
                Layout.rightMargin: 10
                Layout.preferredHeight: childrenRect.height

                onClicked: {
                    profileSwitch.state = "visible";
                }

                RowLayout {
                    width: parent.width

                    Components.Icon {
                        Layout.alignment: Qt.AlignVCenter

                        color: colors.offwhite
                        icon: ui.profile.icon
                        size: 80
                    }

                    Text {
                        id: profileNameText

                        Layout.fillWidth: true
                        Layout.leftMargin: 10
                        Layout.alignment: Qt.AlignVCenter

                        color: colors.offwhite
                        text: ui.profile.name
                        wrapMode: Text.NoWrap
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        font: fonts.primaryFont(50, "Light")
                        fontSizeMode: Text.Fit
                        minimumPixelSize: 30
                    }

                    Components.Icon {
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

                        color: colors.offwhite
                        icon: "uc:arrow-right"
                        size: 60
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: ui.profile.restricted || !Config.webConfiguratorEnabled
                Layout.preferredHeight: 20
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height
                Layout.bottomMargin: 20

                color: colors.transparent
                border { color: colors.medium; width: 2 }
                radius: ui.cornerRadiusSmall

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    ColumnLayout {
                        spacing: 0

                        Layout.alignment: Qt.AlignBottom
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.margins: 20

                        visible: !ui.profile.restricted

                        // web configurator enable
                        ColumnLayout {
                            Layout.alignment: Qt.AlignBottom
                            Layout.fillHeight: false
                            Layout.bottomMargin: Config.webConfiguratorEnabled ? 30 : 0

                            RowLayout {
                                Text {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                                    wrapMode: Text.WordWrap
                                    verticalAlignment: Text.AlignVCenter
                                    color: colors.light
                                    text: Config.webConfiguratorEnabled ? qsTr("Web configurator enabled") : qsTr("Web configurator disabled")
                                    font: fonts.secondaryFont(22)
                                }

                                Components.Switch {
                                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                                    icon: "uc:check"
                                    checked: Config.webConfiguratorEnabled
                                    trigger: function() {
                                        Config.webConfiguratorEnabled = !Config.webConfiguratorEnabled
                                    }
                                }
                            }

                            RowLayout {
                                visible: Config.webConfiguratorEnabled && Config.webConfiguratorAddress != ""

                                Text {
                                    id: webConfiguratorAddress

                                    property bool showIp: true

                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                                    wrapMode: Text.WordWrap
                                    verticalAlignment: Text.AlignVCenter
                                    color: colors.light
                                    text: ("http://%1/configurator").arg(webConfiguratorAddress.showIp ? Wifi.ipAddress : Config.webConfiguratorAddress)
                                    font: fonts.secondaryFont(22)

                                    Components.HapticMouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (Wifi.ipAddress) {
                                                webConfiguratorAddress.showIp = !webConfiguratorAddress.showIp;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // pin & qr code
                        RowLayout {
                            id: pinQrContainer
                            spacing: 10
                            clip: true

                            Layout.alignment: Qt.AlignBottom
                            Layout.fillWidth: true
                            Layout.preferredHeight: Config.webConfiguratorEnabled ? 60 : 0

                            Behavior on Layout.preferredHeight {
                                NumberAnimation {
                                    duration: 300
                                    easing.type: Easing.OutExpo
                                }
                            }

                            RowLayout {
                                id: pinContainer
                                spacing: 10
                                width: childrenRect.width
                                height: childrenRect.height

                                property string pin: Config.webConfiguratorPin
                                property int containerWidth: 45
                                property int containerHeight: 60

                                Rectangle {
                                    width: pinContainer.containerWidth
                                    height: pinContainer.containerHeight
                                    color: colors.black
                                    border { color: colors.medium; width: 2 }
                                    radius: ui.cornerRadiusSmall

                                    Text {
                                        text: pinContainer.pin[0]
                                        color: colors.offwhite
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                        font: fonts.primaryFont(36, "Light")
                                        anchors.centerIn: parent
                                    }
                                }

                                Rectangle {
                                    width: pinContainer.containerWidth
                                    height: pinContainer.containerHeight
                                    color: colors.black
                                    border { color: colors.medium; width: 2 }
                                    radius: ui.cornerRadiusSmall

                                    Text {
                                        text: pinContainer.pin[1]
                                        color: colors.offwhite
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                        font: fonts.primaryFont(36, "Light")
                                        anchors.centerIn: parent
                                    }
                                }

                                Rectangle {
                                    width: pinContainer.containerWidth
                                    height: pinContainer.containerHeight
                                    color: colors.black
                                    border { color: colors.medium; width: 2 }
                                    radius: ui.cornerRadiusSmall

                                    Text {
                                        text: pinContainer.pin[2]
                                        color: colors.offwhite
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                        font: fonts.primaryFont(36, "Light")
                                        anchors.centerIn: parent
                                    }
                                }

                                Rectangle {
                                    width: pinContainer.containerWidth
                                    height: pinContainer.containerHeight
                                    color: colors.black
                                    border { color: colors.medium; width: 2 }
                                    radius: ui.cornerRadiusSmall

                                    Text {
                                        text: pinContainer.pin[3]
                                        color: colors.offwhite
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                        font: fonts.primaryFont(36, "Light")
                                        anchors.centerIn: parent
                                    }
                                }
                            }

                            Components.HapticMouseArea {
                                Layout.preferredWidth: pinContainer.height
                                Layout.preferredHeight: pinContainer.height
                                Layout.alignment: Qt.AlignVCenter

                                onClicked: {
                                    Config.generateNewWebConfigPin();
                                }

                                onPressed: generateQrCodeIcon.color = colors.highlight
                                onReleased: generateQrCodeIcon.color = colors.light

                                Components.Icon {
                                    id: generateQrCodeIcon
                                    icon: "uc:arrow-rotate-right"
                                    color: colors.light
                                    size: 60
                                    anchors.centerIn: parent

                                    Behavior on color {
                                        ColorAnimation { duration: 200 }
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                id: qrCode

                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.preferredWidth: 60
                                Layout.preferredHeight: 60

                                color: colors.transparent
                                border { width: 10; color: colors.offwhite }

                                Image {
                                    width: parent.width - (profileRoot.state === "showLargeQr" ? 20 : 0)
                                    height: width
                                    anchors.centerIn: parent
                                    fillMode: Image.PreserveAspectFit
                                    antialiasing: false
                                    source: "data:image/png;base64," + ui.createQrCode(("http://%1/configurator").arg(Config.webConfiguratorAddress))
                                    visible: Config.webConfiguratorAddress != ""

                                    Components.HapticMouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (profileRoot.state != "showLargeQr") {
                                                profileRoot.state = "showLargeQr";
                                            } else {
                                                profileRoot.state = "";
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // restricted
                    Item {
                        Layout.alignment: Qt.AlignBottom
                        Layout.preferredHeight: 30
                        Layout.margins: 20

                        visible: ui.profile.restricted

                        Components.Icon {
                            id: lockIcon
                            icon: "uc:lock"
                            color: colors.offwhite
                            opacity: 0.6
                            anchors { left: parent.left }
                            size: 30
                        }

                        Text {
                            color: colors.offwhite
                            opacity: 0.6
                            //: Text explaining that the profile has restricted access
                            text: qsTr("Restricted")
                            anchors { left: lockIcon.right; leftMargin: 10; verticalCenter: lockIcon.verticalCenter }
                            font: fonts.secondaryFont(24)
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

                Item {
                    width: parent.width; height: 80
                    anchors { top: parent.top }
                    opacity: menu.atYBeginning ? 0 : 1

                    Behavior on opacity { PropertyAnimation { duration: 300; easing.type: Easing.OutExpo } }

                    LinearGradient {
                        anchors.fill: parent
                        start: Qt.point(0, 0)
                        end: Qt.point(0, 50)
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: colors.black }
                            GradientStop { position: 1.0; color: colors.transparent }
                        }
                    }
                }

                Item {
                    width: parent.width; height: 80
                    anchors { bottom: parent.bottom }
                    opacity: menu.atYEnd ? 0 : 1

                    Behavior on opacity { PropertyAnimation { duration: 300; easing.type: Easing.OutExpo } }

                    LinearGradient {
                        anchors.fill: parent
                        start: Qt.point(0, 0)
                        end: Qt.point(0, 50)
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: colors.transparent }
                            GradientStop { position: 1.0; color: colors.black }
                        }
                    }
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
            color: colors.light
            text: qsTr("Scan to open\nthe Web Configurator")
            font: fonts.secondaryFont(22)
            anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }
        }

        Text {
            width: parent.width - 40
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            horizontalAlignment: Text.AlignHCenter
            color: colors.light
            text: qsTr("Tap to close")
            font: fonts.secondaryFont(22)
            anchors { bottom: parent.bottom; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }
        }
    }

    Components.ProfileSwitch {
        id: profileSwitch
    }

    Component {
        id: menuItem

        Rectangle {
            width: ListView.view.width
            height: visible ? 80 : 0
            color: isCurrentItem && ui.keyNavigationEnabled ? colors.dark : colors.transparent
            border {
                color: isCurrentItem && ui.keyNavigationEnabled ? colors.medium : colors.transparent
                width: 1
            }
            radius: ui.cornerRadiusSmall
            visible: {
                switch (pos) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    if (ui.profile.restricted) {
                        return false;
                    } else {
                        return true;
                    }
                case 6:
                    return true;
                }
            }

            property bool isCurrentItem: ListView.isCurrentItem

            Rectangle {
                width: counterText.implicitWidth + 20
                height: 30
                radius: 15
                color: colors.medium
                anchors { verticalCenter: parent.verticalCenter; left: menuItemText.right; leftMargin: 10 }
                visible: {
                    switch (pos) {
                    case 0:
                        return SoftwareUpdate.updateAvailable;
                    case 2:
                        return IntegrationController.integrationsModel.count > 0;
                    case 3:
                        return DockController.configuredDocks.count > 0;
                    case 4:
                        return false;
                    case 5:
                        return false;
                    default:
                        return false;
                    }
                }

                Text {
                    id: counterText
                    text: {
                        switch (pos) {
                        case 0:
                            return "1";
                        case 2:
                            return IntegrationController.integrationsModel.count;
                        case 3:
                            return DockController.configuredDocks.count;
                        case 4:
                            return qsTranslate("Abbreviation for not available", "N/A");
                        case 5:
                            return qsTranslate("Abbreviation for not available", "N/A");
                        default:
                            return "";
                        }
                    }
                    color: colors.offwhite
                    anchors.centerIn: parent
                    font: fonts.secondaryFont(20)
                }
            }

            Text {
                id: menuItemText
                color: colors.offwhite
                text: qsTr(name)
                horizontalAlignment: Text.AlignHCenter
                anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter; }
                font: fonts.primaryFont(30)
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    menu.currentIndex = index;
                    loadPage(menu.currentIndex);
                }
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
