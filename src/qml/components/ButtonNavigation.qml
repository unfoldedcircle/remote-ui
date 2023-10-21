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

    function takeControl() {
        ui.inputController.takeControl(String(buttonNavigation.parent));
        console.debug("Button control enabled for: " + String(buttonNavigation.parent));
    }

    function releaseControl(newNavigation = "") {
        ui.inputController.releaseControl(newNavigation);
        console.debug("Button control disabled for: " + String(buttonNavigation.parent));
    }

    function extendDefaultConfig(config, overWrite = false) {
        console.debug("Extending default config for: " + String(buttonNavigation.parent));
        buttonNavigation.defaultConfigOriginal = buttonNavigation.defaultConfig;

        for (const [key, value] of Object.entries(config)) {
            if (defaultConfig[key] && overWrite) {
                defaultConfig[key] = value;
            } else if (!defaultConfig[key] && !overWrite) {
                defaultConfig[key] = value;
            }
        }
    }

    function restoreDefaultConfig() {
        console.debug("Restoring default config for: " + String(buttonNavigation.parent));
        buttonNavigation.defaultConfig = buttonNavigation.defaultConfigOriginal;
    }

    function extendOverrideConfig(config, overWrite = false) {
        console.debug("Extending override config for: " + String(buttonNavigation.parent));
        buttonNavigation.overrideConfigOriginal = buttonNavigation.overrideConfig;

        for (const [key, value] of Object.entries(config)) {
            if (overrideConfig[key] && overWrite) {
                overrideConfig[key] = value;
            } else if (!overrideConfig[key] && !overWrite) {
                overrideConfig[key] = value;
            }
        }
    }

    function restoreOverrideConfig() {
        console.debug("Restoring override config for: " + String(buttonNavigation.parent));
        buttonNavigation.overrideConfig = buttonNavigation.overrideConfigOriginal;
    }

    Connections {
        target: ui.inputController
        enabled: ui.inputController.activeObject === String(buttonNavigation.parent) || buttonNavigation.overrideActive

        function onKeyPressed(key) {
            if (overrideConfig[key]) {
                // execute override config
                if (overrideConfig[key].pressed) {
                    console.debug("OVERRIDE PRESSED: " + key + " " + buttonNavigation.parent);
                    overrideConfig[key].pressed();
                }

            } else if (defaultConfig[key]) {
                // execute default config
                if (defaultConfig[key].pressed) {
                    console.debug("DEFAULT PRESSED: " + key + " " + buttonNavigation.parent);
                    defaultConfig[key].pressed();
                }
            }
        }

        function onKeyLongPressed(key) {
            if (overrideConfig[key]) {
                // execute override config
                if (overrideConfig[key].long_press) {
                    console.debug("OVERRIDE LONG_PRESS: " + key + " " + buttonNavigation.parent);
                    overrideConfig[key].long_press();
                }

            } else if (defaultConfig[key]) {
                // execute default config
                if (defaultConfig[key].long_press) {
                    console.debug("DEFAULT LONG_PRESS: " + key + " " + buttonNavigation.parent);
                    defaultConfig[key].long_press();
                }
            }
        }

        function onKeyReleased(key) {
            if (overrideConfig[key]) {
                // execute override config
                if (overrideConfig[key].released) {
                    console.debug("OVERRIDE RELEASED: " + key + " " + buttonNavigation.parent);
                    overrideConfig[key].released();
                }

            } else if (defaultConfig[key]) {
                // execute default config
                if (defaultConfig[key].released) {
                    console.debug("DEFAULT RELEASED: " + key + " " + buttonNavigation.parent);
                    defaultConfig[key].released();
                }
            }
        }
    }
}
