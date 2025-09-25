// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components

Item {
    Components.ButtonNavigation {
        overrideActive: OnboardingController.currentStep === OnboardingController.Language
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
        enabled: OnboardingController.currentStep === OnboardingController.Language

        function onItemSelected(value) {
            Config.language = value;
            OnboardingController.setLanguageSelected(true);
            OnboardingController.nextStep();
        }
    }

    Connections {
        target: OnboardingController
        ignoreUnknownSignals: true

        function onCurrentStepChanged() {
            if (OnboardingController.currentStep == OnboardingController.Language) {
                 selectList.state = "visible";
            }
        }
    }

    Components.PopupList {
        id: selectList
        title: qsTr("Select language")
        showSearch: true
        hideClose: true
        closeOnSelected: false
        listModel: listModel
        Component.onCompleted: selectList.state = "hidden"
    }

    ListModel {
        id: listModel

        Component.onCompleted: {
            listModel.clear();

            let list = Config.getTranslations();

            for (let i = 0; i < list.length; i ++) {
                listModel.append({'name': Config.getLanguageAsNative(list[i]), 'value': list[i]})
            }
        }
    }
}
