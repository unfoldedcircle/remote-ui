// Copyright (c) 2022-2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0
import Entity.Select 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: selectBase

    function selectOption(option) {
        entityObj.selectOption(option);

        if (selectBase.state == "open") {
            selectBase.close();
        }
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

    overrideConfig: {
        "DPAD_LEFT": {
            "pressed": function() {
                entityObj.selectPrevious();

                if (selectBase.state == "open") {
                    selectBase.close();
                }
            }
        },
        "DPAD_RIGHT": {
            "pressed": function() {
                entityObj.selectNext();

                if (selectBase.state == "open") {
                    selectBase.close();
                }
            }
        },
        "PREV": {
            "pressed": function() {
                entityObj.selectFirst();

                if (selectBase.state == "open") {
                    selectBase.close();
                }
            }
        },
        "NEXT": {
            "pressed": function() {
                entityObj.selectLast();

                if (selectBase.state == "open") {
                    selectBase.close();
                }
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
                selectBase.selectOption(optionsList.currentItem.optionName);
            }
        },
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    ListView {
        id: optionsList
        clip: true
        pressDelay: 100
        keyNavigationEnabled: true
        focus: true
        model: entityObj.options
        anchors { left: parent.left; leftMargin: 10; right: parent.right; rightMargin: 10; top: title.bottom; topMargin: 20; bottom: parent.bottom; bottomMargin: 20 }

        delegate: Components.HapticMouseArea {
            width: parent.width
            height: 120

            property string optionName: modelData

            onClicked: {
                selectBase.selectOption(modelData);
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

    Connections {
        target: entityObj
        ignoreUnknownSignals: true

        function onCurrentOptionChanged() {
            selectBase.selectCurrent();
        }
    }

    Component.onCompleted: selectBase.selectCurrent()
}
