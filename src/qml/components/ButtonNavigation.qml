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

Item {
    id: buttonNavigation
    property Item scope: buttonNavigation.parent

    property bool ignoreInput: false

    property bool overrideActive: false
    property var defaultConfig: ({})
    property var defaultConfigOriginal: ({})
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
        buttonNavigation.defaultConfigOriginal = cloneConfig(buttonNavigation.defaultConfig);
        const nextConfig = cloneConfig(buttonNavigation.defaultConfig);

        for (const [key, value] of Object.entries(config)) {
            if (config[key]) {
                nextConfig[key] = value && typeof value === "object" ? Object.assign({}, value) : value;
            }
        }

        buttonNavigation.defaultConfig = nextConfig;
    }

    function restoreDefaultConfig() {
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
