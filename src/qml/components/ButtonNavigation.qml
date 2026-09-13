// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 BUTTON NAVIGATION COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - overrideActive
 - defaultConfig
 - overrideConfig
**/

import QtQuick 2.15
import QtQuick.Window 2.15

Item {
    id: buttonNavigation
    property Item scope: buttonNavigation.parent

    property bool ignoreInput: false

    /**
      FOCUS OWNERSHIP (opt-in, only needed when the scope navigates via the QML focus chain)

      A key press travels two independent paths: the input controller, which routes it to whichever
      scope holds takeControl(), and the QML focus chain (KeyNavigation, Keys handlers, ListView
      arrows). takeControl() does not move the keyboard focus, so a page keeps its focus - and keeps
      reacting to the same key press - while a popup is open on top of it.

      With manageFocus the keyboard focus follows the input ownership: the focus is parked on this
      inert item while another layer owns the input, and handed back to the control the user was on
      when the scope becomes the front layer again.
     */
    property bool manageFocus: false

    // control that should hold the focus the first time the scope becomes the front layer
    property Item initialFocusItem: null

    // control the user was last on inside this scope, restored when the scope comes back to front
    property Item lastFocusItem: null

    // Fallback for lastFocusItem: a ListView delegate holding the focus is destroyed when its model
    // is rebuilt (a wifi scan result) while a popup owns the input, and an Item property is nulled
    // with it. A container that declares `property bool keypadFocusAnchor: true` (the ListView) is
    // remembered alongside, so the scope comes back to the list instead of its first control.
    property Item lastFocusAnchor: null

    function focusAnchorOf(item) {
        for (let p = item; p; p = p.parent) {
            if (p === scope) {
                return null;
            }

            if (p.keypadFocusAnchor === true) {
                return p;
            }
        }

        return null;
    }

    // Set this to the scope's Flickable to keep the focused control on screen. Flickable does not
    // follow the keyboard focus by itself, so without this the focus chain walks off the bottom of
    // the display on any page taller than the viewport.
    property Flickable scrollTarget: null

    function isChildOf(item, ancestor) {
        for (let p = item; p; p = p.parent) {
            if (p === ancestor) {
                return true;
            }
        }

        return false;
    }

    // The rectangle to reveal for a focused control: the control's section - the direct child of the
    // top-level layout inside the Flickable (a settings row with its title and description, a slider
    // with its title) - as long as that section fits into the viewport; otherwise the control itself
    // (a ListView delegate whose section is the whole list, a drawer taller than the screen).
    function scrollSectionOf(item) {
        if (!scrollTarget) {
            return item;
        }

        const contentItem = scrollTarget.contentItem;
        for (let p = item; p && p.parent; p = p.parent) {
            if (p.parent.parent === contentItem) {
                return p;
            }

            if (p.parent === contentItem) {
                return item;
            }
        }

        return item;
    }

    function ensureVisible(item) {
        if (!scrollTarget || !item || !isChildOf(item, scrollTarget.contentItem)) {
            return;
        }

        const margin = 40;
        let target = scrollSectionOf(item);
        if (target !== item && target.height + 2 * margin > scrollTarget.height) {
            target = item;
        }

        const pos = target.mapToItem(scrollTarget.contentItem, 0, 0);
        const top = pos.y - margin;
        const bottom = pos.y + target.height + margin;
        const maxContentY = Math.max(0, scrollTarget.contentHeight - scrollTarget.height);

        if (top < scrollTarget.contentY) {
            scrollTarget.contentY = Math.min(Math.max(0, top), maxContentY);
        } else if (bottom > scrollTarget.contentY + scrollTarget.height) {
            scrollTarget.contentY = Math.min(Math.max(0, bottom - scrollTarget.height), maxContentY);
        }
    }

    // Scroll the scrollTarget by delta pixels, clamped to its content. Returns whether it moved, so a
    // key handler at the end of a focus chain can report the key as handled only when there was
    // something left to scroll to.
    function scrollBy(delta) {
        if (!scrollTarget) {
            return false;
        }

        const maxContentY = Math.max(0, scrollTarget.contentHeight - scrollTarget.height);
        const next = Math.min(Math.max(0, scrollTarget.contentY + delta), maxContentY);
        if (Math.abs(next - scrollTarget.contentY) < 1) {
            return false;
        }

        scrollTarget.contentY = next;
        return true;
    }

    // the null guard keeps the binding quiet while the ui context is torn down on shutdown
    readonly property bool hasInputControl: ui && ui.inputController
                                            ? ui.inputController.activeItem === scope : false
    readonly property Item windowFocusItem: Window.activeFocusItem

    function isInScope(item) {
        for (let p = item; p; p = p.parent) {
            if (p === scope) {
                return true;
            }
        }

        return false;
    }

    function focusTarget() {
        if (lastFocusItem && lastFocusItem.visible && isInScope(lastFocusItem)) {
            return lastFocusItem;
        }

        if (lastFocusAnchor && lastFocusAnchor.visible && isInScope(lastFocusAnchor)) {
            return lastFocusAnchor;
        }

        if (initialFocusItem && initialFocusItem.visible) {
            return initialFocusItem;
        }

        return scope;
    }

    function claimFocus() {
        if (!manageFocus || !hasInputControl || !scope || !scope.visible) {
            return;
        }

        // the focus is already on a control of this scope: leave the user's selection alone.
        // buttonNavigation itself is the parking spot, not a control, so it does not count, and
        // neither does the scope itself: a swipe view hands the focus to its page wrapper on a
        // level change, and that must not pass as a selection.
        if (windowFocusItem !== buttonNavigation && windowFocusItem !== scope && isInScope(windowFocusItem)) {
            return;
        }

        const target = focusTarget();
        if (target) {
            target.forceActiveFocus();
        }
    }

    // Park the focus on this item while another layer owns the input. It carries no key handlers,
    // so the keys that still reach it do nothing, and the controls of the scope go quiet.
    function parkFocus() {
        if (!manageFocus || !isInScope(windowFocusItem) || windowFocusItem === buttonNavigation) {
            return;
        }

        // the layer that took the input can live inside this scope (a popup or form declared in
        // the page) and may have focused its own control already - that focus is its, not ours
        const owner = ui.inputController.activeItem;
        if (owner && owner !== scope && isChildOf(windowFocusItem, owner)) {
            return;
        }

        buttonNavigation.forceActiveFocus();
    }

    onManageFocusChanged: claimFocus()
    onInitialFocusItemChanged: claimFocus()

    onHasInputControlChanged: {
        if (hasInputControl) {
            // Deferred: the input usually comes back on a key press that closed the layer above
            // (Cancel in a drawer, BACK on a dialog). That key is still being delivered on the
            // focus path, and a control focused right now would act on it as well - a drawer
            // closed with OK on Cancel reopened itself through its opener row this way.
            Qt.callLater(claimFocus);
        } else {
            parkFocus();
        }
    }

    onWindowFocusItemChanged: {
        ensureVisible(windowFocusItem);

        if (!manageFocus || !hasInputControl) {
            return;
        }

        if (windowFocusItem !== buttonNavigation && windowFocusItem !== scope && isInScope(windowFocusItem)) {
            // remember where the user is, so the scope comes back to the same control
            buttonNavigation.lastFocusItem = windowFocusItem;
            buttonNavigation.lastFocusAnchor = focusAnchorOf(windowFocusItem);
            return;
        }

        // We own the input but the focus is not on one of our controls: it was parked, the focused
        // control was hidden or destroyed, or the swipe view handed the focus to its page wrapper
        // after a level change - which happens after we claimed it. Take it back - deferred, as
        // moving the focus from inside the focus change handler is a binding loop on
        // windowFocusItem, which QML aborts, and the claim then never happens.
        Qt.callLater(claimFocus);
    }

    // a scope that gains the input while it is still hidden (popup opening, swipe view animating)
    // can only take the focus once it is actually on screen
    readonly property bool scopeVisible: scope ? scope.visible : false
    onScopeVisibleChanged: claimFocus()

    // The control to come back to can still be hidden when the scope regains the input (a tile that
    // fades back in after the popup it opened has closed): the claim then lands on the scope itself.
    // Claim again once the control is visible.
    readonly property bool lastFocusVisible: lastFocusItem ? lastFocusItem.visible : false
    readonly property bool initialFocusVisible: initialFocusItem ? initialFocusItem.visible : false

    function reclaimFocus() {
        if (manageFocus && hasInputControl) {
            Qt.callLater(claimFocus);
        }
    }

    onLastFocusVisibleChanged: reclaimFocus()
    onInitialFocusVisibleChanged: reclaimFocus()

    property bool overrideActive: false
    property var defaultConfig: ({})
    property var defaultConfigOriginal: ({})
    property bool defaultConfigCaptured: false
    property var overrideConfig: ({})
    property var overrideConfigOriginal: ({})

    property var timers: ({})
    property var repeats: ({})
    property var longPressExecuted: ({})

    enum ConfigType {
        Pressed,
        PressedRepeat,
        Released,
        LongPress
    }

    function takeControl() {
        ui.inputController.takeControl(scope)
    }

    function releaseControl() {
        ui.inputController.releaseControl(scope)
    }

    function cloneConfig(config) {
        let cloned = {};

        for (const [key, value] of Object.entries(config || {})) {
            if (value && typeof value === "object") {
                cloned[key] = Object.assign({}, value);
            } else {
                cloned[key] = value;
            }
        }

        return cloned;
    }

    function ensureTimer(key) {
        let timer = buttonNavigation.timers[key];
        if (!timer) {
            timer = longPressTimer.createObject(buttonNavigation);
            buttonNavigation.timers[key] = timer;
        }

        return timer;
    }

    function stopTimer(key) {
        const timer = buttonNavigation.timers[key];
        if (!timer) {
            return;
        }

        timer.stop();
        timer.keyName = "";
        timer.action = undefined;
    }

    function destroyTimer(key) {
        const timer = buttonNavigation.timers[key];
        if (!timer) {
            return;
        }

        stopTimer(key);
        timer.destroy();
        delete buttonNavigation.timers[key];
    }

    function extendDefaultConfig(config) {
        // capture the untouched config once: extending twice must not make the second extension
        // the "original" the scope is restored to
        if (!buttonNavigation.defaultConfigCaptured) {
            buttonNavigation.defaultConfigOriginal = cloneConfig(buttonNavigation.defaultConfig);
            buttonNavigation.defaultConfigCaptured = true;
        }

        const nextConfig = cloneConfig(buttonNavigation.defaultConfig);

        for (const [key, value] of Object.entries(config)) {
            if (config[key]) {
                nextConfig[key] = value && typeof value === "object" ? Object.assign({}, value) : value;
            }
        }

        buttonNavigation.defaultConfig = nextConfig;
    }

    function restoreDefaultConfig() {
        // nothing was ever extended: restoring the empty original would drop the handlers the
        // scope declared itself, leaving it without a way out
        if (!buttonNavigation.defaultConfigCaptured) {
            return;
        }

        buttonNavigation.defaultConfig = cloneConfig(buttonNavigation.defaultConfigOriginal);
    }

    function extendOverrideConfig(config, overWrite = false) {
        buttonNavigation.overrideConfigOriginal = cloneConfig(buttonNavigation.overrideConfig);
        const nextConfig = cloneConfig(buttonNavigation.overrideConfig);

        for (const [key, value] of Object.entries(config)) {
            if (config[key]) {
                if (overWrite || !nextConfig[key] || typeof value !== "object") {
                    nextConfig[key] = value && typeof value === "object" ? Object.assign({}, value) : value;
                } else {
                    nextConfig[key] = Object.assign({}, nextConfig[key], value);
                }
            }
        }

        buttonNavigation.overrideConfig = nextConfig;
    }

    function restoreOverrideConfig() {
        buttonNavigation.overrideConfig = cloneConfig(buttonNavigation.overrideConfigOriginal);
    }

    function hasConfig(key, type) {
        switch (type) {
        case ButtonNavigation.ConfigType.Pressed:
            if (overrideConfig[key] && overrideConfig[key].pressed) {
                return true;
            } else if (defaultConfig[key] && defaultConfig[key].pressed) {
                return true;
            } else {
                return false;
            }
        case ButtonNavigation.ConfigType.PressedRepeat:
            if (overrideConfig[key] && overrideConfig[key].pressed_repeat) {
                return true;
            } else if (defaultConfig[key] && defaultConfig[key].pressed_repeat) {
                return true;
            } else {
                return false;
            }
        case ButtonNavigation.ConfigType.Released:
            if (overrideConfig[key] && overrideConfig[key].released) {
                return true;
            } else if (defaultConfig[key] && defaultConfig[key].released) {
                return true;
            } else {
                return false;
            }
        case ButtonNavigation.ConfigType.LongPress:
            if (overrideConfig[key] && overrideConfig[key].long_press) {
                return true;
            } else if (defaultConfig[key] && defaultConfig[key].long_press) {
                return true;
            } else {
                return false;
            }
        }
    }

    function executeCommand(key, type) {
        switch (type) {
        case ButtonNavigation.ConfigType.Pressed:
            if (overrideConfig[key] && overrideConfig[key].pressed) {
                overrideConfig[key].pressed();
                return;
            } else if (defaultConfig[key] && defaultConfig[key].pressed) {
                defaultConfig[key].pressed();
                return;
            } else {
                return;
            }
        case ButtonNavigation.ConfigType.PressedRepeat:
            if (overrideConfig[key] && overrideConfig[key].pressed_repeat) {
                overrideConfig[key].pressed_repeat();
                return
            } else if (defaultConfig[key] && defaultConfig[key].pressed_repeat) {
                defaultConfig[key].pressed_repeat();
                return;
            } else {
                return;
            }
        case ButtonNavigation.ConfigType.Released:
            if (overrideConfig[key] && overrideConfig[key].released) {
                overrideConfig[key].released();
                return;
            } else if (defaultConfig[key] && defaultConfig[key].released) {
                defaultConfig[key].released();
                return;
            } else {
                return;
            }
        case ButtonNavigation.ConfigType.LongPress:
            if (overrideConfig[key] && overrideConfig[key].long_press) {
                overrideConfig[key].long_press();
                return;
            } else if (defaultConfig[key] && defaultConfig[key].long_press) {
                defaultConfig[key].long_press();
                return;
            } else {
                return;
            }
        }
    }

    Connections {
        id: inputControllerConnection
        target: ui.inputController
        enabled: true

        function handlesOwner(owner) {
            return buttonNavigation.overrideActive || owner === buttonNavigation.scope;
        }

        function onKeyPressedFor(owner, key) {
            if (!handlesOwner(owner)) {
                return;
            }

            if (buttonNavigation.ignoreInput) {
                return;
            }

            if (hasConfig(key, ButtonNavigation.ConfigType.LongPress) === true && (buttonNavigation.repeats[key] === false || !buttonNavigation.repeats[key])) {
                stopTimer(key);

                if (buttonNavigation.longPressExecuted[key]) {
                    delete buttonNavigation.longPressExecuted[key];
                }

                buttonNavigation.repeats[key] = true;
                const timer = ensureTimer(key);
                timer.keyName = key;
                timer.action = function() {
                    const timerKey = key;
                    stopTimer(timerKey);
                    buttonNavigation.longPressExecuted[timerKey] = true;
                    executeCommand(timerKey, ButtonNavigation.ConfigType.LongPress);
                };
                timer.restart();
            } else if (hasConfig(key, ButtonNavigation.ConfigType.LongPress) === false && (buttonNavigation.repeats[key] === false || !buttonNavigation.repeats[key])) {
                buttonNavigation.repeats[key] = true;
                executeCommand(key, ButtonNavigation.ConfigType.Pressed);
            } else if (hasConfig(key, ButtonNavigation.ConfigType.LongPress) === false && buttonNavigation.repeats[key] === true) {
                if (hasConfig(key, ButtonNavigation.ConfigType.PressedRepeat)) {
                    executeCommand(key, ButtonNavigation.ConfigType.PressedRepeat);
                } else {
                    executeCommand(key, ButtonNavigation.ConfigType.Pressed);
                }
            }
        }

        function onKeyReleasedFor(owner, key) {
            if (!handlesOwner(owner)) {
                return;
            }

            buttonNavigation.repeats[key] = false;

            if (timers[key] && timers[key].running) {
                stopTimer(key);

                if (!buttonNavigation.ignoreInput) {
                    executeCommand(key, ButtonNavigation.ConfigType.Pressed);
                }
            }

            if (buttonNavigation.longPressExecuted[key]) {
                delete buttonNavigation.longPressExecuted[key];
            }

            if (buttonNavigation.ignoreInput) {
                return;
            }

            if (hasConfig(key, ButtonNavigation.ConfigType.Released)) {
                executeCommand(key, ButtonNavigation.ConfigType.Released);
            }
        }
    }

    Component {
        id: longPressTimer
        Timer {
            property string keyName
            property var action
            running: false
            interval: 800
            repeat: false
            onTriggered: {
                if (action && buttonNavigation.timers[keyName] === this) {
                    action();
                }
            }
        }
    }

    Component.onDestruction: {
        for (const key of Object.keys(buttonNavigation.timers)) {
            destroyTimer(key);
        }
    }
}
