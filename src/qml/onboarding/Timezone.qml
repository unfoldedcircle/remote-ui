// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    // The list navigates with the d-pad through its own button navigation and takes the input when
    // it becomes visible. It is hidden again when the step is left: a list that stays open on a
    // step that is no longer on screen keeps its search field (and the keyboard) fighting for the
    // focus with the step that replaced it, and its input ownership would be stale on return.
    onStepEntered: {
        selectList.state = "visible";
        Config.getTimeZones(Config.country);
    }

    onStepLeft: selectList.state = "hidden"

    // the list owns the input, so BACK has to be bound on the list to reach the previous step
    Component.onCompleted: {
        selectList.buttonNavigation.extendDefaultConfig({
                                                            "BACK": {
                                                                "pressed": function() {
                                                                    OnboardingController.previousStep();
                                                                }
                                                            }
                                                        });
    }

    Connections {
        target: selectList
        enabled: OnboardingController.currentStep == OnboardingController.Timezone

        function onItemSelected(value) {
            Config.timezone = value;
        }
    }

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

    Connections {
        target: Config
        enabled: OnboardingController.currentStep == OnboardingController.Timezone

        function onTimeZoneListChanged (list) {
            listModel.clear();

            let timezoneList = list;
            for (let i = 0; i < timezoneList.length; i ++) {
                listModel.append({'name': timezoneList[i], 'value': timezoneList[i]})
            }

            selectList.popupListmodel.reload();
        }

        function onTimezoneChanged(success) {
            if (success) {
                OnboardingController.setTimezoneSelected(true);
                OnboardingController.nextStep();
            }
        }
    }
}
