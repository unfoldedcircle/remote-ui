// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.1

import Haptic 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/keypad" as Keypad

Rectangle {
    id: profileSelector
    width: parent.width; height: parent.height
    color: colors.black
    enabled: state === "visible"

    property bool noProfile: false
    property bool hiddenAnimationDone: false
    property bool profileSelected: false
    property string selectedProfileId

    signal closed

    function open() {
        profileSelector.state = "visible";
    }

    state: "hidden"

    transform: Scale {
        origin.x: profileSelector.width/2; origin.y: profileSelector.height/2
    }

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: profileSelector; opacity: 0; x: ui.width }
        },
        State {
            name: "visible"
            PropertyChanges { target: profileSelector; opacity: 1; x: 0 }
        }
    ]
    transitions: [
        Transition {
            from: "visible"
            to: "hidden"

            SequentialAnimation {
                PropertyAnimation { target: profileSelector; properties: "opacity, x"; easing.type: Easing.InExpo; duration: 200 }
                PropertyAction { target: profileSelector; property: "hiddenAnimationDone"; value: true }
                ScriptAction { script: buttonNavigation.releaseControl() }
            }
        },
        Transition {
            from: "hidden"
            to: "visible"

            SequentialAnimation {
                PropertyAnimation { target: profileSelector; properties: "opacity, x"; easing.type: Easing.OutExpo; duration: 300 }
                ScriptAction { script: buttonNavigation.takeControl() }
            }
        }
    ]

    onStateChanged: {
        if (state == "visible") {
            ui.getProfilesFromCore();

            for (let i = 0; i < profileList.count; i++) {
                if (ui.profiles.getProfileId(i) === Config.currentProfileId) {
                    profileList.currentIndex = i;
                    return;
                }
            }
        } else {
            closed();
        }
    }

    onHiddenAnimationDoneChanged: {
        if (hiddenAnimationDone) {
            console.debug("Animation is done");

        }
    }

    function switchProfile(profileId) {
        if (ui.profile.restricted) {
            profileSelector.selectedProfileId = profileId;
            pinKeyPopup.open();
        } else {
            loading.start();
            if  (ui.switchProfile(profileId) === -1 ) {
                loading.failure(true);
            }
        }
    }

    Connections {
        target: ui
        ignoreUnknownSignals: true

        function onProfileSwitch(success) {
            if (success) {
                profileSelector.profileSelected = true;
                loading.success(true, function() { profileSelector.state = "hidden"; parent.closeAnimation.start(); });
                pinKeyPopup.close();
            } else {
                loading.stop();
                pinKeyPad.showError();
            }
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    profileList.incrementCurrentIndex();
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    profileList.decrementCurrentIndex();
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (keyboard.state === "") {
                        switchProfile(visualModel.items.get(profileList.currentIndex).model.profileId);
                    }
                }
            },
            "BACK": {
                "pressed": function() {
                    if (!profileSelector.noProfile) {
                        profileSelector.state = "hidden";
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    if (!profileSelector.noProfile) {
                        profileSelector.state = "hidden";
                        profileSelector.parent.closeAnimation.start();
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    DelegateModel {
        id: visualModel

        model: ui.profiles
        delegate: profileItem
    }

    ListView {
        id: profileList
        width: parent.width; height: parent.height-60
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        pressDelay: 200

        model: visualModel //ui.profiles
        //        delegate: profileItem

        footer: ui.isOnboarding ? null : footerItem

        ScrollBar.vertical: ScrollBar {
            opacity: 0.5
        }

        remove: Transition {
            NumberAnimation { properties: "x"; from:0; to: 100; duration: 300; easing.type: Easing.OutExpo }
            NumberAnimation { properties: "opacity"; from:1; to: 0; duration: 300; easing.type: Easing.OutExpo }
        }

        displaced: Transition {
            NumberAnimation { property: "y"; duration: 300; easing.type: Easing.OutExpo }
        }

        Component.onCompleted: {
            profileList.positionViewAtIndex(profileList.currentIndex, ListView.Visible);
        }
    }


    Rectangle {
        width: ui.width; height: 60
        color: colors.black
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: titleText
            color: colors.offwhite
            //: User profiles
            text: qsTr("Profiles")
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }

        Components.Icon {
            id: closeIcon
            color: colors.offwhite
            icon: "uc:arrow-left"
            anchors { verticalCenter: titleText.verticalCenter; left: parent.left }
            size: 80
            visible: !profileSelector.noProfile
        }

        Components.HapticMouseArea {
            width: 120; height: 120
            anchors.centerIn: closeIcon
            onClicked: {
                profileSelector.state = "hidden";
            }
            enabled: closeIcon.visible
        }
    }

    Components.ProfileAdd {
        id: profileAdd
        parentController: String(profileSelector)
        anchors.centerIn: parent
    }

    Components.PopupMenu {
        id: popupMenu
    }

    Components.ProfileRename {
        id: profileRename
        anchors.centerIn: parent
    }

    Components.IconSelector {
        id: iconSelector

        property string profileId

        Connections {
            target: iconSelector
            ignoreUnknownSignals: true

            function onIconSelected(icon) {
                ui.changeProfileIcon(iconSelector.profileId, icon);
            }
        }
    }

    Popup {
        id: pinKeyPopup
        modal: false
        width: parent.width
        height: parent.height
        closePolicy: Popup.NoAutoClose
        padding: 0

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        onOpened: pinKeyPopupButtonNavigation.takeControl()
        onClosed: pinKeyPopupButtonNavigation.releaseControl()

        background: Rectangle { color: colors.black }

        contentItem: Item {
            Text {
                id: description
                width: parent.width
                wrapMode: Text.WordWrap
                color: colors.light
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Please enter the administrator PIN.")
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 20 }
                font: fonts.secondaryFont(24)
            }

            Keypad.KeyPad {
                id: pinKeyPad
                width: parent.width
                anchors { top: description.bottom; topMargin: 20 }

                onPinEntered: {
                    loading.start();
                    if  (ui.switchProfile(profileSelector.selectedProfileId, pinKeyPad.pinToCheck) === -1) {
                        loading.failure(true);
                        pinKeyPad.showError();
                    }
                }
            }

            Components.Button {
                width: parent.width - 40
                anchors { bottom: parent.bottom; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }
                text: qsTr("Cancel")
                trigger: function() { pinKeyPopup.close(); }
            }
        }

        Components.ButtonNavigation {
            id: pinKeyPopupButtonNavigation
        }
    }



    Component {
        id: profileItem

        Rectangle {
            width: ui.width; height: 120
            color: isCurrentItem && ui.keyNavigationEnabled ? colors.dark : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: isCurrentItem && ui.keyNavigationEnabled ? colors.medium : colors.transparent
                width: 1
            }

            property bool isCurrentItem: ListView.isCurrentItem

            RowLayout {
                spacing: 10
                anchors.fill: parent

                Components.Icon {
                    id: icon

                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 10
                    color: colors.offwhite
                    icon: profileIcon
                    size: 80
                }

                Text {
                    id: profileItemText

                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    color: colors.offwhite
                    text: profileName
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.NoWrap
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    font: fonts.primaryFont(40, "Light")
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 30
                }

                Components.Icon {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: 10
                    icon: "uc:lock"
                    color: colors.offwhite
                    size: 40
                    visible: profileRestricted
                }

                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.rightMargin: 20
                    visible: profileId == Config.currentProfileId
                    width: 20; height: 20
                    color: colors.green
                    radius: 10
                }
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    profileList.currentIndex = index;
                    profileList.positionViewAtIndex(profileList.currentIndex, ListView.Visible);

                    switchProfile(profileId);
                }

                onPressAndHold: {
                    if (!ui.profile.restricted) {
                        Haptic.play(Haptic.Buzz);

                        popupMenu.title = profileName;
                        popupMenu.menuItems =
                                [
                                    {
                                        //: Menu item for profile rename
                                        title: qsTr("Rename"),
                                        icon: "uc:pen-to-square",
                                        callback: function() {
                                            profileRename.profileId = profileId;
                                            profileRename.inputFieldContainer.inputField.text = profileName;
                                            profileRename.state = "visible";
                                        }
                                    },
                                    {
                                        //: Menu item for changing icon
                                        title: qsTr("Edit icon"),
                                        icon: "uc:user",
                                        callback: function() {
                                            iconSelector.profileId = profileId;
                                            iconSelector.open();
                                        }
                                    },
                                    {
                                        //: Menu item for profile delete
                                        title: qsTr("Delete"),
                                        icon: "uc:trash",
                                        callback: function() { ui.deleteProfile(profileId, -1); }
                                    }

                                ];

                        popupMenu.open();
                    } else {
                        Haptic.play(Haptic.Error);
                    }
                }
            }
        }
    }


    Component {
        id: footerItem

        Item {
            width: ui.width; height: visible ? 150 : 0
            visible: !ui.profile.restricted

            Item {
                id: plusIcon
                anchors.centerIn: parent

                Rectangle {
                    width: 60
                    height: 2
                    color: colors.offwhite
                    anchors.centerIn: parent
                }

                Rectangle {
                    width: 2
                    height: 60
                    color: colors.offwhite
                    anchors.centerIn: parent
                }
            }

            Components.HapticMouseArea {
                anchors.fill: parent

                onClicked: {
                    popupMenu.title = qsTr("Add a new profile");
                    popupMenu.menuItems =
                            [
                                {
                                    //: Menu item for adding a normal profile
                                    title: qsTr("Normal"),
                                    icon: "uc:user",
                                    callback: function() {
                                        profileAdd.state = "visible";
                                        profileAdd.limited = false;
                                    }
                                },
                                {
                                    //: Menu item for adding a limited guest profile
                                    title: qsTr("Restricted"),
                                    icon: "uc:ghost",
                                    callback: function() {
                                        profileAdd.state = "visible";
                                        profileAdd.limited = true;
                                    }
                                }

                            ];

                    popupMenu.open();
                }
            }
        }
    }
}
