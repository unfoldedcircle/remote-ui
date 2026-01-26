// Copyright (c) 2022-2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SELECT COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width
 - height
 - text
 - textColor
 - show name
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Entity.Controller 1.0

import "qrc:/components" as Components

Item {
    id: selectWidget
    width: 80
    height: 80

    property string customText
    property string entityId
    property QtObject entityObj
    property bool showName: false

    function ensureEntityLoaded() {
        entityObj = EntityController.get(entityId);

        if (!entityObj) {
            EntityController.load(entityId);
        }

        evaluateSelect();
    }

    function evaluateSelect() {
        if (!entityObj) {
            selectNameText.text = qsTranslate("Abbreviation for not available", "N/A");
            selectOptionText.text = qsTranslate("Abbreviation for not available", "N/A");
            return;
        }

        if (selectWidget.showName) {
            selectNameText.text = entityObj.name;
        } else {
            selectNameText.text = selectWidget.customText;
        }

        if (entityObj.currentOption == "") {
            selectOptionText.text = qsTranslate("Abbreviation for nothing is selected", "None");
        } else {
            selectOptionText.text = entityObj.currentOption;
        }

        optionsList.model = entityObj.options;

        selectWidget.selectCurrent();
    }

    function selectOption(option) {
        entityObj.selectOption(option);
    }

    function selectCurrent() {
        for (let i = 0; i < entityObj.options.length; i++) {
            if (entityObj.options[i] === entityObj.currentOption) {
                optionsList.currentIndex = i;
                optionsList.positionViewAtIndex(i, ListView.Center);
                break;
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ColumnLayout {
            spacing: 4
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Components.Icon {
                    color: colors.red
                    icon: "uc:link-slash"
                    size: 40
                    visible: entityObj && !entityObj.enabled
                }

                Item { Layout.preferredWidth: (entityObj && !entityObj.enabled) ? 2 : 0 }

                Text {
                    id: selectNameText
                    text: ""
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    color: colors.light
                    verticalAlignment: Text.AlignVCenter
                    font: fonts.primaryFont(24)
                }

                Item { Layout.fillWidth: true }
            }

            Text {
                id: selectOptionText
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter

                text: ""
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
                elide: Text.ElideRight
                color: colors.offwhite
                opacity: entityObj.currentOption == "" ? 0.3 : 1
                verticalAlignment: Text.AlignVCenter
                font: fonts.primaryFont(36)
            }
        }

        Item { Layout.fillWidth: true }

        Components.Icon {
            color: colors.offwhite
            icon: "uc:chevron-down"
            size: 60
            Layout.leftMargin: 10
        }
    }

    Component.onCompleted: ensureEntityLoaded()
    onEntityIdChanged: ensureEntityLoaded()

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onEntityLoaded(success, loadedId) {
            if (!success || loadedId !== selectWidget.entityId) {
                return;
            }

            selectWidget.entityObj = EntityController.get(loadedId);
            selectWidget.evaluateSelect();
        }
    }

    Connections {
        target: selectWidget.entityObj
        ignoreUnknownSignals: true

        function onCurrentOptionChanged() {
            selectWidget.evaluateSelect();
        }

        function onOptionsChanged() {
            selectWidget.evaluateSelect();
        }
    }

    Components.HapticMouseArea {
        anchors.fill: parent
        onClicked: {
            sourceListPopup.open();
        }
    }

    Popup {
        id: sourceListPopup
        width: parent.width; height: parent.height
        y: 500
        opacity: 0
        modal: false
        closePolicy: Popup.CloseOnPressOutside
        padding: 0
        parent: Overlay.overlay

        enter: Transition {
            SequentialAnimation {
                ParallelAnimation {
                    PropertyAnimation { properties: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
                    PropertyAnimation { properties: "y"; from: 500; to: 0; easing.type: Easing.OutExpo; duration: 300 }
                }
            }
        }

        exit: Transition {
            SequentialAnimation {
                PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 300 }
                PropertyAnimation { properties: "y"; from: 0; to: 500; duration: 0 }
            }
        }

        onOpened: {
            buttonNavigation.takeControl();
            selectWidget.selectCurrent();
        }

        onClosed: {
            buttonNavigation.releaseControl();
        }

        Components.ButtonNavigation {
            id: buttonNavigation
            defaultConfig: {
                "HOME": {
                    "pressed": function() {
                        sourceListPopup.close();
                    }
                },
                "BACK": {
                    "pressed": function() {
                        sourceListPopup.close();
                    }
                },
                "DPAD_LEFT": {
                    "pressed": function() {
                        entityObj.selectPrevious();
                        sourceListPopup.close();
                    }
                },
                "DPAD_RIGHT": {
                    "pressed": function() {
                        entityObj.selectNext();
                        sourceListPopup.close();
                    }
                },
                "PREV": {
                    "pressed": function() {
                        entityObj.selectFirst();
                        sourceListPopup.close();
                    }
                },
                "NEXT": {
                    "pressed": function() {
                        entityObj.selectLast();
                        sourceListPopup.close();
                    }
                },
                "DPAD_UP": {
                    "pressed": function() {
                        optionsList.decrementCurrentIndex();
                    }
                },
                "DPAD_DOWN": {
                    "pressed": function() {
                        optionsList.incrementCurrentIndex();
                    }
                },
                "DPAD_MIDDLE": {
                    "pressed": function() {
                        selectWidget.selectOption(optionsList.currentItem.optionName);
                        sourceListPopup.close();
                    }
                },
            }
        }

        background: Rectangle {
            color: colors.black
            opacity: 0.8
        }

        contentItem: Rectangle {
            color: colors.black
            radius: ui.cornerRadiusLarge

            Components.Icon {
                id: iconClose
                color: colors.offwhite
                icon: "uc:xmark"
                anchors { right: parent.right; top: parent.top; topMargin: 5 }
                size: 70

                Components.HapticMouseArea {
                    width: parent.width + 20; height: parent.height + 20
                    anchors.centerIn: parent
                    onClicked: {
                        sourceListPopup.close();
                    }
                }
            }

            Text {
                id: optionPopupTitle
                text: qsTr("Select an option")
                color: colors.offwhite
                font: fonts.primaryFont(24, "Medium")
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                elide: Text.ElideRight
                maximumLineCount: 2
                lineHeight: 0.8
                anchors { left: parent.left; leftMargin: 20; right: iconClose.left; rightMargin: 20; verticalCenter: iconClose.verticalCenter; }
            }

            ListView {
                id: optionsList
                clip: true
                pressDelay: 100
                keyNavigationEnabled: true
                focus: true
                model: entityObj.options
                anchors { left: parent.left; leftMargin: 10; right: parent.right; rightMargin: 10; top: optionPopupTitle.bottom; topMargin: 20; bottom: parent.bottom; bottomMargin: 20 }

                delegate: Components.HapticMouseArea {
                    width: parent.width
                    height: 120

                    property string optionName: modelData

                    onClicked: {
                        selectWidget.selectOption(modelData);
                        sourceListPopup.close();
                    }

                    Rectangle {
                        width: parent.width - 20
                        height: parent.height - 20
                        anchors.centerIn: parent
                        color: optionsList.currentIndex == index ? colors.offwhite : colors.transparent
                        radius: ui.cornerRadiusSmall

                        Text {
                            text: modelData
                            color: optionsList.currentIndex == index ? colors.dark : colors.offwhite
                            font: fonts.primaryFont(26)
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            padding: 20
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Components.ScrollIndicator {
                parentObj: optionsList
                hideOverride: optionsList.atYEnd
            }
        }
    }
}
