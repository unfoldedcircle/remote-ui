// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Integration.Controller 1.0

import "qrc:/components" as Components

Item {
    id: integrationConfigureContainer

    signal cancelled
    signal home

    // error text of the current page: the driver rejected the provided input
    property string pageError: ""

    function cancelSetup() {
        loading.stop();

        // a session exists as soon as the setup has been started from the first page, whether or not a further
        // page has been shown: it has to be stopped, otherwise it lives on in the core (and is kept alive)
        if (IntegrationController.setupSessionActive) {
            IntegrationController.stopIntegrationSetup(IntegrationController.integrationDriverTosetup.id)
        }

        for (let i = configurationStepsSwipeView.count - 1; i >= 0; i--) {
            const page = configurationStepsSwipeView.takeItem(i);
            if (page) {
                page.destroy();
            }
        }

        IntegrationController.clearConfigPages();

        integrationConfigureContainer.cancelled();
    }

    /** KEYBOARD NAVIGATION **/
    // The form navigates through the QML focus chain: the fields of the current page (or its text,
    // for a user action page), then Next, LEFT to Cancel. The step owns the input while it is the
    // current step of the setup; taking it is deferred, as the step change is triggered by a key
    // press that is still being delivered.
    readonly property bool isCurrentStep: SwipeView.isCurrentItem
    onIsCurrentStepChanged: Qt.callLater(activate)

    function activate() {
        if (integrationConfigureContainer.isCurrentStep) {
            buttonNavigation.lastFocusItem = null;
            buttonNavigation.lastFocusAnchor = null;
            buttonNavigation.takeControl();
        } else {
            buttonNavigation.releaseControl();
        }
    }

    readonly property Item currentPage: configurationStepsSwipeView.currentItem

    // The focus stays on the previous page after a page change: it is still visible in the swipe
    // view and counts as a control of this scope. Move it to the first control of the new page.
    function focusPage() {
        if (!buttonNavigation.hasInputControl) {
            return;
        }

        buttonNavigation.lastFocusItem = null;
        buttonNavigation.lastFocusAnchor = null;
        const target = buttonNavigation.initialFocusItem;
        if (target) {
            target.forceActiveFocus();
        }
    }

    onCurrentPageChanged: Qt.callLater(focusPage)

    Components.ButtonNavigation {
        id: buttonNavigation
        manageFocus: true
        scrollTarget: integrationConfigureContainer.currentPage ? integrationConfigureContainer.currentPage.flickable : null
        initialFocusItem: integrationConfigureContainer.currentPage && integrationConfigureContainer.currentPage.firstFocusItem
                          ? integrationConfigureContainer.currentPage.firstFocusItem : buttonNext
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    integrationConfigureContainer.cancelSetup();
                }
            },
            "HOME": {
                "pressed": function() {
                    integrationConfigureContainer.cancelSetup();
                    integrationConfigureContainer.home();
                }
            }
        }
    }

    function processConfigPages() {
        let count = IntegrationController.configPages.length;

        console.debug("Config pages changed");
        console.debug("Config pages length: " + count);

        if (count === 0) {
            return;
        }

        let page = IntegrationController.configPages[count-1];
        console.debug(page);

        if (page.settings) {
            let component = Qt.createComponent("qrc:/components/integrations/Settings.qml");
            let obj = component.createObject(configurationStepsSwipeView, {
                                                 title: page.title,
                                                 settings: page.settings,
                                                 navExit: buttonNext
                                             });
        } else {
            let component = Qt.createComponent("qrc:/components/integrations/UserAction.qml");
            let obj = component.createObject(configurationStepsSwipeView, {
                                                 title: page.title,
                                                 message1: page.message1,
                                                 image: page.image,
                                                 message2:page.message2,
                                                 navExit: buttonNext
                                             });
        }
    }

    function goToStart() {
        integrationConfigureContainer.pageError = "";
        configurationStepsSwipeView.currentIndex = 0;
    }

    Component.onCompleted: integrationConfigureContainer.processConfigPages()

    Connections {
        target: IntegrationController
        ignoreUnknownSignals: true

        function onConfigPagesChanged() {
            integrationConfigureContainer.processConfigPages();
        }
    }

    Rectangle {
        id: integrationItemContainer

        width: parent.width - 40
        height: childrenRect.height
        color: colors.dark
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        radius: ui.cornerRadiusSmall
        border {
            color: colors.medium
            width: 1
        }

        Components.Icon {
            icon: "uc:globe"
            size: 30
            color: colors.light
            anchors { top: parent.top; topMargin: 5; right: parent.right; rightMargin: 5 }
            visible: IntegrationController.integrationDriverTosetup.external
        }

        RowLayout {
            width: parent.width - 60
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Rectangle {
                Layout.preferredWidth: 60
                Layout.preferredHeight: 60
                Layout.topMargin: 30
                Layout.bottomMargin: 30

                radius: 30
                color: colors.offwhite

                Components.Icon {
                    icon: IntegrationController.integrationDriverTosetup.icon
                    size: 60
                    color: colors.black
                    anchors.centerIn: parent
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 60

                Text {
                    Layout.fillWidth: true

                    color: colors.offwhite
                    text: IntegrationController.integrationDriverTosetup.name
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    font: fonts.primaryFont(30)
                }

                Text {
                    Layout.fillWidth: true

                    color: colors.light
                    //: Integration driver developer name
                    text: qsTr("By %1").arg(IntegrationController.integrationDriverTosetup.developerName)
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    font: fonts.secondaryFont(22)
                }
            }
        }
    }

    SwipeView {
        id: configurationStepsSwipeView

        interactive: false
        clip: true
        anchors { top: integrationItemContainer.bottom; bottom: setupInfo.top; bottomMargin: 10; left: parent.left; right: parent.right }
    }

    Column {
        id: setupInfo

        width: parent.width - 40
        anchors { bottom: footer.top; bottomMargin: 10; horizontalCenter: parent.horizontalCenter }
        spacing: 5

        // battery budget of the session: on battery the setup has to finish within the reported time.
        // A warning banner as in the web-configurator: orange, red once the time gets short
        Rectangle {
            id: setupLimitBanner

            // counted down locally between the updates of the core
            property int remaining: IntegrationController.setupExpiresInSec
            readonly property string formatted: Math.floor(remaining / 60) + ":" + ("0" + remaining % 60).slice(-2)
            readonly property bool urgent: IntegrationController.setupLimitLowBattery || remaining < 60

            width: parent.width
            height: setupLimitText.implicitHeight + 20
            radius: ui.cornerRadiusSmall
            visible: IntegrationController.setupLimitActive
            color: urgent ? colors.red : colors.orange

            Components.Icon {
                id: setupLimitIcon
                icon: "uc:triangle-exclamation"
                size: 48
                color: colors.offwhite
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
            }

            Text {
                id: setupLimitText

                anchors { left: setupLimitIcon.right; leftMargin: 10; right: parent.right; rightMargin: 15; verticalCenter: parent.verticalCenter }
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                color: colors.offwhite
                font: fonts.primaryFont(22)
                lineHeight: 0.9
                text: IntegrationController.setupLimitLowBattery
                      //: %1 is a countdown in minutes:seconds
                      ? qsTr("Low battery: the setup ends in %1").arg(setupLimitBanner.formatted)
                      //: %1 is a countdown in minutes:seconds
                      : qsTr("Running on battery: the setup ends in %1").arg(setupLimitBanner.formatted)
            }

            Connections {
                target: IntegrationController
                ignoreUnknownSignals: true

                function onSetupLimitChanged() {
                    setupLimitBanner.remaining = IntegrationController.setupExpiresInSec;
                }
            }

            Timer {
                interval: 1000
                repeat: true
                running: setupLimitBanner.visible && setupLimitBanner.remaining > 0
                onTriggered: setupLimitBanner.remaining--
            }
        }

        Text {
            width: parent.width
            visible: integrationConfigureContainer.pageError !== ""
            text: integrationConfigureContainer.pageError
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
            color: colors.red
            font: fonts.secondaryFont(20)
        }
    }

    Item {
        id: footer

        width: parent.width
        height: 80
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }

        Components.Button {
            id: buttonNext

            text: qsTr("Next")
            width: (parent.width - 20 ) / 2
            anchors { right: parent.right; bottom: parent.bottom }

            KeyNavigation.up: integrationConfigureContainer.currentPage ? integrationConfigureContainer.currentPage.lastFocusItem : null
            KeyNavigation.left: buttonCancel
            trigger: function() {
                // a driver without a setup page has no current page
                const page = configurationStepsSwipeView.currentItem;
                const setupData = page && typeof page.getData === "function" ? page.getData() : {};

                integrationConfigureContainer.pageError = "";

                if (configurationStepsSwipeView.currentIndex === 0) {
                    loading.start(true, 0, integrationConfigureContainer.cancelSetup);
                    IntegrationController.setupIntegration(IntegrationController.integrationDriverTosetup.id, setupData);
                } else {
                    if (page && page.settings) {
                        IntegrationController.integrationSetUserDataSettings(IntegrationController.integrationDriverTosetup.id, setupData);
                    } else {
                        IntegrationController.integrationSetUserDataConfirm(IntegrationController.integrationDriverTosetup.id);
                    }
                }
            }
        }

        Components.Button {
            id: buttonCancel

            text: qsTr("Cancel")
            width: (parent.width - 20 ) / 2
            color: colors.secondaryButton
            anchors { left: parent.left; bottom: parent.bottom }

            KeyNavigation.up: integrationConfigureContainer.currentPage ? integrationConfigureContainer.currentPage.lastFocusItem : null
            KeyNavigation.right: buttonNext
            trigger: function() {
                integrationConfigureContainer.cancelSetup();
            }
        }
    }

    Connections {
        target: IntegrationController
        ignoreUnknownSignals: true

        function onIntegrationSetupStopped() {
            loading.stop();
        }

        function onIntegrationSetupChange(driverId, state, error, requireUserAction) {
            switch (state) {
            case IntegrationControllerEnums.Setup:
                if (requireUserAction) {
                    configurationStepsSwipeView.incrementCurrentIndex();
                    loading.stop();
                } else {
                    // the driver is working: a step may take as long as the driver needs, so the
                    // wait stays cancellable
                    loading.start(true, 0, integrationConfigureContainer.cancelSetup);
                }
                integrationConfigureContainer.pageError = error;
                break;

            case IntegrationControllerEnums.Wait_user_action:
                // a new page, or the current page again with the reason why the input was rejected
                if (requireUserAction) {
                    configurationStepsSwipeView.incrementCurrentIndex();
                }
                integrationConfigureContainer.pageError = error;
                loading.stop();
                break;

            case IntegrationControllerEnums.Error:
                break;

            case IntegrationControllerEnums.Ok:
                break;
            }
        }
    }
}
