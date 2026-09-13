// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Integration.Controller 1.0
import Settings.SchemaTypes 1.0

import "qrc:/components" as Components

Item {
    id: root

    property alias title: title.text
    property var settings
    property var inputObjects: []

    /** KEYBOARD NAVIGATION **/
    // The created fields are chained through their navUp / navDown; the last one leads to the
    // control the hosting form passes as navExit (its Next button).
    property alias flickable: contentFlickable
    property Item firstFocusItem: null
    property Item lastFocusItem: null
    property Item navExit: null

    function chainFields() {
        const chain = inputObjects.filter(function(obj) { return obj.focusItem; });

        for (let i = 0; i < chain.length; i++) {
            chain[i].navUp = i > 0 ? chain[i - 1].focusItem : null;
            chain[i].navDown = i < chain.length - 1 ? chain[i + 1].focusItem : null;
        }

        if (chain.length > 0) {
            root.firstFocusItem = chain[0].focusItem;
            root.lastFocusItem = chain[chain.length - 1].focusItem;
            chain[chain.length - 1].navDown = Qt.binding(function() { return root.navExit; });
        }
    }

    function getData() {
        let data = {};

        inputObjects.forEach((obj) => {
                                 data[obj.labelId] = String(obj.value);
                             });
        return data;
    }

    Component.onCompleted: {
        let component, obj;

        settings.forEach((item) => {
                             switch (item.type) {
                                 case SettingsSchemaTypes.Number:
                                 component = Qt.createComponent("qrc:/components/integrations/fields/Number.qml");
                                 obj = component.createObject(content, {
                                                                  labelId: item.id,
                                                                  labelText: item.label,
                                                                  value: item.value
                                                              });
                                 inputObjects.push(obj);
                                 break;

                                 case SettingsSchemaTypes.Text:
                                 component = Qt.createComponent("qrc:/components/integrations/fields/TextInput.qml");
                                 obj = component.createObject(content, {
                                                                  labelId: item.id,
                                                                  labelText: item.label,
                                                                  value: item.value
                                                              });
                                 inputObjects.push(obj);
                                 break;

                                 case SettingsSchemaTypes.Textarea:
                                 component = Qt.createComponent("qrc:/components/integrations/fields/TextArea.qml");
                                 obj = component.createObject(content, {
                                                                  labelId: item.id,
                                                                  labelText: item.label,
                                                                  value: item.value
                                                              });
                                 inputObjects.push(obj);
                                 break;

                                 case SettingsSchemaTypes.Password:
                                 component = Qt.createComponent("qrc:/components/integrations/fields/Password.qml");
                                 obj = component.createObject(content, {
                                                                  labelId: item.id,
                                                                  labelText: item.label,
                                                                  value: item.value
                                                              });
                                 inputObjects.push(obj);
                                 break;

                                 case SettingsSchemaTypes.Checkbox:
                                 component = Qt.createComponent("qrc:/components/integrations/fields/Checkbox.qml");
                                 obj = component.createObject(content, {
                                                                  labelId: item.id,
                                                                  labelText: item.label,
                                                                  value: item.value
                                                              });
                                 inputObjects.push(obj);
                                 break;

                                 case SettingsSchemaTypes.Dropdown:
                                 component = Qt.createComponent("qrc:/components/integrations/fields/Dropdown.qml");
                                 obj = component.createObject(content, {
                                                                  labelId: item.id,
                                                                  labelText: item.label,
                                                                  value: item.value,
                                                                  model: item.model
                                                              });
                                 inputObjects.push(obj);
                                 break;

                                 case SettingsSchemaTypes.Label:
                                 component = Qt.createComponent("qrc:/components/integrations/fields/Label.qml");
                                 obj = component.createObject(content, {
                                                                  labelId: item.id,
                                                                  labelText: item.label,
                                                                  value: item.value
                                                              });
                                 break;
                             }
                         });

        root.chainFields();
    }

    Text {
        id: title

        width: parent.width - 40
        height: visible ? undefined : 0
        color: colors.offwhite
        maximumLineCount: 2
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        font: fonts.primaryFont(30)
        anchors { top: parent.top; topMargin: title.visible ? 20 : 0; horizontalCenter: parent.horizontalCenter }
        visible: title.text !== ""
    }

    Flickable {
        id: contentFlickable

        width: parent.width - 40
        clip: true
        contentHeight: content.height
        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        anchors { top: title.bottom; topMargin: 20; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        // the on-screen keyboard covers the lower part of the screen: keep the focused field above
        // it (computed from the page's own bottom edge, the flickable's height depends on this)
        anchors.bottomMargin: keyboard.active
                              ? Math.max(0, root.mapToItem(null, 0, root.height).y - keyboard.visibleY)
                              : 0

        ScrollBar.vertical: ScrollBar {
            opacity: 0.5
        }

        ColumnLayout {
            id: content
            spacing: 60
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Connections {
        target: IntegrationController
        ignoreUnknownSignals: true

        function onIntegrationUserDataError(labelId, error) {
            inputObjects.forEach((obj) => {
                                     if (obj.labelId === labelId) {
                                         obj.showError(error);
                                         // the keypad user lands on the field to correct
                                         if (obj.focusItem) {
                                             Qt.callLater(function() { obj.focusItem.forceActiveFocus(); });
                                         }
                                     }
                                 });
        }

    }
}
