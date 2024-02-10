// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.1
import QtGraphicalEffects 1.0

import Entity.Controller 1.0
import Group.Controller 1.0
import Haptic 1.0

import "qrc:/components" as Components

ListView {
    id: page
    maximumFlickVelocity: 6000
    flickDeceleration: 1000
    highlightMoveDuration: 200
    cacheBuffer: 260 * 30
    pressDelay: 200
    keyNavigationEnabled: false

    model: visualModel
    header: header
    currentIndex: 0
    
    // Disable scrolling when no items on page, otherwise header is movable
    interactive: visualModel.count

    property string title: pageName
    property string _id: pageId
    property QtObject items: pageItems
    property bool isCurrentItem: ListView.isCurrentItem
    property int headerHeight: 260


    Behavior on height {
        NumberAnimation { easing.type: Easing.OutExpo; duration: 200 }
    }

    Behavior on contentY {
        NumberAnimation { easing.type: Easing.OutExpo; duration: 200 }
    }

    Connections {
        target: ui
        ignoreUnknownSignals: true

        function onEditModeChanged() {
            if (!page.isCurrentItem) {
                return;
            }

            if (!ui.editMode) {
                ui.updatePageItems(pageId)
                containerMain.item.pages.interactive = true;
            } else {
                containerMain.item.pages.interactive = false;
            }
        }
    }

    onIsCurrentItemChanged: {
        if (!isCurrentItem) {
            ui.setTimeOut(100, () =>{ page.currentIndex = 0; });
            if (ui.editMode) {
                ui.editMode = false;
            }
        }
    }
    onContentYChanged: {
            // Adjust the height of the header image based on overscroll
            if (contentY < -260){
                headerHeight= Math.max(-contentY, 260);
                // Return the header to its normal size when we release 
                if (!dragging){
                    contentY = -260;
                }
            }

        }

    DelegateModel {
        id: visualModel
        model: pageItems
        delegate: listItem
    }

    // header
    Component {
        id: header

        Item {
            width: ListView.view.width; height: headerImage.height

            Image {
                id: headerImage
                width: parent.width; height: headerHeight
                source: resource.getBackgroundImage(pageImage)
                sourceSize.width: parent.width
                sourceSize.height: 260
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
                cache: true
                visible: pageImage != ""

                Rectangle {
                    visible: headerImage.visible
                    anchors.fill: parent
                    color: colors.black; opacity: 0.7
                }

                LinearGradient {
                    visible: headerImage.visible
                    anchors.fill: parent
                    start: Qt.point(0, headerImage.height * 0.6)
                    end: Qt.point(0, headerImage.height)
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: colors.transparent }
                        GradientStop { position: 1.0; color: colors.black }
                    }
                }

                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource:
                        Rectangle {
                        id: opacityMask
                        width: headerImage.width; height: headerImage.height
                        radius: statusBar.height / 2
                    }
                }

            }

            Components.HapticMouseArea {
                anchors.fill: parent

                onClicked: {
                    if (!ui.editMode) {
                        loadSecondContainer("qrc:/components/PageSelector.qml", { currentPage: pageName });
                    } else {
                        ui.editMode = false;
                    }
                }
            }

            ColumnLayout {
                width: parent.width
                anchors.centerIn: headerImage
                spacing: 0

                Text {
                    id: titleText

                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10

                    color: colors.offwhite
                    text: title
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                    font: fonts.primaryFont(60, "Light")
                    lineHeight: 0.8
                }

                ListView {
                    id: activityList
                    Layout.fillWidth: true
                    Layout.preferredHeight: activityList.count * 30
                    Layout.topMargin: 10
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10

                    visible: activityList.count > 0 && !ui.editMode
                    model: pageActivities

                    delegate: Components.HapticMouseArea {
                        width: ListView.view.width
                        height: 30

                        property QtObject entity: EntityController.get(pageItemId)

                        onClicked: {
                            loadSecondContainer("qrc:/components/entities/" + entity.getTypeAsString() + "/deviceclass/" + entity.getDeviceClass() + ".qml", { "entityId": entity.id, "entityObj": entity });
                        }

                        Text {
                            anchors.centerIn: parent
                            color: Qt.lighter(colors.light)
                            text: {
                                //: Used to show the entity state: %1 is the entity name, %2 is the state
                                return qsTr("%1 is %2").arg(entity.name).arg(entity.stateAsString.toLowerCase());
                            }

                            elide: Text.ElideRight
                            maximumLineCount: 1
                            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                            font: fonts.secondaryFont(22)
                        }
                    }
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: visualModel.count === 0 && !ui.editMode

        Text {
            color: colors.offwhite
            //: Web configurator is the name of the application, does not need translation
            text: qsTr("Press and hold the Home button or use the Web Configurator to configure the page")
            width: parent.width - 40
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.secondaryFont(22)
        }
    }

    Component {
        id: listItem

        MouseArea {
            id: dragArea
            width: delegate.item ? delegate.item.width : delegate.width
            height: delegate.item ? delegate.item.height : delegate.height
            enabled: ui.editMode
            pressAndHoldInterval: 100
            anchors.horizontalCenter: parent.horizontalCenter

            property alias dragArea: dragArea
            property alias delegate: delegate
            property alias delegateItem: delegate.item

            property bool isCurrentItem: ListView.isCurrentItem
            property bool held: false
            property int toVal: 0

            property string itemId: pageItemId

            drag.target: held ? delegate : undefined
            drag.axis: Drag.YAxis

            onPressAndHold:  {
                page.interactive = false;

                if (!held) {
                    Haptic.play(Haptic.Click);
                    held = true;

                    if (delegate.item.groupObj) {
                        delegate.item.close();
                    }
                }
            }

            onReleased: {
                page.interactive = true;
                if (held) {
                    Haptic.play(Haptic.Click);
                    held = false;
                }
            }

            property int scrollEdgeSize: 200
            property int scrollingDirection: 0

            SmoothedAnimation {
                id: upAnimation
                target: page
                property: "contentY"
                to: -100
                running: scrollingDirection == -1
                duration: 500
            }

            SmoothedAnimation {
                id: downAnimation
                target: page
                property: "contentY"
                to: page.contentHeight - 130 - page.height
                running: scrollingDirection == 1
                duration: 500
            }

            states: [
                State {
                    when: dragArea.drag.active
                    name: "dragging"

                    PropertyChanges {
                        target: dragArea
                        scrollingDirection: {
                            var yCoord = page.mapFromItem(dragArea, dragArea.mouseY, 0).y;

                            if (yCoord < scrollEdgeSize) {
                                -1;
                            } else if (yCoord > page.height - 130 - scrollEdgeSize) {
                                1;
                            } else {
                                0;
                            }
                        }
                    }
                }
            ]

            Loader {
                id: delegate
                asynchronous: true
                enabled: !ui.editMode

                property bool isCurrentItem: parent.isCurrentItem
                property bool editMode: ui.editMode

                Drag.active: dragArea.held
                Drag.source: dragArea
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2

                states: State {
                    when: dragArea.held
                    ParentChange { target: delegate; parent: page }
                }

                onEditModeChanged: {
                    if (!editMode) {
                        delegate.x = 0;
                    }
                }

                Component.onCompleted: {
                    if (pageItemType === 0) {
                        this.setSource("qrc:/components/entities/Base.qml", { "entityId": pageItemId });
                    } else {
                        this.setSource("qrc:/components/group/Base.qml", { "groupId": pageItemId });
                    }
                }

                Behavior on x {
                    NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                }

                Components.Icon {
                    id: moveIcon
                    z: delegate.item ? delegate.item.z + 100 : 100
                    color: colors.light
                    opacity: delegate.editMode ? 1 : 0
                    icon: "uc:hamburger"
                    anchors { right: delegate.right; rightMargin: 20; top: delegate.top; topMargin: 35 }
                    size: 80

                    Behavior on opacity {
                        OpacityAnimator { duration: 300 }
                    }
                }
            }

            DropArea {
                anchors.fill: parent

                onEntered: {
                    dragArea.toVal = dragArea.DelegateModel.itemsIndex;
                    pageItems.swapData(drag.source.DelegateModel.itemsIndex, dragArea.toVal);

                }
            }
        }
    }
}

