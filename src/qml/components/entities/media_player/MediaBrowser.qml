// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Entity.MediaPlayer 1.0
import Haptic 1.0

import QtGraphicalEffects 1.0

import Config 1.0

import "qrc:/components" as Components
import "qrc:/components/entities/media_player" as MediaPlayerComponents

Popup {
    id: mediaBrowser
    width: parent.width; height: parent.height
    y: 500
    opacity: 0
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0

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
            PropertyAnimation { properties: "y"; from: 0; to: 500; easing.type: Easing.InExpo; duration: 300 }
            PropertyAnimation { properties: "opacity"; from: 1.0; to: 0.0 }
        }
    }

    property QtObject entityObj

    readonly property int defaultPageLimit: 20

    // ----------- root-level state -----------
    property bool searchMode: false
    property bool coverFlowMode: false
    property var  searchResults: []
    property bool searchNoResults: false
    property bool searchLoading: false
    property var  selectedMediaClasses: []
    property string pendingPlayMediaId: ""
    property string pendingPlayIcon: "uc:play"
    readonly property var availableSearchMediaClasses: entityObj ? entityObj.searchMediaClasses : []

    readonly property bool isAtRoot: browseNav.depth <= 1
    readonly property var  currentPage: browseNav.currentItem
    readonly property bool currentIsContainer: currentPage && currentPage.isContainerView

    // drives the global loading-screen timer
    readonly property bool isLoading: (currentPage ? currentPage.pageLoading : false) || searchLoading

    // ----------- play menu helper -----------
    function buildPlayMenu(mediaId, mediaType, actions) {
        var all = !actions || actions.length === 0;
        var items = [];
        if (all || actions.indexOf("PLAY_NOW") >= 0)
            items.push({ title: qsTr("Play now"),     icon: "uc:play",
                         callback: (function(id, t) { return function() { mediaBrowser.requestPlayMedia(id, t); }; })(mediaId, mediaType) });
        if (all || actions.indexOf("PLAY_NEXT") >= 0)
            items.push({ title: qsTr("Play next"),    icon: "uc:forward-step",
                         callback: (function(id, t) { return function() { mediaBrowser.requestPlayMedia(id, t, "PLAY_NEXT"); }; })(mediaId, mediaType) });
        if (all || actions.indexOf("ADD_TO_QUEUE") >= 0)
            items.push({ title: qsTr("Add to queue"), icon: "uc:album-circle-plus",
                         callback: (function(id, t) { return function() { mediaBrowser.requestPlayMedia(id, t, "ADD_TO_QUEUE"); }; })(mediaId, mediaType) });
        return items;
    }

    // ----------- navigation functions -----------
    function loadRoot() {
        // pop everything back to root without animation (called from search exit only)
        while (browseNav.depth > 1)
            browseNav.pop(null, StackView.Immediate);

        var page = browseNav.currentItem;
        if (page) {
            page.pageLoading     = true;
            page.pageItems       = [];
            page.pageContainer   = null;
            page.pageThumbnail   = "";
            page.pagePage        = 1;
            page.requestedPage   = 1;
            page.pageLimit       = defaultPageLimit;
            page.pageHasMore     = false;
            page.pageLoadingMore = false;
        }

        entityObj.browseMedia("", "", defaultPageLimit, 1);
    }

    function browseInto(mediaId, mediaType, title, thumbnail) {
        searchMode = false;
        browseNav.push(levelPage, {
            pageTitle:       title,
            pageThumbnail:   (thumbnail && !thumbnail.startsWith("icon://")) ? thumbnail : "",
            pageMediaId:     mediaId,
            pageMediaType:   mediaType,
            pageLoading:     true,
            pageItems:       [],
            pageContainer:   null,
            pagePage:        1,
            requestedPage:   1,
            pageLimit:       defaultPageLimit,
            pageHasMore:     false,
            pageLoadingMore: false
        });
        entityObj.browseMedia(mediaId, mediaType, defaultPageLimit, 1);
    }

    function goBack() { browseNav.pop(); }

    function clearSearch() {
        searchDebounce.stop();
        inlineSearch.inputField.clear();
        inlineSearch.inputField.focus = false;
        searchResults        = [];
        searchNoResults      = false;
        selectedMediaClasses = [];
    }

    function clearPendingPlay() {
        pendingPlayMediaId = "";
        pendingPlayIcon = "uc:play";
        pendingPlayFeedback.stop();
        if (mediaBrowser.opened) {
            mediaBrowser.close();
        }
    }

    function doSearch(query) {
        if (!query.trim()) return;
        searchLoading = true;
        searchNoResults = false;
        entityObj.searchMedia(query.trim(), "", "", selectedMediaClasses, defaultPageLimit, 1);
    }

    function requestPlayMedia(mediaId, mediaType, action) {
        pendingPlayMediaId = mediaId || "";
        if (action === "PLAY_NEXT")
            pendingPlayIcon = "uc:forward-step";
        else if (action === "ADD_TO_QUEUE")
            pendingPlayIcon = "uc:album-circle-plus";
        else
            pendingPlayIcon = "uc:play";
        pendingPlayFeedback.restart();

        if (action)
            entityObj.playMedia(mediaId, mediaType, action);
        else
            entityObj.playMedia(mediaId, mediaType);
    }

    function displayLabelForMediaClass(mediaClass) {
        var labels = {
            "album": qsTr("Album"),
            "app": qsTr("App"),
            "apps": qsTr("Apps"),
            "artist": qsTr("Artist"),
            "channel": qsTr("Channel"),
            "channels": qsTr("Channels"),
            "composer": qsTr("Composer"),
            "directory": qsTr("Directory"),
            "episode": qsTr("Episode"),
            "game": qsTr("Game"),
            "genre": qsTr("Genre"),
            "image": qsTr("Image"),
            "movie": qsTr("Movie"),
            "music": qsTr("Music"),
            "playlist": qsTr("Playlist"),
            "podcast": qsTr("Podcast"),
            "radio": qsTr("Radio"),
            "season": qsTr("Season"),
            "track": qsTr("Track"),
            "tv_show": qsTr("TV Show"),
            "url": qsTr("URL"),
            "video": qsTr("Video")
        };
        return labels[mediaClass] || mediaClass.replace(/_/g, " ");
    }

    function syncSelectedMediaClasses() {
        var available = availableSearchMediaClasses || [];
        var filtered = selectedMediaClasses.filter(function(mediaClass) {
            return available.indexOf(mediaClass) >= 0;
        });

        if (filtered.length !== selectedMediaClasses.length) {
            selectedMediaClasses = filtered;
        }
    }

    function loadMore() {
        var page = browseNav.currentItem;
        if (!page || !page.pageHasMore || page.pageLoadingMore || page.pageLoading)
            return;

        page.pageLoadingMore = true;
        page.requestedPage = page.pagePage + 1;
        entityObj.browseMedia(page.pageMediaId, page.pageMediaType, page.pageLimit, page.requestedPage);
    }

    onOpened: {
        coverFlowMode = Config.mediaCoverflowDefault;
        browseNav.clear();
        browseNav.push(levelPage, {
            pageLoading: true,
            pagePage: 1,
            requestedPage: 1,
            pageLimit: defaultPageLimit,
            pageHasMore: false,
            pageLoadingMore: false
        }, StackView.Immediate);
        entityObj.browseMedia("", "", defaultPageLimit, 1);
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
        searchMode = false;
        clearSearch();
        clearPendingPlay();
    }

    onAvailableSearchMediaClassesChanged: syncSelectedMediaClasses()

    onIsLoadingChanged: {
        if (isLoading) {
            startLoadingScreen.start();
        } else {
            startLoadingScreen.stop();
            loading.stop();
        }
    }

    Timer {
        id: startLoadingScreen
        repeat: false; interval: 500; running: false
        onTriggered: loading.start()
    }

    Timer {
        id: searchDebounce
        repeat: false; interval: 800; running: false
        onTriggered: {
            var q = inlineSearch.inputField.text.trim();
            if (q) {
                inlineSearch.inputField.focus = false;
                mediaBrowser.doSearch(q);
            }
        }
    }

    Timer {
        id: pendingPlayFeedback
        repeat: false
        interval: 2000
        onTriggered: mediaBrowser.clearPendingPlay()
    }

    // ----------- button navigation -----------
    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_UP": {
                "pressed": function() {
                    var lv = currentPage ? currentPage.pageListView : null;
                    if (lv && lv.currentIndex > 0) lv.currentIndex--;
                },
                "pressed_repeat": function() {
                    var lv = currentPage ? currentPage.pageListView : null;
                    if (lv && lv.currentIndex > 0) lv.currentIndex--;
                }
            },
            "DPAD_DOWN": {
                "pressed": function() {
                    var lv    = currentPage ? currentPage.pageListView  : null;
                    var items = currentPage ? currentPage.displayItems  : [];
                    if (lv && lv.currentIndex < items.length - 1) lv.currentIndex++;
                },
                "pressed_repeat": function() {
                    var lv    = currentPage ? currentPage.pageListView  : null;
                    var items = currentPage ? currentPage.displayItems  : [];
                    if (lv && lv.currentIndex < items.length - 1) lv.currentIndex++;
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    var page = currentPage; if (!page) return;
                    var item = page.displayItems[page.pageListView.currentIndex];
                    if (!item) return;
                    if (item.can_browse) mediaBrowser.browseInto(item.media_id, item.media_type, item.title, item.thumbnail);
                    else if (item.can_play) mediaBrowser.requestPlayMedia(item.media_id, item.media_type);
                }
            },
            "PLAY": {
                "pressed": function() {
                    entityObj.playPause();
                }
            },
            "VOLUME_UP": {
                "pressed": function() {
                    entityObj.volumeUp();
                    volume.start(entityObj);
                }
            },
            "VOLUME_DOWN": {
                "pressed": function() {
                    entityObj.volumeDown();
                    volume.start(entityObj, false);
                }
            },
            "BACK": {
                "pressed": function() {
                    if (searchMode) {
                        mediaBrowser.searchMode = false;
                        mediaBrowser.clearSearch();
                        mediaBrowser.loadRoot();
                    } else if (browseNav.depth > 1) {
                        mediaBrowser.goBack();
                    } else {
                        mediaBrowser.close();
                    }
                }
            },
            "HOME": { "pressed": function() { mediaBrowser.close(); } }
        }
    }

    // ----------- signal connections -----------
    Connections {
        target: entityObj
        ignoreUnknownSignals: true

        function onBrowseMediaResult(media, pagination) {
            var page = browseNav.currentItem;
            if (!page) return;

            if (browseNav.depth > 1) {
                page.pageContainer = media;
                var t = media.thumbnail || "";
                if (t && !t.startsWith("icon://"))
                    page.pageThumbnail = t;
            } else {
                page.pageContainer = null;
            }

            var incoming = media.items || [];
            var append = page.pageLoadingMore && page.requestedPage > 1;
            var lv = page.pageListView;
            var cf = page.pageCoverFlowView;
            var prevCount = append ? page.pageItems.length : 0;
            // Save PathView's current sentinel-aware index before the model is replaced.
            var savedCFIndex = (append && cf) ? cf.currentIndex : -1;

            page.pageItems = append ? page.pageItems.concat(incoming) : incoming;

            // Always trust our own request tracking.
            page.pagePage = page.requestedPage;

            // Do NOT overwrite page.pageLimit from server response.
            page.pageHasMore = incoming.length > 0
                               && pagination && pagination.count > 0
                               && page.pageItems.length < pagination.count;

            page.pageLoadingMore = false;
            page.pageLoading = false;

            // After a load-more the model array is replaced, which resets scroll/index state.
            // Both restorations are merged into one deferred tick to avoid inter-frame ordering issues.
            if (append && lv) {
                Qt.callLater(function() {
                    if (prevCount > 0)    lv.positionViewAtIndex(prevCount - 1, ListView.End);
                    if (savedCFIndex > 0) lv.currentIndex = savedCFIndex - 1;
                });
            }
        }

        function onSearchMediaResult(items, pagination) {
            searchLoading    = false;
            searchResults    = items.slice();   // new reference forces binding re-evaluation
            searchNoResults  = (items.length === 0);
        }

        function onMediaBrowseError(code, message) {
            loading.stop();
            var page = browseNav.currentItem; if (!page) return;
            page.pageLoading     = false;
            page.pageLoadingMore = false;
            ui.createActionableWarningNotification(
                qsTr("Could not load media"),
                message || qsTr("An error occurred while loading media content."),
                "uc:warning",
                function() {
                    page.pageLoading = true;
                    page.pageLoadingMore = false;
                    page.pagePage = 1;
                    page.requestedPage = 1;
                    page.pageItems = [];
                    page.pageHasMore = false;
                    if (browseNav.depth <= 1) {
                        entityObj.browseMedia("", "", page.pageLimit, 1);
                    } else {
                        entityObj.browseMedia(page.pageMediaId, page.pageMediaType, page.pageLimit, 1);
                    }
                },
                qsTr("Retry")
            );
        }

        function onSearchMediaClassesChanged() {
            mediaBrowser.syncSelectedMediaClasses();
        }
    }

    background: Rectangle { color: colors.black; opacity: 0.8 }

    contentItem: Rectangle {
        color: colors.black
        radius: ui.cornerRadiusLarge

        // ----------- header -----------
        Item {
            id: browserHeader
            width: parent.width
            height: 75
            anchors { top: parent.top; topMargin: 5; left: parent.left; right: parent.right }

            Components.Icon {
                id: backBtn
                icon: !isAtRoot ? "uc:arrow-left" : "uc:xmark"
                size: 60
                color: colors.offwhite
                visible: !searchMode
                anchors { left: parent.left; leftMargin: 4; verticalCenter: parent.verticalCenter }
                Components.HapticMouseArea {
                    width: parent.width + 20; height: parent.height + 20
                    anchors.centerIn: parent
                    onClicked: { if (!isAtRoot) mediaBrowser.goBack(); else mediaBrowser.close(); }
                }
            }

            Text {
                id: headerTitle
                text: !isAtRoot && currentPage ? currentPage.pageTitle : qsTr("Browse")
                color: colors.offwhite
                font: fonts.primaryFont(30, "Normal")
                elide: Text.ElideRight
                visible: !searchMode
                anchors {
                    left: backBtn.right; leftMargin: 8
                    right: headerModeToggle.visible ? headerModeToggle.left
                           : (searchToggleBtn.visible ? searchToggleBtn.left : parent.right)
                    rightMargin: 8
                    verticalCenter: parent.verticalCenter
                }
            }

            Rectangle {
                id: headerModeToggle
                visible: !searchMode && isAtRoot && !currentIsContainer
                width: 160; height: 68; radius: 34
                color: colors.dark
                anchors {
                    right: searchToggleBtn.visible ? searchToggleBtn.left : parent.right
                    rightMargin: searchToggleBtn.visible ? 6 : 16
                    verticalCenter: parent.verticalCenter
                }

                // sliding active indicator
                Rectangle {
                    width: 76; height: 60; radius: 30
                    color: colors.medium
                    anchors.verticalCenter: parent.verticalCenter
                    x: coverFlowMode ? 80 : 4
                    Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                }

                // list icon (left slot)
                Components.Icon {
                    icon: "uc:list"; size: 60
                    color: !coverFlowMode ? colors.offwhite : colors.light
                    anchors { left: parent.left; leftMargin: 4 + 8; verticalCenter: parent.verticalCenter }
                }

                // coverflow icon (right slot)
                Components.Icon {
                    icon: "uc:album-collection"; size: 60
                    color: coverFlowMode ? colors.offwhite : colors.light
                    anchors { right: parent.right; rightMargin: 4 + 8; verticalCenter: parent.verticalCenter }
                }

                Components.HapticMouseArea {
                    anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                    width: parent.width / 2
                    onClicked: coverFlowMode = false
                }
                Components.HapticMouseArea {
                    anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
                    width: parent.width / 2
                    onClicked: coverFlowMode = true
                }
            }

            Components.SearchField {
                id: inlineSearch
                visible: searchMode
                placeholderText: qsTr("Search…")
                anchors {
                    left: parent.left; leftMargin: 16
                    right: searchToggleBtn.left; rightMargin: 8
                    verticalCenter: parent.verticalCenter
                }
                height: 56
                inputField.onAccepted: {
                    searchDebounce.stop();
                    mediaBrowser.doSearch(inputField.text);
                }
                inputField.onTextChanged: {
                    if (inputField.text.trim()) searchDebounce.restart();
                    else searchDebounce.stop();
                }
            }

            Components.Icon {
                id: searchToggleBtn
                icon: searchMode ? "uc:xmark" : "uc:magnifying-glass"
                size: 60
                color: colors.offwhite
                visible: entityObj.hasFeature(MediaPlayerFeatures.Search_media) && isAtRoot
                anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        mediaBrowser.searchMode = !mediaBrowser.searchMode;
                        if (mediaBrowser.searchMode) inlineSearch.focus();
                        else { mediaBrowser.clearSearch(); mediaBrowser.loadRoot(); }
                    }
                }
            }
        }

        // ----------- filter chip row (search mode only) -----------
        Item {
            id: filterRow
            anchors { top: browserHeader.bottom; left: parent.left; right: parent.right }
            height: (searchMode && mediaBrowser.availableSearchMediaClasses.length > 0) ? 52 : 0
            clip: true
            Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

            Flickable {
                anchors { fill: parent; topMargin: 8; bottomMargin: 8; leftMargin: 16 }
                flickableDirection: Flickable.HorizontalFlick
                contentWidth: chipRow.implicitWidth
                clip: true

                Row {
                    id: chipRow
                    spacing: 8
                    height: parent.height

                    Repeater {
                        model: mediaBrowser.availableSearchMediaClasses

                        delegate: Rectangle {
                            property bool active: mediaBrowser.selectedMediaClasses.indexOf(modelData) >= 0
                            height: chipRow.height
                            width: chipText.implicitWidth + 24
                            radius: height / 2
                            color: active ? colors.primaryButton : colors.dark

                            Text {
                                id: chipText
                                anchors.centerIn: parent
                                text: mediaBrowser.displayLabelForMediaClass(modelData)
                                color: colors.offwhite
                                font: fonts.secondaryFont(22)
                            }

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    var classes = mediaBrowser.selectedMediaClasses.slice();
                                    var idx = classes.indexOf(modelData);
                                    if (idx >= 0) classes.splice(idx, 1);
                                    else classes.push(modelData);
                                    mediaBrowser.selectedMediaClasses = classes;
                                    var q = inlineSearch.inputField.text.trim();
                                    if (q) {
                                        searchDebounce.stop();
                                        mediaBrowser.searchLoading = true;
                                        mediaBrowser.searchNoResults = false;
                                        entityObj.searchMedia(q, "", "", classes, mediaBrowser.defaultPageLimit, 1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ----------- navigation stack -----------
        StackView {
            id: browseNav
            anchors { top: filterRow.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
            clip: true

            pushEnter: Transition {
                XAnimator { from: browseNav.width; to: 0; duration: 300; easing.type: Easing.OutCubic }
            }
            pushExit: Transition {
                XAnimator { from: 0; to: -browseNav.width; duration: 300; easing.type: Easing.OutCubic }
            }
            popEnter: Transition {
                XAnimator { from: -browseNav.width; to: 0; duration: 300; easing.type: Easing.OutCubic }
            }
            popExit: Transition {
                XAnimator { from: 0; to: browseNav.width; duration: 300; easing.type: Easing.OutCubic }
            }
        }

        Components.PopupMenu { id: popupMenu }
    }

    // ----------- page component (one instance per navigation level) -----------
    Component {
        id: levelPage

        Item {
            id: pageRoot

            // per-level state (set via StackView.push properties)
            property string pageTitle:        ""
            property string pageMediaId:      ""
            property string pageMediaType:    ""
            property string pageThumbnail:    ""
            property bool   pageLoading:      true
            property var    pageItems:        []
            property var    pageContainer:    null
            property int    pagePage:         1
            property int    requestedPage:    1
            property int    pageLimit:        mediaBrowser.defaultPageLimit
            property bool   pageHasMore:      false
            property bool   pageLoadingMore:  false
            readonly property bool isContainerView: pageContainer !== null &&
                (pageContainer.media_class === "album" || pageContainer.media_class === "playlist")

            // items shown in the list: search results when searching at root, else pageItems
            readonly property var displayItems: (mediaBrowser.isAtRoot && mediaBrowser.searchMode)
                                                 ? mediaBrowser.searchResults : pageItems

            // sentinel-wrapped model for the coverflow (root only)
            property var coverFlowItems: {
                if (pageItems.length === 0) return [];
                return [{__sentinel: true}].concat(pageItems).concat([{__sentinel: true}]);
            }

            property alias pageListView:     listView
            property alias pageCoverFlowView: coverFlowView

            // ----------- no-results state -----------
            Column {
                anchors.centerIn: parent
                visible: !pageLoading &&
                         ((mediaBrowser.searchMode && mediaBrowser.searchNoResults) ||
                          (!mediaBrowser.searchMode && !mediaBrowser.searchLoading && displayItems.length === 0))
                spacing: 8
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("No results")
                    color: colors.offwhite
                    font: fonts.primaryFont(46, "Bold")
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Try something else.")
                    color: colors.light
                    font: fonts.secondaryFont(26)
                }
            }

            // ----------- list view -----------
            ListView {
                id: listView
                visible: (!mediaBrowser.coverFlowMode || isContainerView) &&
                         (!pageLoading || displayItems.length > 0)
                clip: true
                model: displayItems
                spacing: 10
                anchors.fill: parent
                onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                onMovementEnded: {
                    if (atYEnd && !pageLoadingMore && pageHasMore) {
                        mediaBrowser.loadMore();
                        Qt.callLater(positionViewAtEnd);
                    }
                }

                footer: Item {
                    width: listView.width
                    height: pageRoot.pageLoadingMore ? 60 : 0

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: pageRoot.pageLoadingMore
                        width: 40; height: 40
                    }
                }

                // album / playlist header
                header: Item {
                    width: listView.width
                    height: isContainerView ? containerHeader.implicitHeight + 50 : 0
                    visible: isContainerView

                    Column {
                        id: containerHeader
                        width: parent.width
                        anchors { top: parent.top; topMargin: 10 }
                        spacing: 20

                        MediaPlayerComponents.ImageLoader {
                            id: headerArt
                            width: 260; height: 260
                            anchors.horizontalCenter: parent.horizontalCenter
                            url: pageThumbnail
                            aspectFit: false

                            Rectangle {
                                anchors.fill: parent; color: colors.dark; radius: 10
                                visible: !pageThumbnail
                            }
                            Components.Icon {
                                anchors.centerIn: parent
                                icon: (pageContainer && pageContainer.thumbnail &&
                                       pageContainer.thumbnail.startsWith("icon://"))
                                      ? pageContainer.thumbnail.replace("icon://", "") : "uc:music"
                                size: 120; color: colors.medium
                                visible: !pageThumbnail || parent.failed
                            }

                            Components.Icon {
                                id: headerPlayPendingIcon
                                anchors.centerIn: parent
                                icon: mediaBrowser.pendingPlayIcon
                                size: 88
                                color: colors.offwhite
                                visible: mediaBrowser.pendingPlayMediaId !== "" &&
                                         mediaBrowser.pendingPlayMediaId === ((pageContainer && pageContainer.media_id) || "")
                                opacity: 0
                                scale: 1.35

                                onVisibleChanged: {
                                    if (visible) {
                                        headerPlayPendingAppear.stop();
                                        scale = 1.35;
                                        opacity = 0;
                                        headerPlayPendingAppear.start();
                                    }
                                }

                                SequentialAnimation {
                                    id: headerPlayPendingAppear
                                    running: false
                                    ParallelAnimation {
                                        NumberAnimation { target: headerPlayPendingIcon; property: "scale"; to: 1.0; duration: 260; easing.type: Easing.OutExpo }
                                        NumberAnimation { target: headerPlayPendingIcon; property: "opacity"; to: 1.0; duration: 220; easing.type: Easing.OutCubic }
                                    }
                                }
                            }
                        }

                        Column {
                            width: parent.width; spacing: 8
                            MediaPlayerComponents.MarqueeText {
                                width: parent.width
                                text: pageContainer ? (pageContainer.title || "") : ""
                                color: colors.offwhite; font: fonts.primaryFont(36, "Medium")
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Text {
                                width: parent.width
                                text: pageContainer ? (pageContainer.artist || "") : ""
                                color: colors.light; font: fonts.secondaryFont(22)
                                elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter
                                visible: text !== ""
                            }
                        }

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter; spacing: 40
                            Rectangle {
                                width: 120; height: 70; radius: 35; color: colors.primaryButton

                                Components.Icon { icon: "uc:play"; size: 60; color: colors.offwhite; anchors.centerIn: parent }

                                Components.HapticMouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (!pageContainer) return;
                                        if (entityObj.hasFeature(MediaPlayerFeatures.Play_media_action)) {
                                            var mediaId   = pageContainer.media_id;
                                            var mediaType = pageContainer.media_type;
                                            popupMenu.title     = pageContainer.title || "";
                                            popupMenu.menuItems = buildPlayMenu(mediaId, mediaType, pageContainer.play_media_action);
                                            popupMenu.open();
                                        } else {
                                            mediaBrowser.requestPlayMedia(pageContainer.media_id, pageContainer.media_type);
                                        }
                                    }
                                }
                            }
                            Rectangle {
                                width: 120; height: 70; radius: 35; color: colors.primaryButton

                                Components.Icon { icon: "uc:shuffle"; size: 60; color: colors.offwhite; anchors.centerIn: parent }

                                Components.HapticMouseArea {
                                    anchors.fill: parent
                                    onClicked: { if (pageContainer) mediaBrowser.requestPlayMedia(pageContainer.media_id, pageContainer.media_type, "SHUFFLE") }
                                }
                            }
                        }
                    }
                }

                delegate: Item {
                    id: itemDelegate
                    width: listView.width
                    height: 80

                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (modelData.can_browse)
                                mediaBrowser.browseInto(modelData.media_id, modelData.media_type, modelData.title, modelData.thumbnail);
                            else if (modelData.can_play)
                                mediaBrowser.requestPlayMedia(modelData.media_id, modelData.media_type);
                        }
                    }

                    Rectangle {
                        anchors.fill: parent; color: colors.medium; radius: 10
                        visible: itemDelegate.ListView.isCurrentItem
                    }

                    property bool playPending: mediaBrowser.pendingPlayMediaId !== "" &&
                                               mediaBrowser.pendingPlayMediaId === (modelData.media_id || "")

                    Text {
                        id: trackNumber
                        text: (index + 1).toString() + "."
                        color: colors.offwhite; font: fonts.secondaryFont(22)
                        width: 44; horizontalAlignment: Text.AlignRight
                        anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
                        visible: isContainerView
                    }

                    MediaPlayerComponents.ImageLoader {
                        id: thumb
                        width: 60; height: 60
                        url: (!isContainerView && modelData.thumbnail &&
                              !modelData.thumbnail.startsWith("icon://")) ? modelData.thumbnail : ""
                        aspectFit: true
                        anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
                        visible: !isContainerView
                    }

                    Components.Icon {
                        icon: (modelData.thumbnail && modelData.thumbnail.startsWith("icon://"))
                              ? modelData.thumbnail.replace("icon://", "") : "uc:music"
                        size: 52; color: colors.offwhite
                        visible: !isContainerView && (!thumb.url || thumb.failed)
                        anchors.centerIn: thumb
                    }

                    Column {
                        anchors {
                            left: isContainerView ? trackNumber.right : thumb.right; leftMargin: 12
                            right: rightAction.left; rightMargin: 8
                            verticalCenter: parent.verticalCenter
                        }
                        spacing: 2

                        MediaPlayerComponents.MarqueeText {
                            text: modelData.title || ""; color: colors.offwhite
                            font: fonts.primaryFont(24); width: parent.width
                            running: itemDelegate.ListView.isCurrentItem
                            elide: Text.ElideRight
                        }

                        Text {
                            text: modelData.subtitle ||
                                  (isContainerView ? (modelData.artist || "")
                                                   : (modelData.artist || modelData.album || modelData.media_class || ""))
                            color: colors.light; font: fonts.secondaryFontCapitalizedFirst(22)
                            elide: Text.ElideRight; width: parent.width; visible: text !== ""
                        }
                    }

                    Item {
                        id: rightAction
                        width: isContainerView ? (durationText.visible ? durationText.implicitWidth + 68 : 60) : 60
                        anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }

                        Text {
                            id: durationText
                            visible: isContainerView && (modelData.duration || 0) > 0
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            text: {
                                var s = modelData.duration || 0;
                                return Math.floor(s / 60) + ":" + (s % 60 < 10 ? "0" : "") + (s % 60);
                            }
                            color: colors.medium; font: fonts.secondaryFont(22)
                        }

                        // --- container view: simple play (feature not supported) ---
                        Components.Icon {
                            icon: "uc:play"; size: 56; color: colors.offwhite
                            visible: !itemDelegate.playPending &&
                                     isContainerView && modelData.can_play &&
                                     !entityObj.hasFeature(MediaPlayerFeatures.Play_media_action)
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: mediaBrowser.requestPlayMedia(modelData.media_id, modelData.media_type)
                            }
                        }

                        // --- container view: ellipsis menu (feature supported) ---
                        Components.Icon {
                            icon: "uc:ellipsis"; size: 56; color: colors.offwhite
                            visible: !itemDelegate.playPending &&
                                     isContainerView && modelData.can_play &&
                                     entityObj.hasFeature(MediaPlayerFeatures.Play_media_action)
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    var mediaId   = modelData.media_id;
                                    var mediaType = modelData.media_type;
                                    popupMenu.title     = modelData.title || "";
                                    popupMenu.menuItems = buildPlayMenu(mediaId, mediaType, modelData.play_media_action);
                                    popupMenu.open();
                                }
                            }
                        }

                        // --- simple play (feature not supported) ---
                        Components.Icon {
                            icon: "uc:play"; size: 56; color: colors.offwhite
                            visible: !itemDelegate.playPending &&
                                     !isContainerView && modelData.can_play &&
                                     !entityObj.hasFeature(MediaPlayerFeatures.Play_media_action)
                            anchors.centerIn: parent

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: mediaBrowser.requestPlayMedia(modelData.media_id, modelData.media_type)
                            }
                        }

                        Components.Icon {
                            id: playPendingIcon
                            anchors.centerIn: parent
                            icon: mediaBrowser.pendingPlayIcon
                            size: 56
                            color: colors.offwhite
                            visible: itemDelegate.playPending
                            opacity: 0
                            scale: 1.35

                            onVisibleChanged: {
                                if (visible) {
                                    playPendingAppear.stop();
                                    scale = 1.35;
                                    opacity = 0;
                                    playPendingAppear.start();
                                }
                            }

                            SequentialAnimation {
                                id: playPendingAppear
                                running: false
                                ParallelAnimation {
                                    NumberAnimation { target: playPendingIcon; property: "scale"; to: 1.0; duration: 260; easing.type: Easing.OutExpo }
                                    NumberAnimation { target: playPendingIcon; property: "opacity"; to: 1.0; duration: 220; easing.type: Easing.OutCubic }
                                }
                            }
                        }

                        // --- ellipsis menu (feature supported) ---
                        Components.Icon {
                            icon: "uc:ellipsis"; size: 56; color: colors.offwhite
                            visible: !itemDelegate.playPending &&
                                     !isContainerView && modelData.can_play &&
                                     entityObj.hasFeature(MediaPlayerFeatures.Play_media_action)
                            anchors.centerIn: parent

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    var mediaId   = modelData.media_id;
                                    var mediaType = modelData.media_type;
                                    popupMenu.title     = modelData.title || "";
                                    popupMenu.menuItems = buildPlayMenu(mediaId, mediaType, modelData.play_media_action);
                                    popupMenu.open();
                                }
                            }
                        }
                    }
                }

                Components.ScrollIndicator { parentObj: listView }
            }

            // ----------- coverflow (root page only) -----------
            Item {
                id: coverFlowContainer
                visible: mediaBrowser.coverFlowMode && !isContainerView &&
                         (!pageLoading || pageItems.length > 0)
                anchors.fill: parent

                property real artSize: Math.min(parent.width - 32, parent.height - 140)
                property real frontY:  height - 110 - artSize / 2

                PathView {
                    id: coverFlowView
                    anchors.fill: parent
                    interactive: true
                    clip: true
                    model: pageRoot.coverFlowItems
                    pathItemCount: Math.min(pageRoot.coverFlowItems.length, 5)
                    preferredHighlightBegin: 0.5
                    preferredHighlightEnd: 0.5
                    highlightRangeMode: PathView.StrictlyEnforceRange

                    property int prevIndex: 1

                    onCurrentIndexChanged: {
                        var last = pageRoot.coverFlowItems.length - 1;
                        if (currentIndex === 0 || prevIndex === 0) {
                            if (dragging) interactive = false;
                            currentIndex = 1;
                        } else if (currentIndex === last || prevIndex === last) {
                            if (dragging) interactive = false;
                            currentIndex = last - 1;
                        }
                        prevIndex = currentIndex;
                        var realIndex = currentIndex - 1;
                        if (listView.currentIndex !== realIndex) listView.currentIndex = realIndex;
                        if (realIndex === pageRoot.pageItems.length - 1) mediaBrowser.loadMore();
                    }

                    onDraggingChanged: { if (!dragging) interactive = true; }

                    Binding {
                        target: coverFlowView
                        property: "currentIndex"
                        value: listView.currentIndex + 1
                        when: !coverFlowView.moving
                    }

                    // length-balanced path: L1=artSize (past, hidden), L2=0.4×artSize (first peek),
                    // L3=0.6×artSize (second peek). With pathItemCount=5 & 0.2 spacing,
                    // t=0.5→front, t=0.7→first peek, t=0.9→second peek exactly.
                    path: Path {
                        startX: coverFlowView.width / 2
                        startY: coverFlowContainer.frontY + coverFlowContainer.artSize

                        PathAttribute { name: "itemScale";   value: 0.50 }
                        PathAttribute { name: "itemZ";       value: 1    }
                        PathAttribute { name: "itemOpacity"; value: 0.0  }

                        PathLine { x: coverFlowView.width / 2; y: coverFlowContainer.frontY }
                        PathAttribute { name: "itemScale";   value: 1.0  }
                        PathAttribute { name: "itemZ";       value: 50   }
                        PathAttribute { name: "itemOpacity"; value: 1.0  }

                        PathLine { x: coverFlowView.width / 2; y: coverFlowContainer.frontY - coverFlowContainer.artSize * 0.4 }
                        PathAttribute { name: "itemScale";   value: 0.72 }
                        PathAttribute { name: "itemZ";       value: 10   }
                        PathAttribute { name: "itemOpacity"; value: 0.9  }

                        PathLine { x: coverFlowView.width / 2; y: coverFlowContainer.frontY - coverFlowContainer.artSize }
                        PathAttribute { name: "itemScale";   value: 0.50 }
                        PathAttribute { name: "itemZ";       value: 1    }
                        PathAttribute { name: "itemOpacity"; value: 0.85 }
                    }

                    delegate: Item {
                        id: cfDelegate
                        width: coverFlowContainer.artSize
                        height: coverFlowContainer.artSize
                        scale: PathView.itemScale
                        z: PathView.itemZ
                        property bool playPending: mediaBrowser.pendingPlayMediaId !== "" &&
                                                   mediaBrowser.pendingPlayMediaId === (modelData.media_id || "")
                        visible: !modelData.__sentinel && index >= coverFlowView.currentIndex
                        opacity: visible ? PathView.itemOpacity : 0

                        layer.enabled: true
                        layer.effect: OpacityMask {
                            maskSource: Rectangle { width: cfDelegate.width; height: cfDelegate.height; radius: 16 }
                        }

                        Rectangle { anchors.fill: parent; color: colors.medium }

                        MediaPlayerComponents.ImageLoader {
                            id: cfArt
                            anchors.fill: parent
                            url: (modelData.thumbnail && !modelData.thumbnail.startsWith("icon://"))
                                 ? modelData.thumbnail : ""
                            aspectFit: false
                        }

                        Components.Icon {
                            anchors.centerIn: parent
                            icon: (modelData.thumbnail && modelData.thumbnail.startsWith("icon://"))
                                  ? modelData.thumbnail.replace("icon://", "") : "uc:music"
                            size: coverFlowContainer.artSize * 0.5
                            color: colors.offwhite
                            visible: !cfArt.url || cfArt.failed
                        }

                        Components.Icon {
                            id: coverFlowPlayPendingIcon
                            anchors.centerIn: parent
                            icon: mediaBrowser.pendingPlayIcon
                            size: coverFlowContainer.artSize * 0.34
                            color: colors.offwhite
                            visible: cfDelegate.playPending
                            opacity: 0
                            scale: 1.35

                            onVisibleChanged: {
                                if (visible) {
                                    coverFlowPlayPendingAppear.stop();
                                    scale = 1.35;
                                    opacity = 0;
                                    coverFlowPlayPendingAppear.start();
                                }
                            }

                            SequentialAnimation {
                                id: coverFlowPlayPendingAppear
                                running: false
                                ParallelAnimation {
                                    NumberAnimation { target: coverFlowPlayPendingIcon; property: "scale"; to: 1.0; duration: 260; easing.type: Easing.OutExpo }
                                    NumberAnimation { target: coverFlowPlayPendingIcon; property: "opacity"; to: 1.0; duration: 220; easing.type: Easing.OutCubic }
                                }
                            }
                        }

                        Components.HapticMouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (index !== coverFlowView.currentIndex) {
                                    listView.currentIndex = index - 1;
                                } else {
                                    var item = pageRoot.pageItems[index - 1];
                                    if (item && item.can_browse)
                                        mediaBrowser.browseInto(item.media_id, item.media_type, item.title, item.thumbnail);
                                    else if (item && item.can_play) {
                                        if (entityObj.hasFeature(MediaPlayerFeatures.Play_media_action)) {
                                            var mediaId   = item.media_id;
                                            var mediaType = item.media_type;
                                            popupMenu.title     = item.title || "";
                                            popupMenu.menuItems = buildPlayMenu(mediaId, mediaType, item.play_media_action);
                                            popupMenu.open();
                                        } else {
                                            mediaBrowser.requestPlayMedia(item.media_id, item.media_type);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // fixed title / artist strip at bottom
                Column {
                    anchors { bottom: parent.bottom; bottomMargin: 30; left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20 }
                    spacing: 6

                    property var cfItem: (pageRoot.pageItems.length > 0 && coverFlowView.currentIndex >= 1)
                                         ? pageRoot.pageItems[coverFlowView.currentIndex - 1] : null

                    MediaPlayerComponents.MarqueeText {
                        width: parent.width
                        text: parent.cfItem ? (parent.cfItem.title || "") : ""
                        color: colors.offwhite; font: fonts.primaryFont(34, "Medium")
                    }
                    Text {
                        width: parent.width
                        text: parent.cfItem ? (parent.cfItem.subtitle || parent.cfItem.artist || parent.cfItem.album || parent.cfItem.media_class || "") : ""
                        color: colors.light; font: fonts.secondaryFont(22)
                        elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter
                        visible: text !== ""
                    }
                }
            }
        }
    }
}
