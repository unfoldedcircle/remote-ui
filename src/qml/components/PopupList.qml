// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15


import Haptic 1.0

import "qrc:/components" as Components

Rectangle {
    id: popupList
    width: parent.width; height: parent.height
    enabled: state === "visible"
    color: colors.black

    signal itemSelected(var value)
    signal done()

    property alias title: titleText.text
    property var listModel
    property bool inputHasFocus: false
    property bool showSearch: true
    property bool hideClose: false
    property bool closeOnSelected: true
    property int initialSelected: 0
    property bool countryList: false
    // optional: name of a string role that groups adjacent rows under a section header (e.g. "section").
    // Model rows may also carry the optional roles "secondary" (second text line), "rightText"
    // (right-aligned text, e.g. a GMT offset) and "searchKey" (matched by the filter in addition
    // to "name"). ListModel roles are fixed on first append: a model using any of these roles must
    // set them on every row (empty string when unused).
    property string sectionRole: ""

    property alias popupListmodel: popupListmodel
    property alias buttonNavigation: buttonNavigation

    state: "hidden"

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: popupList; opacity: 0; scale: 0.6; enabled: false }
        },
        State {
            name: "visible"
            PropertyChanges { target: popupList; opacity: 1; scale: 1; enabled: true }
        }

    ]

    transitions: [
        Transition {
            from: "visible"
            to: "hidden"
            SequentialAnimation {
                ParallelAnimation {
                    PropertyAnimation { target: popupList; properties: "scale, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                }
                PropertyAnimation { target: popupList; properties: "enabled" }
                ScriptAction { script: buttonNavigation.releaseControl() }
            }

            onRunningChanged: {
                if (!running) {
                    popupList.done();
                }
            }
        },
        Transition {
            from: "hidden"
            to: "visible"
            SequentialAnimation {
                PropertyAnimation { target: popupList; properties: "enabled" }
                ParallelAnimation {
                    PropertyAnimation { target: popupList; properties: "scale, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                }
                ScriptAction { script: buttonNavigation.takeControl() }
            }

        }
    ]

    onStateChanged:  {
        if (popupList.state == "visible") {
            popupListmodel.reload();
            loading.stop();
        } else {
            keyboard.hide();
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    itemList.incrementCurrentIndex();
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    itemList.decrementCurrentIndex();
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (itemList.currentIndex < 0 || itemList.currentIndex >= itemList.model.count) {
                        return;
                    }
                    itemSelected(itemList.model.get(itemList.currentIndex).value);
                    if (closeOnSelected) {
                        popupList.state = "hidden";
                    }
                }
            },
            "BACK": {
                "pressed": function() {
                    if (!hideClose) {
                        popupList.state = "hidden";
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    if (!hideClose) {
                        popupList.state = "hidden";
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Rectangle {
        id: titleContainer
        color: colors.black
        width: parent.width
        height: showSearch ? 180 : 80
        z: 200
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

        Text {
            id: titleText
            width: parent.width
            elide: Text.ElideRight
            color: colors.offwhite
            horizontalAlignment: Text.AlignHCenter
            anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 10 }
            font: fonts.primaryFont(30)
        }

        Components.Icon {
            id: closeIcon
            color: colors.offwhite
            icon: "uc:xmark"
            anchors { verticalCenter: titleText.verticalCenter; right: parent.right }
            size: 80
            visible: !hideClose
        }

        Components.HapticMouseArea {
            enabled: !hideClose
            width: 120; height: 120
            anchors.centerIn: closeIcon
            onClicked: {
                popupList.state = "hidden";
            }
        }

        Components.SearchField {
            id: searchField
            width: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; top: titleText.bottom; topMargin: 20 }
            visible: showSearch

            placeholderText: qsTr("Search")

            inputField.onFocusChanged: {
                if (inputField.focus) {
                    inputHasFocus = true;
                } else {
                    if (keyboard.active) {
                        inputField.forceActiveFocus();
                    }
                }
            }
            inputField.onTextChanged: {
                if (inputField.text.length > 0) {
                    popupListmodel.applyFilter(inputField.text);
                } else {
                    popupListmodel.reload();
                }
            }
        }
    }

    ListView {
        id: itemList
        width: parent.width
        height: parent.height - titleContainer.height
        anchors { horizontalCenter: parent.horizontalCenter; top: titleContainer.bottom }

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        pressDelay: 200

        model: popupListmodel

        delegate: listItem

        section.property: popupList.sectionRole
        section.delegate: sectionHeader

        ScrollBar.vertical: ScrollBar {
            opacity: 0.5
        }
    }

    Components.ScrollIndicator {
        parentObj: itemList
    }

    MouseArea {
        anchors.fill: parent
        enabled: inputHasFocus
        onClicked: {
            keyboard.hide();
            inputHasFocus = false;
        }
    }

    ListModel {
        id: popupListmodel

        // a search term that is still in the field stays applied: the list is reloaded when it is
        // shown again and whenever its model arrives, and must not silently drop the filter
        function reload() {
            if (showSearch && searchField.inputField.text.length > 0) {
                popupListmodel.applyFilter(searchField.inputField.text);
                return;
            }

            // detach the current item before resetting the model: a delegate pinned by
            // currentIndex can survive the clear/append cycle of a visible list and linger as a
            // stale copy painted over the row below it (showed up as a doubled country row)
            itemList.currentIndex = -1;
            popupListmodel.clear();

            for (var i = 0; i < listModel.count; i++) {
                popupListmodel.append(listModel.get(i));
            }

            itemList.forceLayout();
            itemList.currentIndex = popupList.initialSelected;
            if (popupList.initialSelected > 0) {
                itemList.positionViewAtIndex(popupList.initialSelected, ListView.Center);
            } else {
                // positionViewAtIndex(0, Center) can end up past the top and scroll the first
                // section header out of the view
                itemList.positionViewAtBeginning();
            }
        }

        function applyFilter(searchCriteria) {
            itemList.currentIndex = -1;
            popupListmodel.clear();

            console.debug("Search length: " + searchCriteria.length);

            for (var i = 0; i < listModel.count; i++) {
                let item = listModel.get(i);
                let str;
                if (searchCriteria.length === 2 && popupList.countryList) {
                    str = item.name.slice(0, 2);
                } else {
                    str = item.name;
                }
                let searchKey = item.searchKey === undefined ? "" : item.searchKey;
                if (str.toLowerCase().indexOf(searchCriteria.toLowerCase()) > -1 ||
                        (searchKey !== "" && searchKey.toLowerCase().indexOf(searchCriteria.toLowerCase()) > -1)) {
                    popupListmodel.append(item);
                }
            }

            // start the d-pad selection on the first match
            itemList.forceLayout();
            if (popupListmodel.count > 0) {
                itemList.currentIndex = 0;
                itemList.positionViewAtBeginning();
            }
        }
    }

    Component {
        id: listItem

        Rectangle {
            id: listItemBg
            width: ui.width
            height: hasSecondary ? 110 : 80
            color: isCurrentItem && ui.keyNavigationActive ? colors.dark : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: Qt.lighter(listItemBg.color, 1.3)
                width: 1
            }

            property bool isCurrentItem: ListView.isCurrentItem
            // optional roles: access via model.<role> so models without them keep working
            property bool hasSecondary: model.secondary !== undefined && model.secondary !== ""
            property bool hasRightText: model.rightText !== undefined && model.rightText !== ""

            Text {
                id: listItemText
                color: colors.offwhite
                text: name
                width: parent.width - 40 - (rightTextItem.visible ? rightTextItem.width + 20 : 0)
                elide: Text.ElideRight
                anchors {
                    left: parent.left; leftMargin: 20
                    verticalCenter: parent.verticalCenter
                    verticalCenterOffset: listItemBg.hasSecondary ? -18 : 0
                }
                font: fonts.primaryFont(30)
            }

            Text {
                id: secondaryText
                visible: listItemBg.hasSecondary
                color: colors.light
                text: listItemBg.hasSecondary ? model.secondary : ""
                width: listItemText.width
                elide: Text.ElideRight
                anchors { left: listItemText.left; top: listItemText.bottom }
                font: fonts.secondaryFont(24)
            }

            Text {
                id: rightTextItem
                visible: listItemBg.hasRightText
                color: colors.light
                text: listItemBg.hasRightText ? model.rightText : ""
                anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
                font: fonts.secondaryFont(26)
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    itemList.currentIndex = index;
                    itemList.positionViewAtIndex(index, ListView.Beginning);
                    itemSelected(value);
                    if (closeOnSelected) {
                        popupList.state = "hidden";
                    }
                }
            }
        }
    }

    Component {
        id: sectionHeader

        Item {
            width: ui.width
            height: 70

            Text {
                text: section.toUpperCase()
                color: colors.light
                width: parent.width - 40
                elide: Text.ElideRight
                anchors { left: parent.left; leftMargin: 20; bottom: parent.bottom; bottomMargin: 10 }
                font: fonts.secondaryFont(24, "Bold")
            }
        }
    }

    Component.onCompleted: {
        state = "visible";
    }
}
