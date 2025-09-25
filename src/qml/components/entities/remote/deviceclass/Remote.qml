// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Entity.Remote 1.0

import Entity.Controller 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: remoteBase

    property var pages: entityObj.ui.pages

    function parsePageItems(page, index, container) {
        console.debug("Page item: " + container)

        const items = page.items
        const gridWidth = page.grid ? page.grid.width : 4;
        const gridHeight = page.grid ? page.grid.height : 6;
        const gridSizeW = uiPages.width / gridWidth
        const gridSizeH = uiPages.height / gridHeight

        items.forEach(function(item) {
            switch (item.type) {
            case "text":
            case "icon":
                let component = Qt.createComponent("qrc:/components/ButtonIcon.qml");
                let obj = component.createObject(container, {
                                                     "x": gridSizeW * item.location.x,
                                                     "y": gridSizeH * item.location.y,
                                                     "width": gridSizeW * (item.size ? (item.size.width ? item.size.width : 1) : 1),
                                                     "height": gridSizeH * (item.size ? (item.size.height ? item.size.height : 1) : 1),
                                                     "icon": item.icon ? item.icon : "",
                                                     "text": item.text ? item.text : "",
                                                     "trigger": function() {
                                                         console.debug(JSON.stringify(item));

                                                         const cmdString = String(item.command.cmd_id);
                                                         const isSimple = !cmdString.includes("remote.");

                                                         if (isSimple) {
                                                             EntityController.onEntityCommand(
                                                                         entityObj.id,
                                                                         "remote.send",
                                                                         {"command": item.command.cmd_id})
                                                         } else {
                                                             EntityController.onEntityCommand(
                                                                         entityObj.id,
                                                                         item.command.cmd_id,
                                                                         item.command.params ? item.command.params : {});
                                                         }
                                                     }
                                                 });

                break;
            default:
                console.log("Not implemented item type: " + item.type);
                break;
            }
        });
    }

    function updateButtonMapping() {
        let overrideConfig = overrideConfigDefault;

        entityObj.buttonMapping.forEach((buttonMap) => {
                                            if (!overrideConfig[buttonMap.button]) {
                                                overrideConfig[buttonMap.button] =  ({});
                                            }

                                            if (buttonMap.short_press) {
                                                const cmdString = String(buttonMap.short_press.cmd_id);
                                                const canRepeat = !cmdString.includes("remote.");

                                                overrideConfig[buttonMap.button]["pressed"] = function() {
                                                    EntityController.onEntityCommand(
                                                                entityObj.id,
                                                                buttonMap.short_press.cmd_id,
                                                                buttonMap.short_press.params ? buttonMap.short_press.params : {});
                                                }

                                                overrideConfig[buttonMap.button]["pressed_repeat"] = function() {
                                                    if (canRepeat) {
                                                        EntityController.onEntityCommand(
                                                                    entityObj.id,
                                                                    "remote.send",
                                                                    {
                                                                        "command": buttonMap.short_press.cmd_id,
                                                                        "repeat": ui.inputController.repeatCount,
                                                                        "press": true
                                                                    });
                                                    } else {
                                                        EntityController.onEntityCommand(
                                                                    entityObj.id,
                                                                    buttonMap.short_press.cmd_id,
                                                                    buttonMap.short_press.params ? buttonMap.short_press.params : {});
                                                    }
                                                }

                                                // on release of a button we send the stop send command
                                                overrideConfig[buttonMap.button]["released"] = function() {
                                                    if (canRepeat) {
                                                        EntityController.onEntityCommand(
                                                                    entityObj.id,
                                                                    "remote.stop_send",
                                                                    {});
                                                    }
                                                }
                                            }

                                            if (buttonMap.long_press) {
                                                overrideConfig[buttonMap.button]["long_press"] = function() {
                                                    EntityController.onEntityCommand(
                                                                entityObj.id,
                                                                buttonMap.long_press.cmd_id,
                                                                buttonMap.long_press.params ? buttonMap.long_press.params : {});
                                                }
                                            }
                                        });

        buttonNavigation.overrideConfig = overrideConfig;
    }

    Component.onCompleted: updateButtonMapping()

    Connections {
        target: entityObj
        ignoreUnknownSignals: true

        function onButtonMappingChanged() {
            updateButtonMapping();
        }

        function onUiConfigChanged() {
            remoteBase.pages = entityObj.ui.pages;
        }
    }

    property var overrideConfigDefault: {
        "DPAD_LEFT": {
            "pressed": function() {
                uiPages.decrementCurrentIndex();
            }
        },
        "DPAD_RIGHT": {
            "pressed": function() {
                uiPages.incrementCurrentIndex();
            }
        }
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    PathView {
        id: uiPages
        width: parent.width
        height: parent.height - title.height
        anchors { horizontalCenter: parent.horizontalCenter; top: title.bottom }
        interactive: uiPages.count > 1

        snapMode: PathView.SnapToItem
        highlightRangeMode: PathView.StrictlyEnforceRange
        highlightMoveDuration: 200

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        model: pages
        cacheItemCount: 5
        delegate: uiPage

        path: Path {
            startX: -uiPages.width / 2 * (uiPages.count - 1)
            startY: uiPages.height / 2

            PathLine { x: -(uiPages.width / 2 * (uiPages.count - 1)) + (uiPages.width * uiPages.count); y: uiPages.height / 2 }
        }

        preferredHighlightEnd: 0.5
        preferredHighlightBegin: 0.5
    }

    PageIndicator {
        currentIndex: uiPages.currentIndex
        count: uiPages.count
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        visible: uiPages.count > 1
        padding: 0

        delegate: Component {
            Rectangle {
                width: 12; height: 12
                radius: 6
                color: colors.offwhite
                opacity: index == uiPages.currentIndex ? 1 : 0.6
            }
        }
    }

    Component {
        id: uiPage

        Item {
            id: gridContainer
            width: uiPages.width
            height: uiPages.height

            Component.onCompleted: {
                parsePageItems(pages[index], index, gridContainer)
            }

            Text {
                id: noComponentstitle
                text: qsTr("Empty page")
                color: colors.offwhite
                font: fonts.secondaryFont(26)
                width: parent.width - 40
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -100
                visible: pages[index].items.length === 0
            }

            Text {
                text: qsTr("You can add UI elements via the Web Configurator")
                color: colors.light
                font: fonts.secondaryFont(26)
                width: parent.width - 40
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                anchors { top: noComponentstitle.bottom; topMargin: 10; horizontalCenter: parent.horizontalCenter }
                visible: pages[index].items.length === 0
            }
        }
    }
}
