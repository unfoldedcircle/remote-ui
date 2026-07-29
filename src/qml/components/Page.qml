// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.1
import QtGraphicalEffects 1.0

import Entity.Controller 1.0
import Group.Controller 1.0
import Entity.Activity 1.0
import Entity.MediaPlayer 1.0
import Haptic 1.0

import HwInfo 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/components/entities/media_player" as MediaPlayerComponents
import "qrc:/components/entities/activity" as ActivityComponents

ListView {
    id: page
    maximumFlickVelocity: 6000
    flickDeceleration: 1000
    highlightMoveDuration: 200
    cacheBuffer: 260 * 4
    pressDelay: 200
    keyNavigationEnabled: false

    model: visualModel
    header: header
    currentIndex: 0

    signal draggedDownYChanged(int contentY, int treshold)

    property string title: pageName
    property string _id: pageId
    property QtObject items: pageItems
    property bool _isCurrentItem: pages.currentItem._id === page._id

    property bool blockDraggedDownSignal: false
    // Disables the contentY animation so the header resize logic can follow the
    // top of the view instantly, frame by frame, instead of lagging behind.
    property bool suppressContentYAnimation: false

    Behavior on height {
        NumberAnimation { easing.type: Easing.OutExpo; duration: 500 }
    }

    Behavior on contentY {
        enabled: !page.suppressContentYAnimation
        NumberAnimation { easing.type: Easing.OutExpo; duration: 500 }
    }

    Connections {
        target: ui
        ignoreUnknownSignals: true

        function onEditModeChanged() {
            if (!page._isCurrentItem) {
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

    Connections {
        target: Config
        ignoreUnknownSignals: true

        function onEnableActivityBarChanged() {
            page.headerItem.reCalculateHeaderHeight();
        }
    }

    onVerticalVelocityChanged: {
        if (Math.abs(verticalVelocity) > 2500 && !blockDraggedDownSignal) {
            blockDraggedDownSignal = true;
        }
    }

    onFlickingChanged: {
        if (!flicking && blockDraggedDownSignal) {
            blockDraggedDownSignal = false;
        }
    }

    onContentYChanged: {
        if (!blockDraggedDownSignal) {
            draggedDownYChanged(contentY, headerItem.height + 100);
        }
    }

    on_IsCurrentItemChanged: {
        if (!page._isCurrentItem) {
            ui.setTimeOut(100, () =>{ page.currentIndex = 0; });
            if (ui.editMode) {
                ui.editMode = false;
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
            id: headerContainer
            width: ListView.view.width
            height: 260
            property int targetHeight: 260
            // While the header animates its height (e.g. album art appearing or
            // disappearing when a media player starts/stops) the scroll position
            // is preserved: a page that sits at the top keeps following the top,
            // a page scrolled into its content is left alone because the view
            // already keeps the items in place when the header resizes.
            property bool pinToTop: false
            //            clip: true

            function reCalculateHeaderHeight() {
                const hasMediaComponent = activityList.currentItem && activityList.currentItem.hasMediaComponent;
                const nextHeight = activityList.count > 0 && Config.enableActivityBar
                        ? 680 - (hasMediaComponent ? 0 : 240)
                        : 260;

                if (headerContainer.height === nextHeight) {
                    return;
                }

                // The header lives above the first item, so the top of the view
                // is contentY === originY (a negative value the size of the
                // header), not 0. Only pin when the page really is at the top,
                // and never while a finger is on the screen: the pull down
                // gesture must not be fought by the header animation.
                headerContainer.pinToTop = !page.dragging && page.contentY <= page.originY + 1;
                // Pinning adjusts contentY every frame to track the growing or
                // shrinking header, so bypass the contentY animation meanwhile.
                page.suppressContentYAnimation = headerContainer.pinToTop;

                headerContainer.targetHeight = nextHeight;
                headerContainer.height = nextHeight;
            }

            Component.onCompleted: {
                reCalculateHeaderHeight();
            }

            onHeightChanged: {
                if (!headerContainer.pinToTop) {
                    return;
                }

                // originY already reflects the new header size here, so the page
                // stays glued to the top of the header while it animates.
                page.contentY = page.originY;

                if (headerContainer.height === headerContainer.targetHeight) {
                    headerContainer.pinToTop = false;
                    page.suppressContentYAnimation = false;
                }
            }

            Behavior on height {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 500 }
            }

            Image {
                id: headerImage
                width: parent.width; height: parent.height
                source: resource.getBackgroundImage(pageImage)
                sourceSize.width: parent.width
                sourceSize.height: parent.height
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
                cache: true
                visible: pageImage != ""
                anchors.top: parent.top

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

                layer.enabled: headerImage.visible
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
                height: parent.height
                anchors.top: parent.top
                spacing: 0

                Text {
                    id: titleText

                    Layout.fillWidth: true
                    Layout.fillHeight: activityList.count == 0
                    Layout.topMargin: activityList.count > 0 && Config.enableActivityBar ? 60 : 0
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
                    Layout.fillHeight: true
                    Layout.topMargin: 10
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10

                    orientation: ListView.Horizontal
                    snapMode: ListView.SnapOneItem
                    highlightRangeMode: ListView.StrictlyEnforceRange
                    maximumFlickVelocity: 6000
                    flickDeceleration: 1000
                    highlightMoveDuration: 200
                    pressDelay: 100
                    keyNavigationEnabled: false
                    clip: true

                    visible: activityList.count > 0 && !ui.editMode && Config.enableActivityBar
                    model: pageActivities

                    onCountChanged: {
                        headerContainer.reCalculateHeaderHeight();
                    }

                    onCurrentItemChanged: {
                        headerContainer.reCalculateHeaderHeight();
                    }

                    delegate: Components.HapticMouseArea {
                        id: activityListItem
                        width: ListView.view.width
                        height: ListView.view.height

                        property QtObject entity: QtObject
                        property QtObject mediaComponentEntity: QtObject
                        property bool _isCurrentItem: ListView.isCurrentItem
                        property bool _touchSliderActive: !isSecondContainerLoaded && page._isCurrentItem && activityListItem._isCurrentItem

                        on_TouchSliderActiveChanged: {
                            touchSlider.active = _touchSliderActive;
                        }

                        property alias entityIcon: entityIcon
                        property bool hasMediaComponent: false
                        property int mediaComponentHeight: hasMediaComponent ? 420 : 180

                        onHasMediaComponentChanged: {
                            Qt.callLater(function() {
                                activityList.forceLayout();
                                if (activityListItem._isCurrentItem) {
                                    headerContainer.reCalculateHeaderHeight();
                                }
                            });
                        }

                        function syncMediaComponentState() {
                            hasMediaComponent = mediaComponentEntity
                                                && mediaComponentEntity.type === EntityTypes.Media_player
                                                && mediaComponentEntity.mediaImage !== "";
                        }

                        function findButtonMap(entity, buttonName) {
                            const buttonMap = entity.buttonMapping.find(buttonMap => buttonMap.button === buttonName);
                            return buttonMap ? buttonMap["short_press"] : undefined;
                        }

                        function triggerCommand(entityId, cmdId, params) {
                            let e = EntityController.get(entityId);

                            if (e.type === EntityTypes.Macro) {
                                activityLoading.start(entityId, EntityTypes.Macro);
                            }

                            EntityController.onEntityCommand(
                                        entityId,
                                        cmdId,
                                        (params ? params : {}))
                        }

                        onMediaComponentEntityChanged: {
                            touchSlider.entityObj = mediaComponentEntity;
                            syncMediaComponentState();
                        }

                        Connections {
                            target: mediaComponentEntity
                            ignoreUnknownSignals: true

                            function onMediaImageChanged() {
                                activityListItem.syncMediaComponentState();
                            }
                        }

                        Component.onCompleted: {
                            entity = EntityController.get(pageItemId);
                            activityMediaComponent.entityId = pageItemId;

                            if (entity.type === EntityTypes.Activity) {
                                if (!entity.sliderConfig.enabled) {
                                    touchSlider.active = false;
                                }

                                if (entity.sliderConfig.entityId !== "default") {
                                    mediaComponentEntity = EntityController.get(entity.sliderConfig.entityId);
                                    activityMediaComponent.entityId = entity.sliderConfig.entityId;
                                    if (entity.sliderConfig.enabled) {
                                        // "default" is resolved by TouchSlider based on the target entity's type
                                        touchSlider.feature = entity.sliderConfig.entityFeature;
                                    }
                                    syncMediaComponentState();
                                    return;
                                }

                                // default is the first media_player widget
                                const activityPages = entity.ui.pages;

                                for (const activityPage of activityPages) {
                                    const pageItems = activityPage.items;

                                    for (const item of pageItems) {
                                        if (item.type === "media_player") {

                                            const itemMediaPlayerId = item.media_player_id;

                                            mediaComponentEntity = EntityController.get(itemMediaPlayerId);
                                            activityMediaComponent.entityId = itemMediaPlayerId;
                                            syncMediaComponentState();
                                            break;
                                        }
                                    }
                                }
                            } else if (entity.type === EntityTypes.Media_player) {
                                mediaComponentEntity = entity;
                                syncMediaComponentState();
                            }
                        }

                        onClicked: {
                            loadSecondContainer("qrc:/components/entities/" + entity.getTypeAsString() + "/deviceclass/" + entity.getDeviceClass() + ".qml", { "entityId": entity.id, "entityObj": entity });
                        }

                        Components.TouchSlider {
                            id: touchSlider
                            entityObj: mediaComponentEntity
                            active: activityListItem._touchSliderActive //!isSecondContainerLoaded && page._isCurrentItem && activityListItem._isCurrentItem
                            parent: Overlay.overlay
                        }

                        // TODO: map buttons to activity button mapping if exists

                        Components.ButtonNavigation {
                            overrideActive: page._isCurrentItem && activityListItem._isCurrentItem && ui.inputController.activeItem == mainContainerRoot
                            defaultConfig: {
                                "VOLUME_UP": {
                                    "pressed": function() {
                                        if (mainContainerBlockingMouseArea.enabled) {
                                            mainContainerRoot.closeMenu();
                                            return;
                                        }

                                        if (entity.type === EntityTypes.Activity) {
                                            const mapping = findButtonMap(entity, "VOLUME_UP");
                                            if (mapping) {
                                                triggerCommand(mapping.entity_id, mapping.cmd_id, mapping.params);
                                            }
                                        } else if (entity.type === EntityTypes.Media_player) {
                                            mediaComponentEntity.volumeUp();
                                            volume.start(mediaComponentEntity);
                                        }
                                    }
                                },
                                "VOLUME_DOWN": {
                                    "pressed": function() {
                                        if (mainContainerBlockingMouseArea.enabled) {
                                            mainContainerRoot.closeMenu();
                                            return;
                                        }

                                        if (entity.type === EntityTypes.Activity) {
                                            const mapping = findButtonMap(entity, "VOLUME_DOWN");
                                            if (mapping) {
                                                triggerCommand(mapping.entity_id, mapping.cmd_id, mapping.params);
                                            }
                                        } else if (entity.type === EntityTypes.Media_player) {
                                            mediaComponentEntity.volumeDown();
                                            volume.start(mediaComponentEntity, false);
                                        }
                                    }
                                },
                                "MUTE": {
                                    "pressed": function() {
                                        if (mainContainerBlockingMouseArea.enabled) {
                                            mainContainerRoot.closeMenu();
                                            return;
                                        }

                                        if (entity.type === EntityTypes.Activity) {
                                            const mapping = findButtonMap(entity, "MUTE");
                                            if (mapping) {
                                                triggerCommand(mapping.entity_id, mapping.cmd_id, mapping.params);
                                            }
                                        } else if (entity.type === EntityTypes.Media_player) {
                                            mediaComponentEntity.muteToggle();
                                        }
                                    }
                                },
                                "PLAY": {
                                    "pressed": function() {
                                        if (mainContainerBlockingMouseArea.enabled) {
                                            mainContainerRoot.closeMenu();
                                            return;
                                        }

                                        if (entity.type === EntityTypes.Activity) {
                                            const mapping = findButtonMap(entity, "PLAY");
                                            if (mapping) {
                                                triggerCommand(mapping.entity_id, mapping.cmd_id, mapping.params);
                                            }
                                        } else if (entity.type === EntityTypes.Media_player) {
                                            mediaComponentEntity.playPause();
                                        }
                                    }
                                },
                                "PREV": {
                                    "pressed": function() {
                                        if (mainContainerBlockingMouseArea.enabled) {
                                            mainContainerRoot.closeMenu();
                                            return;
                                        }

                                        if (entity.type === EntityTypes.Activity) {
                                            const mapping = findButtonMap(entity, "PREV");
                                            if (mapping) {
                                                triggerCommand(mapping.entity_id, mapping.cmd_id, mapping.params);
                                            }
                                        } else if (entity.type === EntityTypes.Media_player) {
                                            mediaComponentEntity.previous();
                                        }
                                    }
                                },
                                "NEXT": {
                                    "pressed": function() {
                                        if (mainContainerBlockingMouseArea.enabled) {
                                            mainContainerRoot.closeMenu();
                                            return;
                                        }

                                        if (entity.type === EntityTypes.Activity) {
                                            const mapping = findButtonMap(entity, "NEXT");
                                            if (mapping) {
                                                triggerCommand(mapping.entity_id, mapping.cmd_id, mapping.params);
                                            }
                                        } else if (entity.type === EntityTypes.Media_player) {
                                            mediaComponentEntity.next();
                                        }
                                    }
                                },
                                "POWER": {
                                    "pressed": function() {
                                        if (mainContainerBlockingMouseArea.enabled) {
                                            mainContainerRoot.closeMenu();
                                            return;
                                        }

                                        if (EntityController.activities.length === 0) {
                                            return;
                                        }

                                        popupMenu.title = qsTr("Turn off");
                                        let menuItems = [];

                                        for (let i = 0; i<EntityController.activities.length; i++) {
                                            const e = EntityController.activities[i];

                                            const entityObj = EntityController.get(e);

                                            if (entityObj.hasFeature(MediaPlayerFeatures.On_off) || entityObj.type == EntityTypes.Activity) {
                                                menuItems.push({
                                                                   title: entityObj.name,
                                                                   icon: entityObj.icon,
                                                                   callback: function() {
                                                                       function retry() {
                                                                           const res = checkActivityIncludedEntities(entityObj, false);

                                                                           if (!EntityController.resumeWindow) {
                                                                               if (!res.allIncludedEntitiesConnected && entityObj.readyCheck) {
                                                                                   ui.createActionableNotification(qsTr("Some devices are not ready"), (res.notReadyEntityQty == 1 ? qsTr("%1 is not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities) : qsTr("%1 are not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities)), "uc:link-slash", () => { entityObj.turnOff(); }, qsTr("Proceed"));
                                                                                   return;
                                                                               }

                                                                               entityObj.turnOff();
                                                                               return;
                                                                           }

                                                                           if (!res.allIncludedEntitiesConnected) {
                                                                               ui.setTimeOut(500, retry);
                                                                           } else {
                                                                               entityObj.turnOff();
                                                                           }
                                                                       }

                                                                       if (entityObj.type == EntityTypes.Activity) {
                                                                           retry();
                                                                       } else {
                                                                           entityObj.turnOff();
                                                                       }
                                                                   }
                                                               });
                                            }
                                        }

                                        if (EntityController.activities.length > 1) {
                                            menuItems.push({
                                                               title: qsTr("Turn off all"),
                                                               icon: "uc:power-off",
                                                               callback: function() {
                                                                   for (let i = 0; i<EntityController.activities.length; i++) {
                                                                       const eObj = EntityController.get(EntityController.activities[i]);

                                                                       function retry() {
                                                                           const res = checkActivityIncludedEntities(eObj, false);

                                                                           if (!EntityController.resumeWindow) {
                                                                               if (!res.allIncludedEntitiesConnected && eObj.readyCheck) {
                                                                                   ui.createActionableNotification(eObj.name, (res.notReadyEntityQty == 1 ? qsTr("%1 is not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities) : qsTr("%1 are not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities)), "uc:link-slash", () => { eObj.turnOff(); }, qsTr("Proceed"));
                                                                                   return;
                                                                               }

                                                                               eObj.turnOff();
                                                                               return;
                                                                           }

                                                                           if (!res.allIncludedEntitiesConnected) {
                                                                               ui.setTimeOut(500, retry);
                                                                           } else {
                                                                               eObj.turnOff();
                                                                           }
                                                                       }

                                                                       if (eObj.type == EntityTypes.Activity) {
                                                                           retry();
                                                                       } else {
                                                                           eObj.turnOff();
                                                                       }
                                                                   }
                                                               }
                                                           });
                                        }

                                        popupMenu.menuItems = menuItems;
                                        popupMenu.open();
                                    }
                                }
                            }
                        }

                        Column {
                            width: parent.width
                            anchors.top: parent.top
                            spacing: 10

                            Text {
                                width: parent.width
                                color: Qt.lighter(colors.light)
                                text: {
                                    //: Used to show the entity state: %1 is the entity name, %2 is the state
                                    return qsTr("%1 is %2").arg(entity.name).arg(entity.stateAsString.toLowerCase());
                                }
                                elide: Text.ElideRight
                                maximumLineCount: 1
                                horizontalAlignment: Text.AlignHCenter
                                font: fonts.secondaryFont(22)
                            }

                            Item {
                                width: parent.width - 40
                                height: activityListItem.mediaComponentHeight
                                x: 20

                                Rectangle {
                                    id: entityIcon
                                    width: 180
                                    height: 180
                                    color: colors.dark
                                    radius: 8
                                    visible: !activityListItem.hasMediaComponent
                                    anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

                                    Components.Icon {
                                        anchors.fill: parent
                                        icon: entity.icon ? entity.icon : ""
                                        size: 90
                                        color: colors.offwhite
                                    }
                                }

                                ActivityComponents.MediaComponent {
                                    id: activityMediaComponent
                                    width: parent.width
                                    height: parent.height
                                    isComponentHorizontal: false
                                    visible: activityListItem.hasMediaComponent
                                    anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

                                    Component.onCompleted: {
                                        mediaImage.shrinkHeight = true;
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10
                    Layout.topMargin: 10
                    Layout.bottomMargin: 10
                    visible: activityList.count > 1 && Config.enableActivityBar

                    PageIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter

                        currentIndex: activityList.currentIndex
                        count: activityList.count
                        padding: 0

                        delegate: Component {
                            Rectangle {
                                width: 10; height: 10
                                radius: 5
                                color: colors.offwhite
                                opacity: index === activityList.currentIndex ? 1 : 0.6
                            }
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
            anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined

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
                    icon: "uc:bars"
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
