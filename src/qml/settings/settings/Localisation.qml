// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


import Haptic 1.0
import Config 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: localisationPageContent
    scrollTarget: flickable
    initialFocusItem: languageSelector

    // The one handler for a selection in the popup list. Every row sets it in its trigger and it is
    // cleared when the popup closes. Previously each row had its own Connections object that was
    // enabled in the trigger and only disabled again by a selection: a popup closed with BACK left
    // its handler armed, and the next selection in any other popup fired all armed handlers, e.g.
    // writing a timezone id into the language, country and unit system settings.
    property var popupSelectionHandler: null

    function loadList(title, list, showSearch = true, selectedItem = 0, closeOnSelected = true, sectionRole = "") {
        popupListLoader.setSource("qrc:/components/PopupList.qml", { title: title, listModel: list, showSearch: showSearch, initialSelected: selectedItem, closeOnSelected: closeOnSelected, sectionRole: sectionRole });
    }

    // fills listModel with the timezones of the configured country, or all timezones when the
    // country has none (or the user asked for the full list); returns the row to preselect
    function buildTimeZoneModel(world) {
        listModel.clear();

        let zones = world ? [] : Config.getTimeZoneInfos(Config.country);
        if (zones.length === 0) {
            world = true;
            zones = Config.getAllTimeZoneInfos();
        }

        let current = 0;
        for (let i = 0; i < zones.length; i++) {
            // every optional PopupList role on every row: ListModel roles are fixed on first append
            listModel.append({'name': zones[i].city, 'value': zones[i].id, 'secondary': zones[i].zoneName,
                              'rightText': zones[i].offsetLabel, 'searchKey': zones[i].id});
            if (zones[i].id === Config.timezone) {
                current = i;
            }
        }

        if (!world) {
            listModel.append({'name': qsTr("All timezones…"), 'value': "__all__", 'secondary': "",
                              'rightText': "", 'searchKey': ""});
        }

        return current;
    }

    ListModel {
        id: listModel
        // shared by all popups of this page with differing role sets; without this the roles
        // would be fixed by whichever popup appends first (clear() does not reset them) and
        // later popups would silently lose e.g. their section or offset columns
        dynamicRoles: true
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        boundsBehavior: Flickable.StopAtBounds

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 0
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            Loader {
                id: languageSelector
                Layout.alignment: Qt.AlignCenter
                width: parent.width
                sourceComponent: selector
                onLoaded: {
                    item.title = qsTr("Language");
                    item.value = Qt.binding( function() { return Config.getLanguageAsNative(); });
                    if (focus) {
                        item.highlight = true;
                    }
                    item.trigger = function() {
                        loading.start();
                        listModel.clear();

                        popupSelectionHandler = function(value) {
                            Config.language = value;
                        };

                        let languageList = Config.getTranslations();
                        let currentLanguageItem;
                        for (let i = 0; i < languageList.length; i ++) {
                            listModel.append({'name': Config.getLanguageAsNative(languageList[i]), 'value': languageList[i]})

                            if (languageList[i] === Config.language) {
                                currentLanguageItem = i;
                            }
                        }

                        loadList(qsTr("Select language"), listModel, false, currentLanguageItem);
                    }
                }

                onFocusChanged: {
                    if (languageSelector.status === Loader.Ready) {
                        if (focus) {
                            item.highlight = true;
                        } else {
                            item.highlight = false;
                        }
                    }
                }

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.down: countrySelector

                Keys.onReturnPressed: {
                    if (languageSelector.item && languageSelector.item.trigger) {
                        languageSelector.item.trigger();
                    }

                    event.accepted = true;
                }
            }

            Loader {
                id: countrySelector
                Layout.alignment: Qt.AlignCenter
                width: parent.width
                sourceComponent: selector
                onLoaded: {
                    item.title = qsTr("Country");
                    item.value = Qt.binding( function() { return Config.countryName; })

                    getCountriesFromConfig.enabled = true;
                    Config.getCountryList();
                }

                Connections {
                    id: getCountriesFromConfig
                    target: Config
                    enabled: false

                    function onCountryListChanged (list) {
                        countrySelector.item.trigger = function() {
                            loading.start();
                            listModel.clear();

                            popupSelectionHandler = function(value) {
                                Config.country = value;
                            };

                            // same structure as the onboarding country step: countries where the
                            // configured language is spoken first, then the full list; names
                            // resolved to the current UI language in C++, ISO code searchable
                            let countries = Config.getLocalizedCountryList();
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
                            // most likely country on top, the rest sorted by name. Copied into a
                            // fresh array on purpose: sorting an array modified with shift()
                            // corrupts it in the Qt 5.15 JS engine
                            let rest = [];
                            for (let i = 1; i < matchedRows.length; i++) {
                                rest.push(matchedRows[i]);
                            }
                            rest.sort(function(a, b) { return a.name.localeCompare(b.name); });
                            let suggestedRows = matchedRows.length > 0 ? [matchedRows[0]].concat(rest) : [];

                            for (let i = 0; i < suggestedRows.length; i++) {
                                listModel.append({'name': suggestedRows[i].name, 'value': suggestedRows[i].code,
                                                  'secondary': "", 'rightText': "", 'searchKey': suggestedRows[i].code,
                                                  'section': sectionSuggested});
                            }
                            for (let j = 0; j < countries.length; j++) {
                                listModel.append({'name': countries[j].name, 'value': countries[j].code,
                                                  'secondary': "", 'rightText': "", 'searchKey': countries[j].code,
                                                  'section': sectionAll});
                            }

                            // preselect the configured country, preferring its entry in the
                            // suggestions (earliest index): changing the country usually means
                            // switching between common countries, e.g. GB -> US or AU -> NZ
                            let currentCountryIndex = 0;
                            for (let i = 0; i < listModel.count; i++) {
                                if (listModel.get(i).value === Config.country) {
                                    currentCountryIndex = i;
                                    break;
                                }
                            }

                            loadList(qsTr("Select country"), listModel, true, currentCountryIndex, true, "section");
                        }
                    }
                }

                onFocusChanged: {
                    if (focus) {
                        item.highlight = true;
                    } else {
                        item.highlight = false;
                    }
                }

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: languageSelector
                KeyNavigation.down: timeZoneSelector

                Keys.onReturnPressed: {
                    if (countrySelector.item && countrySelector.item.trigger) {
                        countrySelector.item.trigger();
                    }

                    event.accepted = true;
                }
            }

            Loader {
                id: timeZoneSelector
                Layout.alignment: Qt.AlignCenter
                width: parent.width
                sourceComponent: selector
                onLoaded: {
                    item.title = qsTr("Timezone");
                    // the raw IANA id names the zone after its most populous city: show that city
                    item.value = Qt.binding( function() {
                        return Config.timezone.split("/").pop().replace(/_/g, " ");
                    })
                    item.trigger = function() {
                        loading.start();

                        popupSelectionHandler = function(value) {
                            if (value === "__all__") {
                                popupListLoader.item.initialSelected = buildTimeZoneModel(true);
                                popupListLoader.item.popupListmodel.reload();
                                return;
                            }

                            Config.timezone = value;
                            popupListLoader.item.state = "hidden";
                        };

                        // the list stays open when "All timezones…" swaps the model, so the popup
                        // is closed manually on a real selection
                        loadList(qsTr("Select timezone"), listModel, true, buildTimeZoneModel(false), false);
                    }
                }

                onFocusChanged: {
                    if (focus) {
                        item.highlight = true;
                    } else {
                        item.highlight = false;
                    }
                }

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: countrySelector
                KeyNavigation.down: clock24hSelector

                Keys.onReturnPressed: {
                    if (timeZoneSelector.item && timeZoneSelector.item.trigger) {
                        timeZoneSelector.item.trigger();
                    }

                    event.accepted = true;
                }
            }

            Loader {
                id: clock24hSelector
                Layout.alignment: Qt.AlignCenter
                width: parent.width
                sourceComponent: selector
                onLoaded: {
                    //: Title for indicating if 24h time visualisation is enabled
                    item.title = qsTr("24-hour time");
                }

                Components.Switch {
                    id: clock24hSwitch
                    z: item.z + 1
                    icon: "uc:check"
                    checked: Config.clock24h
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                    trigger: function() {
                        Config.clock24h = !Config.clock24h;
                    }
                    // the row itself carries the focus, like every other row on this page
                    highlight: clock24hSelector.activeFocus && ui.keyNavigationActive
                }

                onFocusChanged: {
                    if (focus) {
                        item.highlight = true;
                    } else {
                        item.highlight = false;
                    }
                }

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: timeZoneSelector
                KeyNavigation.down: unitSelector

                Keys.onReturnPressed: {
                    clock24hSwitch.activate();
                    event.accepted = true;
                }
            }

            Loader {
                id: unitSelector
                Layout.alignment: Qt.AlignCenter
                width: parent.width
                sourceComponent: selector

                onLoaded: {
                    //: Like metric, imperial
                    item.title = qsTr("Unit System");
                    item.value = Qt.binding( function() { return Config.unitSystem; })
                    item.trigger = function() {
                        //                        loading.start();
                        listModel.clear();

                        popupSelectionHandler = function(value) {
                            Config.unitSystem = value;
                        };

                        listModel.append({'name': "Metric", 'value': "Metric"})
                        listModel.append({'name': "Uk", 'value': "Uk"})
                        listModel.append({'name': "Us", 'value': "Us"})

                        loadList(qsTr("Select unit system"), listModel, false);
                    }
                }

                onFocusChanged: {
                    if (focus) {
                        item.highlight = true;
                    } else {
                        item.highlight = false;
                    }
                }

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: clock24hSelector

                Keys.onReturnPressed: {
                    if (unitSelector.item && unitSelector.item.trigger) {
                        unitSelector.item.trigger();
                    }

                    event.accepted = true;
                }
            }
        }
    }

    Loader {
        id: popupListLoader
        anchors.fill: parent

        Connections {
            target: popupListLoader.item

            function onItemSelected(value) {
                if (popupSelectionHandler) {
                    popupSelectionHandler(value);
                }
            }

            function onDone() {
                popupSelectionHandler = null;
                // the page takes the focus back on its own, on the row the user came from
                popupListLoader.source = "";
            }
        }
    }

    Component {
        id: selector

        Rectangle {
            id: selectorBg
            width: parent.width
            height: 120
            color: highlight && ui.keyNavigationActive ? colors.dark : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: Qt.lighter(selectorBg.color, 1.3)
                width: 1
            }

            property string title
            property alias value: valueText.text
            property alias mouseArea: mouseArea
            property bool highlight: false
            property var trigger

            Text {
                id: titleText
                text: qsTr(title)
                width: parent.width/2
                wrapMode: Text.WordWrap
                color: colors.offwhite
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                font: fonts.primaryFont(30)
            }

            Text {
                id: valueText
                width: parent.width/2
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignRight
                color: colors.offwhite
                anchors { right: parent.right; rightMargin: 10; baseline: titleText.baseline }
                font: fonts.primaryFont(20, "Bold")
            }

            Components.HapticMouseArea {
                id: mouseArea
                enabled: valueText.text != ""
                anchors.fill: parent
                onClicked: {
                    trigger();
                }

            }
        }
    }
}
