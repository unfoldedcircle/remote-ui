// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Group.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: container
    width: ui.width - 20; height: groups.count * 140 + title.height
    border { color: isSelected ? colors.medium : colors.transparent; width: 1 }
    radius: ui.cornerRadiusSmall
    clip: true

    state: "closed"

    states: [
        State {
            name: "open"
            PropertyChanges {target: groups; opacity: 1 }
            PropertyChanges {target: container; height:  groups.count * 140 + titleContainer.height + 40 + (editMode ? 20 : 0); color: colors.black }
            PropertyChanges {target: titleContainer; height: 120 }
        },
        State {
            name: "closed"
            PropertyChanges {target: groups; opacity: 0 }
            PropertyChanges {target: container; height: 130; color: isSelected ? Qt.darker(colors.medium) : colors.black }
            PropertyChanges {target: titleContainer; height: 130 }
        }
    ]

    transitions: [
        Transition {
            to: "open"
            ParallelAnimation {
                PropertyAnimation { target: groups; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { target: container; properties: "height"; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { target: titleContainer; properties: "height"; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {
            to: "closed"
            ParallelAnimation {
                PropertyAnimation { target: groups; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { target: container; properties: "height"; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { target: titleContainer; properties: "height"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    property alias groups: groups

    property string groupId
    property QtObject groupObj
    property bool isHighLightEnabled: parent.isCurrentItem
    property bool isSelected: isHighLightEnabled ? parent.isCurrentItem : false
    property bool started: false
    property bool editMode

    onIsHighLightEnabledChanged: {
        if (isHighLightEnabled) {
            enableHighLight(true);
        } else {
            enableHighLight(false);
        }
    }

    onIsSelectedChanged: {
        if (container.isSelected && !container.started) {
            changeCurrentIndexTimer.start();
            container.started = true;
        }
    }

    onEditModeChanged: {
        if (editMode) {
            close();
        }
    }

    Connections {
        target: parent
        ignoreUnknownSignals: true

        function onEditModeChanged() {
            editMode = parent.editMode;
        }
    }

    function open() {
        if (groups.count !== 0) {
            container.state = "open";
        }
    }

    function close() {
        container.state = "closed";
    }

    function enableHighLight(value) {
        for (let i = 0; i < groups.count; i++) {
            if (groups.itemAtIndex(i)) {
                groups.itemAtIndex(i).item.isHighLightEnabled = value;
            }
        }
    }

    function turnOnGroupItems(value) {
        for (let i = 0; i < groups.count; i++) {
            if (groups.itemAtIndex(i)) {
                if (!value) {
                    groups.itemAtIndex(i).item.entityObj.turnOff();
                } else {
                    groups.itemAtIndex(i).item.entityObj.turnOn();
                }
            }
        }
    }

    function toggle() {
        turnOnGroupItems(!onOffSwitch.checked);
    }

    Timer {
        id: changeCurrentIndexTimer
        running: false
        interval: 100
        repeat: false

        onTriggered: groups.currentIndex = 0
    }

    Timer {
        id: setupHighlightTimer
        running: false
        repeat: false
        interval: 500

        onTriggered: {
            if (isHighLightEnabled) {
                enableHighLight(true);
            } else {
                enableHighLight(false);
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.darker(colors.dark, 2)
        radius: ui.cornerRadiusSmall
        opacity: container.state == "open" ? 1 : 0

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
        }
    }

    Components.HapticMouseArea {
        width: !editMode ? parent.width : parent.width - 100
        height: parent.height
        onClicked: {
            if (container.state == "open") {
                close();
            } else {
                open();
            }
        }
        onPressAndHold: {
            if (!ui.editMode) {
                if (ui.profile.restricted) {
                    ui.createNotification(qsTr("Profile is restricted"), true);
                } else {
                    root.containerMainItem.openGroupEditMenu(groupObj);
                }
            }
        }
    }

    Components.Icon {
        id: icon
        color: colors.offwhite
        icon: "uc:up-arrow"
        anchors { left: parent.left; leftMargin: 10; verticalCenter: titleContainer.verticalCenter; }
        size: container.state == "open" ? 60 : 100
        rotation: container.state == "open" ? 0 : 180
        opacity: groups.count === 0 ? 0.2 : 1

        Behavior on width {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
        }

        Behavior on rotation {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
        }

    }

    Item {
        id: titleContainer
        height: 60
        anchors { left: icon.right; leftMargin: 20; right: onOffSwitch.left; top: parent.top }

        Text {
            id: title
            text: groupObj.name
            width: titleContainer.width
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            color: colors.offwhite
            anchors { verticalCenter: parent.verticalCenter; verticalCenterOffset: container.state == "closed" ? -15 : 0 }
            font: fonts.primaryFont(30)
        }

        Text {
            id: moreInfo
            width: title.width
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 1
            //: Tap and hold down to edit a group
            text: groups.count === 1 ? qsTr("%1 entity").arg(1) : qsTr("%1 entities").arg(groups.count)
            color: colors.offwhite
            opacity: 0.6
            anchors { left: title.left; top: title.bottom }
            font: fonts.secondaryFont(22, "Medium")
            lineHeight: 0.8
            visible: container.state == "closed"
        }
    }

    Components.Switch {
        id: onOffSwitch
        checked: false
        anchors { right: parent.right; rightMargin: 10; verticalCenter: titleContainer.verticalCenter }
        visible: !editMode && groups.count !== 0
        trigger: function() {
            container.turnOnGroupItems(!checked);
        }
    }

    ListView {
        id: groups
        width: parent.width-20; height: groups.count * 140
        anchors { top: titleContainer.bottom; horizontalCenter: parent.horizontalCenter }

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        interactive: false

        model: groupObj.groupItems()

        delegate: groupItem

        spacing: 10
    }

    ButtonGroup {
        id: entitySwitchGroup
        exclusive: false

        onCheckStateChanged: {
            if (checkState != Qt.Unchecked) {
                onOffSwitch.checked = true;
            } else {
                onOffSwitch.checked = false;
            }
        }
    }

    Component {
        id: groupItem

        Loader {
            id: groupItemLoader
            asynchronous: true
            enabled: !editMode

            property bool isCurrentItem: ListView.isCurrentItem

            Component.onCompleted: {
                this.setSource("qrc:/components/entities/Base.qml", { "parentInputController": containerMain, "entityId":groupItemId, "isInGroup": true, "parentGroupId": groupId});
            }

            onStatusChanged: {
                if (status == Loader.Ready) {
                    item.isHighLightEnabled = false;
                    entitySwitchGroup.addButton(groupItemLoader.item.button);
                }
            }
        }
    }

    Component.onCompleted: {
        setupHighlightTimer.start();
        groupObj = GroupController.get(groupId);
    }
}
