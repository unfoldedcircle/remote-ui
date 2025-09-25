// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace uc {
namespace core {

class MsgEvent {
    Q_GADGET

 public:
    enum Enum {
        auth_required,

        warning,
        entity_change,
        wifi_change,
        integration_driver_change,
        integration_change,
        integration_state,
        profile_change,
        configuration_change,

        ir_learning,
        dock_change,
        dock_state,
        dock_discovery,
        dock_setup_change,
        dock_update_change,

        integration_discovery,
        integration_setup_change,
        software_update,
        power_mode_change,
        battery_status,
    };
    Q_ENUM(Enum)

 private:
    MsgEvent() {}
};

class MsgEventTypes {
    Q_GADGET

 public:
    enum Enum {
        NEW,
        CHANGE,
        DELETE,
        START,
        DISCOVER,
        STOP,
        SETUP,
        PROGRESS,
    };
    Q_ENUM(Enum)

    enum WarningEvent {
        LOW_BATTERY,
        OPEN_CASE,
        BATTERY_UNDERVOLT,
    };
    Q_ENUM(WarningEvent)

 private:
    MsgEventTypes() {}
};

class RequestTypes {
    Q_GADGET

 public:
    enum Enum {
        // common
        auth,
        ping,

        // system commands
        version,
        system,
        system_cmd,
        get_factory_reset_token,
        factory_reset,
        get_api_access,
        set_api_access,
        check_system_update,
        update_system,
        get_system_update_progress,
        get_power_mode,
        set_power_mode,

        // entity handling
        get_entity_types,
        get_entity,
        get_entities,
        get_available_entities,
        get_entity_features,
        get_entity_commands,
        execute_entity_command,
        update_entity,
        delete_entity,
        delete_entities,

        // profile handling
        get_profiles,
        get_profile,
        get_active_profile,
        switch_profile,
        add_profile,
        update_profile,
        delete_profile,
        delete_all_profiles,

        // page handling
        get_pages,
        get_page,
        add_page,
        update_page,
        delete_page,
        delete_all_pages,

        // group handling
        get_groups,
        get_group,
        add_group,
        update_group,
        delete_group,
        delete_groups_in_profile,

        // integration handling
        get_integration_status,
        integration_cmd,
        integration_driver_cmd,

        get_integration_driver_count,
        get_integration_drivers,
        register_integration_driver,
        get_integration_driver,
        update_integration_driver,
        delete_integration_driver,

        get_integration_count,
        get_integrations,
        create_integration,
        get_integration,
        update_integration,
        delete_integration,

        configure_entities_from_integration,

        get_integration_discovery_status,  // TODO(marton): Implement me
        start_integration_discovery,
        stop_integration_discovery,
        get_discovered_integration_driver,  // TODO(marton): Implement me
        get_discovered_integration_driver_metadata,
        configure_discovered_integration_driver,

        get_integration_setup_processes,  // TODO(marton): Implement me
        setup_integration,
        stop_all_integration_setups,   // TODO(marton): Implement me
        get_integration_setup_status,  // TODO(marton): Implement me
        set_integration_user_data,
        stop_integration_setup,

        // configuration handling
        reset_configuration,
        get_configuration,
        get_button_cfg,
        set_button_cfg,
        get_device_cfg,
        set_device_cfg,
        get_display_cfg,
        set_display_cfg,
        get_haptic_cfg,
        set_haptic_cfg,
        get_localization_cfg,
        set_localization_cfg,
        get_timezone_names,
        get_localization_countries,
        get_localization_languages,
        get_network_cfg,
        set_network_cfg,
        get_software_update_cfg,
        set_software_update_cfg,
        get_power_saving_cfg,
        set_power_saving_cfg,
        get_profile_cfg,
        set_profile_cfg,
        get_sound_cfg,
        set_sound_cfg,
        get_voice_control_cfg,
        set_voice_control_cfg,
        get_voice_assistants,

        // wifi handling
        get_wifi_status,
        wifi_command,
        wifi_scan_start,
        wifi_scan_stop,
        get_wifi_scan_status,
        get_all_wifi_networks,
        add_wifi_network,
        del_all_wifi_networks,
        get_wifi_network,
        update_wifi_network,
        wifi_network_command,
        del_wifi_network,

        // dock handling
        get_dock_count,
        get_docks,
        create_dock,
        delete_all_docks,
        get_dock,
        update_dock,
        dock_connection_command,
        delete_dock,
        dock_command,
        get_dock_discovery_status,
        start_dock_discovery,
        stop_dock_discovery,
        get_dock_discovery_device,
        exec_cmd_on_discovered_dock,
        get_dock_setup_processes,
        create_dock_setup,
        stop_all_dock_setups,
        get_dock_setup_status,
        start_dock_setup,
        stop_dock_setup,
    };
    Q_ENUM(Enum)

 private:
    RequestTypes() {}
};

class MsgResponse {
    Q_GADGET

 public:
    enum Enum {
        // common
        authentication,
        pong,
        result,

        // system commands
        version_info,
        system_info,
        factory_reset_token,
        api_access,
        system_update_info,
        power_mode,

        // entity handling
        entity_types,
        entities,
        available_entities,
        entity_features,
        entity_commands,
        entity,

        // profile handling
        profile,
        profiles,

        // page handling
        page,
        pages,

        // group handling
        group,
        groups,

        // integration handling
        integration_status,
        integration_driver_count,
        integration_drivers,
        integration_driver,
        integration_count,
        integrations,
        integration,

        integration_discovery_status,   // TODO(marton): Implement me
        discovered_integration_driver,  // TODO(marton): Implement me
        integration_setup_processes,    // TODO(marton): Implement me
        integration_setup_info,

        // configuration handling
        configuration,
        button_cfg,
        display_cfg,
        device_cfg,
        haptic_cfg,
        localization_cfg,
        timezone_names,
        localization_countries,
        localization_languages,
        network_cfg,
        software_update_cfg,
        power_saving_cfg,
        profile_cfg,
        sound_cfg,
        voice_control_cfg,
        voice_assistants,

        // wifi handling
        wifi_status,
        wifi_scan_status,
        wifi_networks,
        wifi_network,

        // dock handling
        dock_count,
        docks,
        dock,
        dock_discovery_status,
        dock_discovery_device,
        dock_system_info,
        dock_setup_processes,
        dock_setup_status,
    };
    Q_ENUM(Enum)

 private:
    MsgResponse() {}
};

class SystemEnums {
    Q_GADGET

 public:
    enum Commands {
        STANDBY,
        REBOOT,
        POWER_OFF,
        RESTART,
    };
    Q_ENUM(Commands)

 private:
    SystemEnums() {}
};

class WifiEnums {
    Q_GADGET

 public:
    enum WpaState {
        UNKNOWN,
        ERROR,
        DISCONNECTED,
        INTERFACE_DISABLED,
        INACTIVE,
        SCANNING,
        AUTHENTICATED,
        ASSOCIATING,
        ASSOCIATED,
        FOUR_WAY_HANDSHAKE,
        GROUP_HANDSHAKE,
        COMPLETED,
    };
    Q_ENUM(WpaState)

    enum WifiCmd {
        DISCONNECT,
        RECONNECT,
        REASSOCIATE,
        ENABLE_ALL_NETWORKS,
        DISABLE_ALL_NETWORKS,
    };
    Q_ENUM(WifiCmd)

    enum WifiNetworkCmd {
        ENABLE,
        DISABLE,
        SELECT,
    };
    Q_ENUM(WifiNetworkCmd)

    enum NetworkState {
        CONNECTED,
        OUT_OF_RANGE,
        DISABLED,
        TEMPORARY_DISABLED,
    };
    Q_ENUM(NetworkState)

 private:
    WifiEnums() {}
};

class WifiEvent {
    Q_GADGET

 public:
    enum Enum {
        CONNECTED,
        DISCONNECTED,
        SCAN_STARTED,
        SCAN_COMPLETED,
        SCAN_FAILED,
        NETWORK_NOT_FOUND,
        WRONG_KEY,
        NETWORK_ADDED,
        NETWORK_REMOVED,
    };
    Q_ENUM(Enum);

 private:
    WifiEvent() {}
};

class DockEnums {
    Q_GADGET

 public:
    enum DockState {
        IDLE,
        CONNECTING,
        ACTIVE,
        RECONNECTING,
        ERROR,
    };
    Q_ENUM(DockState)

    enum DockCommands {
        SET_LED_BRIGHTNESS,
        IDENTIFY,
        REMOTE_LOW_BATTERY,
        REMOTE_CHARGED,
        REMOTE_NORMAL,
        REBOOT,
        RESET,
    };
    Q_ENUM(DockCommands)

 private:
    DockEnums() {}
};

class DockSetupEnums {
    Q_GADGET

 public:
    enum DockDiscoveryType {
        BT,
        NET,
    };
    Q_ENUM(DockDiscoveryType)

    enum DockSetupState {
        NEW,
        CONFIGURING,
        UPLOADING,
        RESTARTING,
        OK,
        ERROR,
    };
    Q_ENUM(DockSetupState)

    enum DockSetupError {
        NONE,
        NOT_FOUND,
        CONNECTION_ERROR,
        CONNECTION_REFUSED,
        AUTHORIZATION_ERROR,
        TIMEOUT,
        ABORT,
        PERSISTENCE_ERROR,
        OTHER,
    };
    Q_ENUM(DockSetupError)

    enum DockCommands {
        CONNECTION_TEST,
        IDENTIFY,
    };
    Q_ENUM(DockCommands)

 private:
    DockSetupEnums() {}
};

class IntegrationDriverEnums {
    Q_GADGET

 public:
    enum Commands {
        START,
        STOP,
    };
    Q_ENUM(Commands)

    enum States {
        NOT_CONFIGURED,
        IDLE,
        CONNECTING,
        ACTIVE,
        RECONNECTING,
        ERROR,
    };
    Q_ENUM(States)

 private:
    IntegrationDriverEnums() {}
};

class DeviceStates {
    Q_GADGET

 public:
    enum Enum {
        UNKNOWN,
        CONNECTING,
        CONNECTED,
        DISCONNECTED,
        ERROR,
    };
    Q_ENUM(Enum)

 private:
    DeviceStates() {}
};

class IntegrationEnums {
    Q_GADGET

 public:
    enum Commands {
        CONNECT,
        DISCONNECT,
    };
    Q_ENUM(Commands)

    enum SetupState {
        SETUP,
        WAIT_USER_ACTION,
        OK,
        ERROR,
    };
    Q_ENUM(SetupState)

    enum SetupError {
        NONE,
        NOT_FOUND,
        CONNECTION_REFUSED,
        AUTHORIZATION_ERROR,
        TIMEOUT,
        OTHER,
    };
    Q_ENUM(SetupError)

 private:
    IntegrationEnums() {}
};

class UpdateEnums {
    Q_GADGET

 public:
    enum UpdateChannel {
        DEFAULT,
        TESTING,
        DEVELOPMENT,
    };
    Q_ENUM(UpdateChannel)

    enum UpdateProgressType {
        IDLE,
        START,
        RUN,
        SUCCESS,
        FAILURE,
        DOWNLOAD,
        DONE,
        SUB_PROCESS,
        PROGRESS,
    };
    Q_ENUM(UpdateProgressType)

    enum DownloadState {
        PENDING,
        DOWNLOADING,
        DOWNLOADED,
        ERROR,
    };
    Q_ENUM(DownloadState)

 private:
    UpdateEnums() {}
};

class PowerEnums {
    Q_GADGET

 public:
    enum PowerMode {
        NORMAL,
        IDLE,
        LOW_POWER,
        SUSPEND,
    };
    Q_ENUM(PowerMode)

    enum PowerStatus {
        CHARGING,
        DISCHARGING,
        NOT_CHARGING,
        FULL,
    };
    Q_ENUM(PowerStatus)

 private:
    PowerEnums() {}
};

class AvailableEntityEnums {
    Q_GADGET

 public:
    enum Filter {
        NEW,
        CONFIGURED,
        ALL,
    };
    Q_ENUM(Filter)

 private:
    AvailableEntityEnums() {}
};

}  // namespace core
}  // namespace uc
