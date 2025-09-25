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
        ui.inputController.takeControl(String(buttonNavigation.parent));
        console.info("Button control enabled for: " + String(buttonNavigation.parent));
    }

    function releaseControl(newNavigation = "") {
        ui.inputController.releaseControl(newNavigation);
        console.info("Button control disabled for: " + String(buttonNavigation.parent));
    }

    function extendDefaultConfig(config) {
        console.info("Extending default config for: " + String(buttonNavigation.parent));
        buttonNavigation.defaultConfigOriginal = buttonNavigation.defaultConfig;

        for (const [key, value] of Object.entries(config)) {
            if (config[key]) {
                defaultConfig[key] = value;
            }
        }
    }

    function restoreDefaultConfig() {
        console.info("Restoring default config for: " + String(buttonNavigation.parent));
        buttonNavigation.defaultConfig = buttonNavigation.defaultConfigOriginal;
    }

    function extendOverrideConfig(config, overWrite = false) {
        console.info("Extending override config for: " + String(buttonNavigation.parent));
        buttonNavigation.overrideConfigOriginal = buttonNavigation.overrideConfig;

        for (const [key, value] of Object.entries(config)) {
            if (config[key]) {
                overrideConfig[key] = value;
            }
        }
    }

    function restoreOverrideConfig() {
        console.info("Restoring override config for: " + String(buttonNavigation.parent));
        buttonNavigation.overrideConfig = buttonNavigation.overrideConfigOriginal;
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
                console.debug('Executing override pressed for: ' + key);
                return;
            } else if (defaultConfig[key] && defaultConfig[key].pressed) {
                defaultConfig[key].pressed();
                console.debug('Executing pressed for: ' + key);
                return;
            } else {
                return;
            }
        case ButtonNavigation.ConfigType.PressedRepeat:
            if (overrideConfig[key] && overrideConfig[key].pressed_repeat) {
                overrideConfig[key].pressed_repeat();
                console.debug('Executing override pressed repeat for: ' + key);
                return
            } else if (defaultConfig[key] && defaultConfig[key].pressed_repeat) {
                defaultConfig[key].pressed_repeat();
                console.debug('Executing pressed repeat for: ' + key);
                return;
            } else {
                return;
            }
        case ButtonNavigation.ConfigType.Released:
            if (overrideConfig[key] && overrideConfig[key].released) {
                overrideConfig[key].released();
                console.debug('Executing override released for: ' + key);
                return;
            } else if (defaultConfig[key] && defaultConfig[key].released) {
                defaultConfig[key].released();
                console.debug('Executing released for: ' + key);
                return;
            } else {
                return;
            }
        case ButtonNavigation.ConfigType.LongPress:
            if (overrideConfig[key] && overrideConfig[key].long_press) {
                overrideConfig[key].long_press();
                console.debug('Executing override longpress for: ' + key);
                return;
            } else if (defaultConfig[key] && defaultConfig[key].long_press) {
                defaultConfig[key].long_press();
                console.debug('Executing longpress for: ' + key);
                return;
            } else {
                return;
            }
        }
    }

    Connections {
        target: ui.inputController
        enabled: ui.inputController.activeObject === String(buttonNavigation.parent) || buttonNavigation.overrideActive

        function onKeyPressed(key) {
            console.debug("Key press event: " + key + " " + buttonNavigation.parent);

            if (hasConfig(key, ButtonNavigation.ConfigType.LongPress) === true && (buttonNavigation.repeats[key] === false || !buttonNavigation.repeats[key])) {
                // add timer to execute long press
                if (!buttonNavigation.timers[key] && !buttonNavigation.longPressExecuted[key]) {
                    buttonNavigation.timers[key] = longPressTimer.createObject(buttonNavigation, {
                                                                                   action: function() {
                                                                                       executeCommand(key, ButtonNavigation.ConfigType.LongPress);
                                                                                       // we might execute the command earlier than the user releases the key,
                                                                                       // so we delete the timer here to prevent short press action at release
                                                                                       delete buttonNavigation.timers[key];
                                                                                       buttonNavigation.longPressExecuted[key] = true;
                                                                                   }
                                                                               });
                    console.debug('Adding timer for long press for key: ' + key);
                }
            } else if (hasConfig(key, ButtonNavigation.ConfigType.LongPress) === false && (buttonNavigation.repeats[key] === false || !buttonNavigation.repeats[key])) {
                buttonNavigation.repeats[key] = true;
                console.debug('Repat set true for:' + key);
                executeCommand(key, ButtonNavigation.ConfigType.Pressed);
            } else if (hasConfig(key, ButtonNavigation.ConfigType.LongPress) === false && buttonNavigation.repeats[key] === true) {
                if (hasConfig(key, ButtonNavigation.ConfigType.PressedRepeat)) {
                    executeCommand(key, ButtonNavigation.ConfigType.PressedRepeat);
                } else {
                    executeCommand(key, ButtonNavigation.ConfigType.Pressed);
                }
            }
        }

        function onKeyReleased(key) {
            console.debug("Key release event: " + key + " " + buttonNavigation.parent);

            buttonNavigation.repeats[key] = false;
            console.debug('Repat set false for: ' + key);

            if (timers[key]) {
                // cancel timer
                buttonNavigation.timers[key].stop();
                console.debug('Long press timer stopped for key: ' + key);

                delete buttonNavigation.timers[key];
                console.debug('Long press timer removed for: ' + key);

                executeCommand(key, ButtonNavigation.ConfigType.Pressed);
                return;
            }

            if (buttonNavigation.longPressExecuted[key]) {
                delete buttonNavigation.longPressExecuted[key];
            }

            if (hasConfig(key, ButtonNavigation.ConfigType.Released)) {
                executeCommand(key, ButtonNavigation.ConfigType.Released);
            }
        }
    }

    Component {
        id: longPressTimer
        Timer {
            property var action
            running: true
            interval: 800
            repeat: false
            onTriggered: {
                console.debug("Triggering long press action")
                if (action) {
                    action();
                }
            }
            Component.onCompleted: console.debug("longPressTimer created: " + this);
            Component.onDestruction: console.debug("longPressTimer destroyed: " + this);
        }
    }
}
