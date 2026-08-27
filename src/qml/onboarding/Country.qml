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
        Config.getCountryList();
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
        ignoreUnknownSignals: true
        enabled: OnboardingController.currentStep === OnboardingController.Country

        function onItemSelected(value) {
            Config.country = value;
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
