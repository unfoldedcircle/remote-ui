// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components

Item {
    Components.ButtonNavigation {
        overrideActive: OnboardingController.currentStep === OnboardingController.Timezone
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    OnboardingController.previousStep();
                }
            },
        }
    }

    Connections {
        target: selectList
        enabled: OnboardingController.currentStep == OnboardingController.Timezone

        function onItemSelected(value) {
            Config.timezone = value;
        }
    }

    Connections {
        target: OnboardingController
        ignoreUnknownSignals: true

        function onCurrentStepChanged() {
            if (OnboardingController.currentStep == OnboardingController.Timezone) {
                selectList.state = "visible";
                Config.getTimeZones(Config.country);
            }
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
