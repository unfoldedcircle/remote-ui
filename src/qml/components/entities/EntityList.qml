// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import Haptic 1.0

import Entity.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: entityList
    color: colors.black

    signal closed()

    property bool inputHasFocus: false
    property var openTrigger: function() {}
    property var closeTrigger: function() {}
    property var okTrigger: function() {}
    property string okText: qsTr("Add")
    property bool entityDescriptionIntegration: false
    property bool closeListOnTrigger: true
    property bool showCloseIcon: false
    property string integrationId

    property alias title: titleText.text
    property alias description: descriptionText.text
    property alias itemList: itemList
    property alias count: itemList.count
    property QtObject model

    function open() {
        entityList.model.init(entityList.integrationId);
        stopLoading();
        openTrigger();
    }

    function close() {
        keyboard.hide();
        closeTrigger();
        entityList.closed();
        entityList.model.clear();
    }

    function startLoading() {
        loadingIndicatorTimeoutTimer.start();
        loadingIndicator.state = "visible";
    }

    function stopLoading() {
        loadingIndicatorTimeoutTimer.stop();
        loadingIndicator.state = "hidden";
    }

    function loadMoreItems() {
        if (entityList.model.canLoadMore()) {
            entityList.startLoading();
            entityList.model.loadMore();
        }
    }

    Connections {
        target: entityList.model

        function onEntitiesLoaded(count) {
            entityList.stopLoading();
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Item {
        id: titleContainer
        width: parent.width
        height: childrenRect.height + 20
        z: 200
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

        Text {
            id: titleText
            height: text == "" ? 0 : titleText.implicitHeight
            width: parent.width
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            horizontalAlignment: Text.AlignHCenter
            anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 10 }
            font: fonts.primaryFont(30)
        }

        Components.Icon {
            size: 60
            icon: "uc:close"
            color: colors.offwhite
            anchors { top: parent.top; right: parent.right }
            visible: entityList.showCloseIcon

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    entityList.close();
                }
            }
        }

        Text {
            id: descriptionText
            height: text == "" ? 0 : descriptionText.implicitHeight
            width: parent.width
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.light
            horizontalAlignment: Text.AlignHCenter
            anchors { horizontalCenter: parent.horizontalCenter; top:titleText.bottom; topMargin: 5 }
            font: fonts.secondaryFont(24)
        }

        RowLayout {
            id: searchFieldContainer
            width: parent.width - 20
            clip: true
            spacing: 10
            anchors { top: descriptionText.bottom; horizontalCenter: parent.horizontalCenter }

            Behavior on height {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 200 }
            }

            Components.SearchField {
                id: entitySearch

                Layout.fillWidth: true

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
                    entityList.model.search(inputField.text);
                }
            }

            Rectangle {
                Layout.preferredHeight: entitySearch.height
                Layout.preferredWidth: entitySearch.height

                color: entityList.model.filtered ? colors.highlight : colors.transparent
                radius: ui.cornerRadiusSmall

                Behavior on color {
                    ColorAnimation { duration: 300 }
                }

                Components.Icon {
                    icon: "uc:filter"
                    color: entityList.model.filtered ? colors.black : colors.offwhite
                    size: 80
                    anchors.centerIn: parent

                    Behavior on color {
                        ColorAnimation { duration: 300 }
                    }
                }

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        entityFilterPopup.open();
                    }
                }
            }
        }

        Popup {
            id: entityFilterPopup

            parent: Overlay.overlay
            width: parent.width; height: parent.height
            modal: false
            closePolicy: Popup.CloseOnPressOutside
            padding: 0

            onOpened: {
                entityFilterPopupButtonNavigation.takeControl();
            }

            onClosed: {
                entityFilterPopupButtonNavigation.releaseControl();
            }

            enter: Transition {
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
                NumberAnimation { target: entityFilterContainer; property: "anchors.bottomMargin"; from: -entityFilterContainer.height; to: -ui.cornerRadiusLarge; easing.type: Easing.OutExpo; duration: 300 }
            }

            exit: Transition {
                NumberAnimation { target: entityFilterContainer; property: "anchors.bottomMargin"; from: -ui.cornerRadiusLarge; to: -entityFilterContainer.height; easing.type: Easing.InExpo; duration: 300 }
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 300 }
            }

            Components.ButtonNavigation {
                id: entityFilterPopupButtonNavigation
            }

            background: Rectangle {
                color: colors.black; opacity: 0.6

                MouseArea {
                    anchors.fill: parent
                    onClicked: entityFilterPopup.close()
                }
            }

            contentItem: Item {
                Rectangle {
                    id: entityFilterContainer
                    width: parent.width
                    height: entityFilterContainerContent.height + ui.cornerRadiusLarge
                    radius: ui.cornerRadiusLarge
                    color: colors.dark
                    anchors.bottom: parent.bottom

                    MouseArea {
                        anchors.fill: parent
                    }

                    ColumnLayout {
                        id: entityFilterContainerContent

                        width: parent.width
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20

                            Components.HapticMouseArea {
                                Layout.preferredHeight: 80
                                Layout.preferredWidth: parent.width / 3

                                onClicked: {
                                    entityList.model.cleanEntityTypes();

                                    for (let i = 0; i < filterTypesListView.count; i++) {
                                        filterTypesListView.model.get(i).typeChecked = false;
                                    }
                                }

                                Text {
                                    anchors.fill: parent
                                    text: qsTr("Clear")
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    color: colors.light
                                    font: fonts.secondaryFont(24)
                                }
                            }

                            Text {
                                Layout.fillWidth: true

                                text: qsTr("Filters")
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                maximumLineCount: 1
                                color: colors.offwhite
                                font: fonts.primaryFont(26)
                            }

                            Components.HapticMouseArea {
                                Layout.preferredHeight: 80
                                Layout.preferredWidth: parent.width / 3

                                onClicked: {
                                    entityFilterPopup.close();
                                }

                                Text {
                                    anchors.fill: parent
                                    text: qsTr("Done")
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                    maximumLineCount: 1
                                    color: colors.light
                                    font: fonts.secondaryFont(24)
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            Layout.topMargin: -10
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            color: Qt.lighter(colors.medium, 1.3)
                        }

                        ListView {
                            id: filterTypesListView
                            Layout.fillWidth: true
                            Layout.preferredHeight: (count + 1) * 60
                            Layout.topMargin: 10
                            Layout.leftMargin: 20
                            Layout.rightMargin: 20
                            Layout.bottomMargin: 20

                            interactive: false
                            spacing: 10
                            model: ListModel {
                                id: filterTypesListModel

                                ListElement {
                                    typeName: qsTr("Button")
                                    typeIcon: "uc:power-on"
                                    typeValue: EntityTypes.Button
                                    typeChecked: false
                                }
                                ListElement {
                                    typeName: qsTr("Climate")
                                    typeIcon: "uc:climate"
                                    typeValue: EntityTypes.Climate
                                    typeChecked: false
                                }
                                ListElement {
                                    typeName: qsTr("Cover")
                                    typeIcon: "uc:blind"
                                    typeValue: EntityTypes.Cover
                                    typeChecked: false
                                }
                                ListElement {
                                    typeName: qsTr("Light")
                                    typeIcon: "uc:light"
                                    typeValue: EntityTypes.Light
                                    typeChecked: false
                                }
                                ListElement {
                                    typeName: qsTr("Media player")
                                    typeIcon: "uc:music"
                                    typeValue: EntityTypes.Media_player
                                    typeChecked: false
                                }
                                ListElement {
                                    typeName: qsTr("Sensor")
                                    typeIcon: "uc:sensor"
                                    typeValue: EntityTypes.Sensor
                                    typeChecked: false
                                }
                                ListElement {
                                    typeName: qsTr("Switch")
                                    typeIcon: "uc:power-on"
                                    typeValue: EntityTypes.Switch
                                    typeChecked: false
                                }
                            }

                            delegate: RowLayout {
                                id: filterListViewDelegate
                                width: ListView.view.width
                                height: 60
                                spacing: 20

                                property bool checked: entityList.model.containsEntityType(typeValue)
                                Component.onCompleted: typeChecked = false// entityList.model.containsEntityType(typeValue)

                                Components.Icon {
                                    size: 60
                                    color: colors.offwhite
                                    icon: typeIcon
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 60
                                    text: typeName
                                    color: colors.offwhite
                                    verticalAlignment: Text.AlignVCenter
                                    font: fonts.primaryFont(28)
                                }

                                Components.HapticMouseArea {
                                    Layout.preferredWidth: 60
                                    Layout.preferredHeight: 60
                                    Layout.alignment: Qt.AlignRight

                                    onClicked: {
                                        if (typeChecked) {
                                            entityList.model.removeEntityType(typeValue);
                                        } else {
                                            entityList.model.setEntityType(typeValue);
                                        }
                                        typeChecked = entityList.model.containsEntityType(typeValue);
                                    }

                                    Rectangle {
                                        width: 30
                                        height: width
                                        anchors.centerIn: parent
                                        color: typeChecked ? colors.white : colors.transparent
                                        radius: width / 2
                                        border {
                                            width: 1
                                            color: colors.light
                                        }

                                        Behavior on color {
                                            ColorAnimation { duration: 300 }
                                        }

                                        Components.Icon {
                                            size: parent.width
                                            color: colors.black
                                            icon: "uc:check"
                                            opacity: typeChecked ? 1 : 0

                                            Behavior on opacity {
                                                NumberAnimation { duration: 300 }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }

    ListView {
        id: itemList
        width: parent.width
        height: parent.height - titleContainer.height
        anchors { horizontalCenter: parent.horizontalCenter; top: titleContainer.bottom; topMargin: 10; bottom: footer.top }
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        pressDelay: 200

        interactive: loadingIndicator.state == "hidden"

        model: entityList.model
        delegate: listItem

        onCurrentIndexChanged: {
            positionViewAtIndex(currentIndex, ListView.Contain);
            console.debug("Current index changed: " + currentIndex)
        }

        ScrollBar.vertical: ScrollBar {
            opacity: 0.5
        }

        currentIndex: 0

        property real lastY: 0

        //        onContentYChanged: {
        //            if(!moving){
        //                itemList.contentY = itemList.lastY+itemList.originY
        //            }
        //            itemList.lastY = itemList.contentY-itemList.originY
        //        }

        Text {
            //: No entities are in this list
            text: qsTr("No entities")
            color: colors.offwhite
            anchors { top: parent.top; topMargin: 80; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(30)
            visible: itemList.count == 0
        }

        property bool loadMore: false

        onFlickStarted: {
            if (verticalVelocity > 0 && searchFieldContainer.height != 0) {
                searchFieldContainer.height = 0;
            } else if (verticalVelocity < 0 && searchFieldContainer.height == 0) {
                searchFieldContainer.height = searchFieldContainer.childrenRect.height + 20;
            }
        }

        onFlickEnded: {
            if (atYEnd) {
                entityList.loadMoreItems();
            }
        }
    }

    Item {
        width: parent.width; height: 60
        anchors { bottom: footer.top; horizontalCenter: parent.horizontalCenter }
        opacity: itemList.atYEnd ? 0 : 1

        Behavior on opacity { PropertyAnimation { duration: 300; easing.type: Easing.OutExpo } }

        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, 50)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: colors.black }
            }
        }
    }

    Components.ScrollIndicator {
        parentObj: itemList
    }

    Rectangle {
        id: footer
        width: parent.width
        height: 80
        color: colors.black
        anchors.bottom: parent.bottom

        Components.Button {
            id: buttonOk
            text: entityList.okText
            width: (parent.width - 20 ) / 2
            anchors { right: parent.right; bottom: parent.bottom }
            trigger: function() {
                entityList.okTrigger();

                if (entityList.closeListOnTrigger) {
                    entityList.close();
                }
            }
            opacity: itemList.count == 0 ? 0.3 : 1
            enabled: itemList.count != 0
        }

        Components.Button {
            text: entityList.model.allSelected ? qsTr("Clear") : qsTr("Select all")
            width: (parent.width - 20 ) / 2
            color: colors.secondaryButton
            anchors { left: parent.left; bottom: parent.bottom }
            trigger: function() {
                if (entityList.model.allSelected) {
                    entityList.model.clearSelected();
                } else {
                    entityList.model.selectAll();
                }
            }
            opacity: itemList.count == 0 ? 0.3 : 1
            enabled: itemList.count != 0
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: inputHasFocus
        onClicked: {
            keyboard.hide();
            entityList.inputHasFocus = false;
        }
    }

    Rectangle {
        id: loadingIndicator
        anchors.fill: itemList
        color: colors.black

        state: "visible"

        states: [
            State {
                name: "hidden"
                PropertyChanges { target: loadingIndicator; opacity: 0 }
                PropertyChanges { target: loadingIndicator; visible: false }
            },
            State {
                name: "visible"
                PropertyChanges { target: loadingIndicator; opacity: 1 }
                PropertyChanges { target: loadingIndicator; visible: true }
            }
        ]
        transitions: [
            Transition {
                to: "hidden"
                SequentialAnimation {
                    PropertyAnimation { target: loadingIndicator; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                    PropertyAnimation { target: loadingIndicator; properties: "visible" }
                }
            },
            Transition {
                to: "visible";
                PropertyAnimation { target: loadingIndicator; properties: "visible" }
                PropertyAnimation { target: loadingIndicator; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        ]

        Text {
            id: loadingIndicatorText
            //: The application is loading
            text: qsTr("Loading")
            color: colors.offwhite
            anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter; verticalCenterOffset: -100 }
            font: fonts.primaryFont(30)
        }

        Item {
            id: entitiesLoadingIndicator
            width: 40; height: 40
            anchors { top: loadingIndicatorText.bottom; topMargin: 10;  horizontalCenter: parent.horizontalCenter }

            property int circleSize: 14

            Rectangle {
                id: fillCircle
                width: entitiesLoadingIndicator.circleSize; height: entitiesLoadingIndicator.circleSize
                radius: 7
                color: colors.offwhite
                x: 0
                z: 1
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: outlineCircle
                width: entitiesLoadingIndicator.circleSize; height: entitiesLoadingIndicator.circleSize
                radius: entitiesLoadingIndicator.circleSize/2
                color: colors.black
                border { color: colors.offwhite; width: 2 }
                x: 20
                z: 2
                anchors.verticalCenter: parent.verticalCenter
            }

            SequentialAnimation {
                id: integrationLoadinganimation
                running: loadingIndicator.visible
                loops: Animation.Infinite

                ParallelAnimation {
                    NumberAnimation { target: fillCircle; properties: "z"; to: 1; duration: 1  }
                    NumberAnimation { target: outlineCircle; properties: "z"; to: 2; duration: 1  }
                }

                ParallelAnimation {
                    NumberAnimation { target: fillCircle; properties: "x"; to: entitiesLoadingIndicator.circleSize; easing.type: Easing.OutExpo; duration: 400  }
                    NumberAnimation { target: outlineCircle; properties: "x"; to: 0; easing.type: Easing.OutExpo; duration: 400  }
                }

                PauseAnimation { duration: 500 }

                ParallelAnimation {
                    NumberAnimation { target: fillCircle; properties: "z"; to: 2; duration: 1  }
                    NumberAnimation { target: outlineCircle; properties: "z"; to: 1; duration: 1  }
                }

                ParallelAnimation {
                    NumberAnimation { target: fillCircle; properties: "x"; to: 0; easing.type: Easing.OutExpo; duration: 400  }
                    NumberAnimation { target: outlineCircle; properties: "x"; to: entitiesLoadingIndicator.circleSize; easing.type: Easing.OutExpo; duration: 400  }
                }

                PauseAnimation { duration: 500 }
            }
        }

        Timer {
            id: loadingIndicatorTimeoutTimer
            running: loadingIndicator.visible
            repeat: false
            interval: 5000

            onTriggered: entityList.stopLoading()
        }
    }

    Component {
        id: listItem

        Rectangle {
            width: ListView.view.width; height: entityInfoContainer.height + 40
            color: isCurrentItem && ui.keyNavigationEnabled ? colors.dark : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: isCurrentItem && ui.keyNavigationEnabled ? colors.medium : colors.transparent
                width: 1
            }

            property bool isCurrentItem: ListView.isCurrentItem
            property string key: itemKey
            property bool selected: itemSelected

            Components.Icon {
                id: entityIcon
                color: colors.offwhite
                icon: itemIcon
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter; }
                size: 80
            }

            Item {
                id: entityInfoContainer
                width: ui.width - entityIcon.size - 110
                height: childrenRect.height
                anchors { left: entityIcon.right; leftMargin: 10; verticalCenter: parent.verticalCenter; }

                Text {
                    id: entityTitle
                    text: itemName
                    width: parent.width
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    color: colors.offwhite
                    anchors { left: parent.left; }
                    font: fonts.primaryFont(26)
                    lineHeight: 0.8
                }

                Text {
                    text: itemKey
                    width: parent.width
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.light
                    anchors { left: parent.left; top: entityTitle.bottom; topMargin: 5 }
                    font: fonts.secondaryFont(20)
                    lineHeight: 0.8
                }
            }

            Item {
                width: 100; height: 100
                anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 10 }

                states: [
                    State {
                        name: "selected"
                        when: itemSelected
                        PropertyChanges { target: small; width: 24 }
                        PropertyChanges { target: large; height: 48 }
                    }
                ]
                transitions: [
                    Transition {
                        to: "selected"
                        reversible: true
                        SequentialAnimation {
                            PropertyAnimation { target: small; properties: "width"; easing.type: Easing.OutExpo; duration: 75 }
                            PropertyAnimation { target: large; properties: "height"; easing.type: Easing.OutExpo; duration: 100 }
                        }
                    }
                ]

                Item {
                    anchors { horizontalCenter: parent.horizontalCenter; horizontalCenterOffset: -10; verticalCenter: parent.verticalCenter }
                    rotation: 45
                    transformOrigin: Item.Center

                    Rectangle {
                        id: small
                        width: 0
                        height: 4
                        color: colors.offwhite
                    }

                    Rectangle {
                        id: large
                        width: 4
                        height: 0
                        color: colors.offwhite
                        anchors { bottom: small.bottom; right: small.right }
                    }
                }
            }

            Components.HapticMouseArea {
                anchors.fill: parent

                onClicked: {
                    entityList.model.setSelected(itemKey, !itemSelected);
                    entityList.itemList.currentIndex = index;

                }
            }
        }
    }
}
