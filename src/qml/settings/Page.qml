// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15

import "qrc:/components" as Components
import "qrc:/settings" as Settings

Item {
    id: settingsPageBase
    width: parent.width; height: parent.height;

    // Set this to the page's Flickable to keep the d-pad focused control on screen.
    // Flickable does not follow the keyboard focus by itself, so without this the focus chain walks
    // off the bottom of the display on any page taller than the viewport.
    property Flickable scrollTarget: null

    // The control that should hold the keypad focus when the page opens.
    // Set this instead of calling forceActiveFocus() from a control's Component.onCompleted: the
    // level-2/3 Loaders are asynchronous, so the page being replaced is sometimes destroyed after
    // the new page has completed, and destroying the focused item clears the window's active focus
    // again. Declaring it here lets the page reclaim the focus once it owns the input.
    property Item initialFocusItem: null

    readonly property Item focusedItem: Window.activeFocusItem

    function claimFocus() {
        if (!initialFocusItem || ui.inputController.activeItem !== settingsPageBase) {
            return;
        }

        // never steal the focus from a control the user already moved to inside this page
        if (isChildOf(settingsPageBase.focusedItem, settingsPageBase)) {
            return;
        }

        initialFocusItem.forceActiveFocus();
    }

    onInitialFocusItemChanged: settingsPageBase.claimFocus()

    onFocusedItemChanged: {
        settingsPageBase.ensureVisible(settingsPageBase.focusedItem);

        if (!settingsPageBase.focusedItem) {
            settingsPageBase.claimFocus();
        }
    }

    Connections {
        target: ui.inputController
        ignoreUnknownSignals: true

        function onActiveItemChanged() {
            settingsPageBase.claimFocus();
        }
    }

    function isChildOf(item, ancestor) {
        for (let p = item; p; p = p.parent) {
            if (p === ancestor) {
                return true;
            }
        }

        return false;
    }

    function ensureVisible(item) {
        if (!scrollTarget || !item || !isChildOf(item, scrollTarget.contentItem)) {
            return;
        }

        const margin = 40;
        const pos = item.mapToItem(scrollTarget.contentItem, 0, 0);
        const top = pos.y - margin;
        const bottom = pos.y + item.height + margin;
        const maxContentY = Math.max(0, scrollTarget.contentHeight - scrollTarget.height);

        if (top < scrollTarget.contentY) {
            scrollTarget.contentY = Math.min(Math.max(0, top), maxContentY);
        } else if (bottom > scrollTarget.contentY + scrollTarget.height) {
            scrollTarget.contentY = Math.min(Math.max(0, bottom - scrollTarget.height), maxContentY);
        }
    }

    property var parentSwipeView
    property alias topNavigation: topNavigation
    property alias topNavigationText: topNavigation.text
    property alias buttonNavigation: buttonNavigation

    Settings.TopNavigation {
        id: topNavigation
        anchors.top: parent.top
        goBack: profileRoot.goBack
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    profileRoot.goBack();
                    buttonNavigation.restoreDefaultConfig();
                }
            },
            "HOME": {
                "pressed": function() {
                    profileRoot.goHome();
                }
            }
        }
    }
}
