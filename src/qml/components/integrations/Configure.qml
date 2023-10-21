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

    function cancelSetup() {
        loading.stop();

        if (configurationStepsSwipeView.currentIndex > 0) {
            IntegrationController.stopIntegrationSetup(IntegrationController.integrationDriverTosetup.id)
        }

        for (let i = configurationStepsSwipeView.children.length; i > 0; i--) {
            configurationStepsSwipeView.takeItem(i);
            console.debug("Destroyed child: " + i + configurationStepsSwipeView.children[i]);
        }

        IntegrationController.clearConfigPages();

        integrationConfigureContainer.cancelled();
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
                                                 settings: page.settings
                                             });
        } else {
            let component = Qt.createComponent("qrc:/components/integrations/UserAction.qml");
            let obj = component.createObject(configurationStepsSwipeView, {
                                                 title: page.title,
                                                 message1: page.message1,
                                                 image: page.image,
                                                 message2:page.message2
                                             });
        }
    }

    function goToStart() {
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
        anchors { top: integrationItemContainer.bottom; bottom: footer.top; bottomMargin: 20; left: parent.left; right: parent.right }
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
            trigger: function() {
                if (configurationStepsSwipeView.currentIndex === 0) {
                    loading.start();
                    let setupData = configurationStepsSwipeView.currentItem.getData();
                    IntegrationController.setupIntegration(IntegrationController.integrationDriverTosetup.id, setupData);
                } else {
                    if (configurationStepsSwipeView.currentItem.settings) {
                        let setupData = configurationStepsSwipeView.currentItem.getData();
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
                    loading.start();
                }
                break;

            case IntegrationControllerEnums.Wait_user_action:
                if (requireUserAction) {
                    configurationStepsSwipeView.incrementCurrentIndex();
                    loading.stop();
                }
                break;

            case IntegrationControllerEnums.Error:
                break;

            case IntegrationControllerEnums.Ok:
                break;
            }
        }
    }
}
