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

    property var callBack

    function loadList(title, list, showSearch = true, selectedItem = 0) {
        popupListLoader.setSource("qrc:/components/PopupList.qml", { title: title, listModel: list, showSearch: showSearch, initialSelected: selectedItem, countryList: title.includes("country") });
    }

    ListModel {
        id: listModel
    }

    Timer {
        id: delayTimer
        running: false
        repeat: false
        interval: 50
        onTriggered: callBack()
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

        onContentYChanged: {
            if (contentY < 0) {
                contentY = 0;
            }
            if (contentY > 1100) {
                contentY = 1100;
            }
        }

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

                        languageConnection.enabled = true;

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

                Connections {
                    id: languageConnection
                    target: popupListLoader.item
                    enabled: false

                    function onItemSelected(value) {
                        Config.language = value;
                        languageConnection.enabled = false;
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
                Component.onCompleted: {
                    languageSelector.forceActiveFocus();
                }

                Components.ButtonNavigation {
                    overrideActive: languageSelector.activeFocus
                    defaultConfig: {
                        "DPAD_DOWN": {
                            "pressed": function() {
                                callBack = function () { countrySelector.forceActiveFocus(); }
                                delayTimer.start();
                            }
                        },
                        "DPAD_MIDDLE": {
                            "released": function() {
                                languageSelector.item.trigger();
                            }
                        }
                    }
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

                            countryConnection.enabled = true;

                            let countryList = list;
                            let currentCountryIndex;

                            for (let i = 0; i < countryList.length; i ++) {
                                let country = countryList[i];
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
                                    for (let j = 0; j < name.length; j++) {
                                        if (name.charCodeAt(j) > 255) {
                                            isUtf8 = false;
                                        }
                                    }

                                    if (!isUtf8) {
                                        name = country.name_en;
                                    }

                                    listModel.append({'name': countryList[i].code + String.fromCodePoint(0x0009) + name, 'value': countryList[i].code})
                                }

                                if (countryList[i].code === Config.country) {
                                    currentCountryIndex = i;
                                }
                            }

                            loadList(qsTr("Select country"), listModel, true, currentCountryIndex);
                        }
                    }
                }

                Connections {
                    id: countryConnection
                    target: popupListLoader.item
                    enabled: false

                    function onItemSelected(value) {
                        Config.country = value;
                        countryConnection.enabled = false;
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
                Components.ButtonNavigation {
                    overrideActive: countrySelector.activeFocus
                    defaultConfig: {
                        "DPAD_UP": {
                            "pressed": function() {
                                callBack = function() { languageSelector.forceActiveFocus(); }
                                delayTimer.start();
                            }
                        },
                        "DPAD_DOWN": {
                            "pressed": function() {
                                callBack = function() { timeZoneSelector.forceActiveFocus(); }
                                delayTimer.start();
                            }
                        },
                        "DPAD_MIDDLE": {
                            "released": function() {
                                countrySelector.item.trigger();
                            }
                        }
                    }
                }
            }

            Loader {
                id: timeZoneSelector
                Layout.alignment: Qt.AlignCenter
                width: parent.width
                sourceComponent: selector
                onLoaded: {
                    item.title = qsTr("Timezone");
                    item.value = Qt.binding( function() { return Config.timezone; })

                    getTimeZonesFromConfig.enabled = true;
                    Config.getTimeZones(Config.country);
                }

                Connections {
                    id: getTimeZonesFromConfig
                    target: Config
                    enabled: false

                    function onTimeZoneListChanged (list) {
                        timeZoneSelector.item.trigger = function() {
                            loading.start();
                            listModel.clear();

                            timeZoneConnection.enabled = true;

                            let timeZoneList = list;
                            let currentTimeZoneItem;
                            for (let i = 0; i < timeZoneList.length; i ++) {
                                listModel.append({'name': timeZoneList[i], 'value': timeZoneList[i]})

                                if (timeZoneList[i] === Config.timezone) {
                                    currentTimeZoneItem = i;
                                }
                            }

                            loadList(qsTr("Select timezone"), listModel, true, currentTimeZoneItem);
                        }
                    }
                }

                Connections {
                    id: timeZoneConnection
                    target: popupListLoader.item
                    enabled: false

                    function onItemSelected(value) {
                        Config.timezone = value;
                        timeZoneConnection.enabled = false;
                        getTimeZonesFromConfig.enabled = false;
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
                Components.ButtonNavigation {
                    overrideActive: timeZoneSelector.activeFocus
                    defaultConfig: {
                        "DPAD_UP": {
                            "pressed": function() {
                                callBack = function() { countrySelector.forceActiveFocus(); }
                                delayTimer.start();
                            }
                        },
                        "DPAD_DOWN": {
                            "pressed": function() {
                                callBack = function() { clock24hSelector.forceActiveFocus(); }
                                delayTimer.start();
                            }
                        },
                        "DPAD_MIDDLE": {
                            "released": function() {
                                timeZoneSelector.item.trigger();
                            }
                        }
                    }
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
                    z: item.z + 1
                    icon: "uc:check"
                    checked: Config.clock24h
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                    trigger: function() {
                        Config.clock24h = !Config.clock24h;
                    }
                    highlight: parent.activeFocus && ui.keyNavigationEnabled
                    focus: parent.activeFocus
                }

                onFocusChanged: {
                    if (focus) {
                        item.highlight = true;
                    } else {
                        item.highlight = false;
                    }
                }

                /** KEYBOARD NAVIGATION **/
                Components.ButtonNavigation {
                    overrideActive: clock24hSelector.activeFocus
                    defaultConfig: {
                        "DPAD_UP": {
                            "pressed": function() {
                                callBack = function() { timeZoneSelector.forceActiveFocus(); }
                                delayTimer.start();
                            }
                        },
                        "DPAD_DOWN": {
                            "pressed": function() {
                                callBack = function() { unitSelector.forceActiveFocus(); }
                                delayTimer.start();
                            }
                        }
                    }
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

                        unitSystemConnection.enabled = true;

                        listModel.append({'name': "Metric", 'value': "Metric"})
                        listModel.append({'name': "Uk", 'value': "Uk"})
                        listModel.append({'name': "Us", 'value': "Us"})

                        loadList(qsTr("Select unit system"), listModel, false);
                    }
                }

                Connections {
                    id: unitSystemConnection
                    target: popupListLoader.item
                    enabled: false

                    function onItemSelected(value) {
                        Config.unitSystem = value;
                        unitSystemConnection.enabled = false;
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
                Components.ButtonNavigation {
                    overrideActive: unitSelector.activeFocus
                    defaultConfig: {
                        "DPAD_UP": {
                            "pressed": function() {
                                callBack = function() { clock24hSelector.forceActiveFocus(); }
                                delayTimer.start();
                            }
                        },
                        "DPAD_MIDDLE": {
                            "released": function() {
                                unitSelector.item.trigger();
                            }
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: popupListLoader
        anchors.fill: parent

        Connections {
            target: popupListLoader.item

            function onDone() {
                popupListLoader.source = "";
                languageSelector.forceActiveFocus();
            }

        }
    }

    Component {
        id: selector

        Rectangle {
            id: selectorBg
            width: parent.width
            height: 120
            color: highlight && ui.keyNavigationEnabled ? colors.dark : colors.transparent
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
