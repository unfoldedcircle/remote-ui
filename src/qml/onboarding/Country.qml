// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components

Item {
    Components.ButtonNavigation {
        overrideActive: OnboardingController.currentStep === OnboardingController.Country
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
        ignoreUnknownSignals: true
        enabled: OnboardingController.currentStep === OnboardingController.Country

        function onItemSelected(value) {
            Config.country = value;
        }
    }

    Connections {
        target: OnboardingController
        ignoreUnknownSignals: true

        function onCurrentStepChanged() {
            if (OnboardingController.currentStep === OnboardingController.Country) {
                selectList.state = "visible";
                Config.getCountryList();
            }
        }
    }

    Connections {
        target: Config
        ignoreUnknownSignals: true
        enabled: OnboardingController.currentStep === OnboardingController.Country

        function onCountryListChanged(list) {
            listModel.clear();

            for (let i = 0; i < list.length; i ++) {
                let country = list[i];
                let defaultKey = "name_" + Config.getLanguageCodeFromCountry(country.code.toLowerCase());
                let name = country[defaultKey];

                if (!name) {
                    for (const key in country) {
                        if (key !== "code") {
                            name = country[key];
                            break;
                        }
                    }
                }

                if (!name) {
                    name = country.name_en;
                }

                if (!name || name !== "") {
                    let isUtf8 = true;
                    for (var j = 0; j < name.length; j++) {
                        if (name.charCodeAt(j) > 255) {
                            isUtf8 = false;
                        }
                    }

                    if (!isUtf8) {
                        name = country.name_en;
                    }

                    listModel.append({'name': list[i].code + String.fromCodePoint(0x0009) + name, 'value': list[i].code})
                }
            }

            selectList.popupListmodel.reload();
        }

        function onCountryChanged(success) {
            if (success) {
                OnboardingController.setCountrySelected(true);
                OnboardingController.nextStep();
            }
        }
    }

    Components.PopupList {
        id: selectList
        title: qsTr("Select country")
        showSearch: true
        hideClose: true
        closeOnSelected: false
        listModel: listModel
        countryList: true
        Component.onCompleted: selectList.state = "hidden"
    }

    ListModel {
        id: listModel
    }
}
