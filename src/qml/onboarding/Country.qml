// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: countryStep

    // The list navigates with the d-pad through its own button navigation and takes the input when
    // it becomes visible. It is hidden again when the step is left: a list that stays open on a
    // step that is no longer on screen keeps its search field (and the keyboard) fighting for the
    // focus with the step that replaced it, and its input ownership would be stale on return.
    onStepEntered: {
        selectList.state = "visible";
        Config.getCountryList();
    }

    onStepLeft: {
        selectList.state = "hidden";
        // the fresh list arrives async from core on the next entry: without this the list
        // re-opens with last visit's rows and rebuilds them while visible
        listModel.clear();
    }

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

    // Countries where the chosen UI language is spoken come first (most likely country on top and
    // preselected), followed by the full list. Names arrive localized from C++
    // (Config.getLocalizedCountryList), the ISO code is searchable via the searchKey role.
    function buildModel() {
        listModel.clear();

        let countries = Config.getLocalizedCountryList();
        if (countries.length === 0) {
            return;
        }

        let suggested = Config.getSuggestedCountries();
        let sectionSuggested = qsTr("Suggested");
        let sectionAll = qsTr("All countries");

        let matchedRows = [];
        for (let i = 0; i < suggested.length; i++) {
            for (let j = 0; j < countries.length; j++) {
                if (countries[j].code === suggested[i]) {
                    matchedRows.push(countries[j]);
                    break;
                }
            }
        }
        // The most likely country stays on top, the other suggestions are sorted by name.
        // The rest is copied into a fresh array on purpose: in the Qt 5.15 JS engine, sorting
        // an array that was modified with shift()/unshift() corrupts it — the sort operates on
        // the backing store including the removed element, which duplicated the first row and
        // silently dropped a country.
        let rest = [];
        for (let i = 1; i < matchedRows.length; i++) {
            rest.push(matchedRows[i]);
        }
        rest.sort(function(a, b) { return a.name.localeCompare(b.name); });
        let suggestedRows = matchedRows.length > 0 ? [matchedRows[0]].concat(rest) : [];

        for (let i = 0; i < suggestedRows.length; i++) {
            listModel.append({'name': suggestedRows[i].name, 'value': suggestedRows[i].code,
                              'searchKey': suggestedRows[i].code, 'section': sectionSuggested});
        }
        for (let j = 0; j < countries.length; j++) {
            listModel.append({'name': countries[j].name, 'value': countries[j].code,
                              'searchKey': countries[j].code, 'section': sectionAll});
        }

        // always open at the top: the most likely country for the chosen language is the first
        // row and carries the d-pad selection. No country is scrolled into view — not even one
        // picked earlier in this run, because a changed language makes any earlier pick stale
        // and a centered list hides the suggestions.
        selectList.initialSelected = 0;
        selectList.popupListmodel.reload();
    }

    Connections {
        target: Config
        ignoreUnknownSignals: true
        enabled: OnboardingController.currentStep === OnboardingController.Country

        function onCountryListChanged(list) {
            buildModel();
        }

        // the language step advances before core confirms the new language, so this step can
        // build its model with the previous language still active (wrong names, wrong
        // suggestions); rebuild once the confirmation lands
        function onLanguageChanged(language) {
            buildModel();
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
        sectionRole: "section"
        Component.onCompleted: selectList.state = "hidden"
    }

    ListModel {
        id: listModel
    }
}
