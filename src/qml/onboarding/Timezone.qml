// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: timezoneStep

    // Most countries have exactly one timezone: those get a confirm card instead of a list.
    // Multi-zone countries get their own zones only; the full world list stays reachable as an
    // escape hatch (border regions, countries missing from the tzdata mapping).
    property bool confirmMode: false
    property bool worldList: false
    property int countryZoneCount: 0
    property var singleZone: null

    initialFocusItem: buttonConfirm

    onStepEntered: {
        let zones = Config.getTimeZoneInfos(Config.country);
        countryZoneCount = zones.length;

        if (zones.length === 1) {
            confirmMode = true;
            worldList = false;
            singleZone = zones[0];
        } else {
            confirmMode = false;
            openList(zones);
        }
    }

    // a list that stays open on a step that is no longer on screen keeps fighting for the focus
    onStepLeft: selectList.state = "hidden"

    function appendZone(zone) {
        // every optional PopupList role on every row: ListModel roles are fixed on first append
        listModel.append({'name': zone.city, 'value': zone.id, 'secondary': zone.zoneName,
                          'rightText': zone.offsetLabel, 'searchKey': zone.id});
    }

    function selectedIndex() {
        for (let i = 0; i < listModel.count; i++) {
            if (listModel.get(i).value === Config.timezone) {
                return i;
            }
        }
        return 0;
    }

    function openList(zones) {
        if (zones.length === 0) {
            openWorldList();
            return;
        }

        worldList = false;
        listModel.clear();

        for (let i = 0; i < zones.length; i++) {
            appendZone(zones[i]);
        }
        listModel.append({'name': qsTr("All timezones…"), 'value': "__all__", 'secondary': "",
                          'rightText': "", 'searchKey': ""});

        selectList.initialSelected = selectedIndex();
        selectList.popupListmodel.reload();
        selectList.state = "visible";
    }

    function openWorldList() {
        worldList = true;
        listModel.clear();

        let zones = Config.getAllTimeZoneInfos();
        for (let i = 0; i < zones.length; i++) {
            appendZone(zones[i]);
        }

        selectList.initialSelected = selectedIndex();
        selectList.popupListmodel.reload();
        selectList.state = "visible";
    }

    // the list owns the input while it is visible, so BACK has to be bound on the list. It walks
    // back the way the user came: world list -> country list -> previous step, or world list ->
    // confirm card for single-zone countries.
    Component.onCompleted: {
        selectList.buttonNavigation.extendDefaultConfig({
                                                            "BACK": {
                                                                "pressed": function() {
                                                                    if (confirmMode) {
                                                                        selectList.state = "hidden";
                                                                    } else if (worldList && countryZoneCount > 1) {
                                                                        openList(Config.getTimeZoneInfos(Config.country));
                                                                    } else {
                                                                        OnboardingController.previousStep();
                                                                    }
                                                                }
                                                            }
                                                        });
    }

    Connections {
        target: selectList
        ignoreUnknownSignals: true
        enabled: OnboardingController.currentStep === OnboardingController.Timezone

        function onItemSelected(value) {
            if (value === "__all__") {
                openWorldList();
                return;
            }
            Config.timezone = value;
        }
    }

    Connections {
        target: Config
        ignoreUnknownSignals: true
        enabled: OnboardingController.currentStep === OnboardingController.Timezone

        function onTimezoneChanged(success) {
            if (success) {
                OnboardingController.setTimezoneSelected(true);
                OnboardingController.nextStep();
            }
        }
    }

    // ---- confirm card for countries with exactly one timezone ----

    Item {
        id: confirmCard
        anchors.fill: parent
        visible: timezoneStep.confirmMode

        Item {
            id: title
            width: parent.width
            height: 60

            Text {
                text: qsTr("Confirm timezone")
                width: parent.width
                elide: Text.ElideRight
                color: colors.offwhite
                verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                anchors.centerIn: parent
                font: fonts.primaryFont(30)
            }
        }

        Rectangle {
            id: zoneCard
            width: parent.width - 40
            height: 160
            radius: ui.cornerRadiusSmall
            color: colors.dark
            anchors { horizontalCenter: parent.horizontalCenter; top: title.bottom; topMargin: 60 }

            Text {
                id: countryText
                text: Config.countryName
                width: parent.width - 40
                elide: Text.ElideRight
                color: colors.offwhite
                anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 30 }
                font: fonts.primaryFont(30)
            }

            Text {
                // no current time on purpose: there is no NTP sync yet, the clock may be way off
                text: timezoneStep.singleZone
                      ? timezoneStep.singleZone.city + " · " + timezoneStep.singleZone.offsetLabel
                      : ""
                width: parent.width - 40
                elide: Text.ElideRight
                color: colors.light
                anchors { left: countryText.left; top: countryText.bottom; topMargin: 10 }
                font: fonts.secondaryFont(26)
            }
        }

        Components.Button {
            id: buttonConfirm
            text: qsTr("Confirm")
            width: parent.width - 40
            anchors { horizontalCenter: parent.horizontalCenter; top: zoneCard.bottom; topMargin: 60 }
            KeyNavigation.down: chooseOther
            trigger: function() {
                if (timezoneStep.singleZone) {
                    Config.timezone = timezoneStep.singleZone.id;
                }
            }
        }

        Item {
            id: chooseOther
            width: chooseOtherText.implicitWidth + 40
            height: 80
            anchors { horizontalCenter: parent.horizontalCenter; top: buttonConfirm.bottom; topMargin: 20 }
            KeyNavigation.up: buttonConfirm

            Keys.onReturnPressed: {
                timezoneStep.openWorldList();
                event.accepted = true;
            }

            Text {
                id: chooseOtherText
                text: qsTr("Choose another timezone")
                color: colors.light
                anchors.centerIn: parent
                font: fonts.secondaryFont(26)
            }

            Rectangle {
                anchors.fill: parent
                radius: ui.cornerRadiusSmall
                color: colors.transparent
                border {
                    width: 2
                    color: chooseOther.activeFocus && ui.keyNavigationActive ? colors.highlight : colors.transparent
                }
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    timezoneStep.openWorldList();
                }
            }
        }
    }

    // ---- timezone list for multi-zone countries and the world list escape hatch ----

    Components.PopupList {
        id: selectList
        title: qsTr("Select timezone")
        showSearch: true
        hideClose: true
        closeOnSelected: false
        listModel: listModel
        Component.onCompleted: selectList.state = "hidden"
    }

    ListModel {
        id: listModel
    }
}
