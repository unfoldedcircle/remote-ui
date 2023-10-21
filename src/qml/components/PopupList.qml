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

    property alias popupListmodel: popupListmodel

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
                "released": function() {
                    itemSelected(itemList.model.get(itemList.currentIndex).value);
                    if (closeOnSelected) {
                        popupList.state = "hidden";
                    }
                }
            },
            "BACK": {
                "released": function() {
                    if (!hideClose) {
                        popupList.state = "hidden";
                    }
                }
            },
            "HOME": {
                "released": function() {
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
            icon: "uc:close"
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

        function reload() {
            popupListmodel.clear();

            for (var i = 0; i < listModel.count; i++) {
                popupListmodel.append(listModel.get(i));
            }

            itemList.currentIndex = popupList.initialSelected;
        }

        function applyFilter(searchCriteria) {
            popupListmodel.clear();

            console.debug("Search length: " + searchCriteria.length);

            for (var i = 0; i < listModel.count; i++) {
                let str;
                if (searchCriteria.length === 2 && popupList.countryList) {
                    str = listModel.get(i).name.slice(0, 2);
                } else {
                    str = listModel.get(i).name;
                }
                if (str.toLowerCase().indexOf(searchCriteria.toLowerCase()) > -1) {
                    popupListmodel.append(listModel.get(i));
                }
            }
        }
    }

    Component {
        id: listItem

        Rectangle {
            id: listItemBg
            width: ui.width
            height: 80
            color: isCurrentItem && ui.keyNavigationEnabled ? colors.dark : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: Qt.lighter(listItemBg.color, 1.3)
                width: 1
            }

            property bool isCurrentItem: ListView.isCurrentItem

            Text {
                id: listItemText
                color: colors.offwhite
                text: name
                width: parent.width - 20
                elide: Text.ElideRight
                anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter; }
                font: fonts.primaryFont(30)
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

    Component.onCompleted: {
        state = "visible";
    }
}
