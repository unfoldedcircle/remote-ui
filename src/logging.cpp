// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging.h"

/// Main modules
Q_LOGGING_CATEGORY(lcApp, "uc.app");
Q_LOGGING_CATEGORY(lcI18n, "uc.app.i18n");
Q_LOGGING_CATEGORY(lcCore, "uc.core");
Q_LOGGING_CATEGORY(lcConfig, "uc.config");
Q_LOGGING_CATEGORY(lcVoice, "uc.voice");
Q_LOGGING_CATEGORY(lcSoftwareUpdate, "uc.softwareupdate");

/// UI
Q_LOGGING_CATEGORY(lcUi, "uc.ui");
Q_LOGGING_CATEGORY(lcResources, "uc.ui.resources");
Q_LOGGING_CATEGORY(lcPage, "uc.ui.page");
Q_LOGGING_CATEGORY(lcGroup, "uc.ui.group");
Q_LOGGING_CATEGORY(lcInput, "uc.ui.input");
Q_LOGGING_CATEGORY(lcNotification, "uc.ui.notification");
Q_LOGGING_CATEGORY(lcOnboarding, "uc.ui.onboarding");

Q_LOGGING_CATEGORY(lcEntityController, "uc.ui.entity.controller");
Q_LOGGING_CATEGORY(lcEntities, "uc.ui.entities");
Q_LOGGING_CATEGORY(lcGroupController, "uc.ui.group.controller");
Q_LOGGING_CATEGORY(lcIntegrationController, "uc.integration.controller");
Q_LOGGING_CATEGORY(lcIntegrationDriver, "uc.integration.driver");

Q_LOGGING_CATEGORY(lcDockController, "uc.dock.controller");

/// HW
Q_LOGGING_CATEGORY(lcHw, "uc.hw");
Q_LOGGING_CATEGORY(lcHwHaptic, "uc.hw.haptic");
Q_LOGGING_CATEGORY(lcHwBattery, "uc.hw.battery");
Q_LOGGING_CATEGORY(lcHwMic, "uc.hw.mic");
Q_LOGGING_CATEGORY(lcHwWifi, "uc.hw.wifi");
Q_LOGGING_CATEGORY(lcHwTouchSlider, "uc.hw.touchslider");

/// Entities
Q_LOGGING_CATEGORY(lcEntity, "uc.entity");
Q_LOGGING_CATEGORY(lcLight, "uc.entity.light");
Q_LOGGING_CATEGORY(lcButton, "uc.entity.button");
Q_LOGGING_CATEGORY(lcSwitch, "uc.entity.switch");
Q_LOGGING_CATEGORY(lcClimate, "uc.entity.climate");
Q_LOGGING_CATEGORY(lcCover, "uc.entity.cover");
Q_LOGGING_CATEGORY(lcSensor, "uc.entity.sensor");
Q_LOGGING_CATEGORY(lcMediaPlayer, "uc.entity.mediaplayer");
Q_LOGGING_CATEGORY(lcActivity, "uc.entity.activity");
Q_LOGGING_CATEGORY(lcMacro, "uc.entity.macro");
Q_LOGGING_CATEGORY(lcRemote, "uc.entity.remote");
