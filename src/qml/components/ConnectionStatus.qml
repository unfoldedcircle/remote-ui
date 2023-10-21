// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import Integration.Controller 1.0

import "qrc:/components" as Components

Popup {
    id: connectionStateRoot
    width: parent.width; height: parent.height
    opacity: 0
    modal: false
    closePolicy: Popup.CloseOnPressOutside
    padding: 0

    property int listMaxHeight: ui.height - iconClose.height - 100

    onOpened: {
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    connectionStateRoot.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    connectionStateRoot.close();
                }
            }
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            NumberAnimation { property: "scale"; from: 0.5; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            NumberAnimation { property: "y"; from: -300; to: 0; easing.type: Easing.OutExpo; duration: 400 }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 300 }
            NumberAnimation { property: "y"; from: 0; to: 200; easing.type: Easing.InExpo; duration: 300 }
        }
    }

    background: MouseArea {
        onClicked: connectionStateRoot.close()

        Rectangle {
            anchors.fill: parent
            color: colors.black
            opacity: 0.5
        }
    }

    contentItem: Item {
        Rectangle {
            width: parent.width - 40
            height: iconClose.height + itemList.height + 20 + (itemList.count === 0 ? 60 : 0)
            color: colors.medium
            radius: ui.cornerRadiusLarge
            anchors { top: parent.top; topMargin: 40; horizontalCenter: parent.horizontalCenter }

            Text {
                id: title

                //: Headline for showing integration connection statuses
                text: qsTr("Connection status")
                verticalAlignment: Text.AlignVCenter
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.offwhite
                font: fonts.primaryFont(30)
                anchors { left: parent.left; leftMargin: 20; right: iconClose.left; verticalCenter: iconClose.verticalCenter }
            }

            Components.Icon {
                id: iconClose
                color: colors.offwhite
                icon: "uc:close"
                anchors { right: parent.right; top: parent.top }
                size: 70

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        connectionStateRoot.close();
                    }
                }
            }

            ListView {
                id: itemList
                width: parent.width - 40
                height: {
                    if (count * 80 > connectionStateRoot.listMaxHeight) {
                        return connectionStateRoot.listMaxHeight;
                    } else {
                        return count * 80;
                    }
                }

                anchors { horizontalCenter: parent.horizontalCenter; top: iconClose.bottom }
                clip: true

                maximumFlickVelocity: 6000
                flickDeceleration: 1000
                highlightMoveDuration: 200

                model: IntegrationController.driversError

                delegate: listItem
            }

            Text {
                text: qsTr("No connection errors")
                width: itemList.width
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.light
                font: fonts.secondaryFont(20)
                anchors { top: itemList.top; topMargin: 20; horizontalCenter: itemList.horizontalCenter }
                visible: itemList.count === 0
            }
        }
    }

    Component {
        id: listItem

        Item {
            width: itemList.width; height: 80

            property QtObject integrationObj: IntegrationController.getDriversModelItem(modelData)
            property bool isLast: index + 1 < itemList.count ? false : true

            Components.Icon {
                id: icon
                color: colors.offwhite
                icon: integrationObj.icon === "" ? "uc:integration" : integrationObj.icon
                anchors { left: parent.left; verticalCenter: parent.verticalCenter; }
                size: 40
            }

            Text {
                id: integrationTitle
                text: integrationObj.name
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 2
                color: colors.offwhite
                anchors { left: icon.right; leftMargin: 10; right: integrationStateText.left; rightMargin: 10; verticalCenter: parent.verticalCenter; }
                font: fonts.primaryFont(20)
            }

            Text {
                id: integrationStateText
                text: integrationObj.state
                maximumLineCount: 1
                color: colors.light
                anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter; }
                font: fonts.secondaryFont(20)
            }

            Rectangle {
                width: parent.width; height: 2
                color: colors.dark
                anchors { bottom: parent.bottom }
                visible: !isLast
            }
        }
    }
}
