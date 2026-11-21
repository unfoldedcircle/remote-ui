// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @see https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_webview.md
 */

#include <QUrl>

#include "../../webview/webviewsite.h"
#include "entity.h"

namespace uc {
namespace ui {
namespace entity {

class WebviewAttributes : public QObject {
    Q_GADGET
 public:
    enum Enum { State };
    Q_ENUM(Enum)
};

class WebviewStates : public QObject {
    Q_OBJECT
 public:
    enum Enum { Unavailable = 0, Unknown, On };
    Q_ENUM(Enum)

    static QString getTranslatedString(Enum state) {
        switch (state) {
            case Enum::Unavailable:
                return QCoreApplication::translate("Web view state", "Unavailable");
            case Enum::Unknown:
                return QCoreApplication::translate("Web view state", "Unknown");
            case Enum::On:
                //: Web view state: the web page can be shown. Not the preposition "on".
                return QCoreApplication::translate("Web view state", "On");
            default:
                return Util::convertEnumToString<Enum>(state);
        }
    }
};

// the commands of a web view, without the "webview." prefix the core removes in its entity_command request
class WebviewCommands : public QObject {
    Q_GADGET
 public:
    enum Enum { Open, Reload, Clear_cache, Clear_data };
    Q_ENUM(Enum)
};

class WebviewDeviceClass : public QObject {
    Q_GADGET
 public:
    enum Enum { Webview };
    Q_ENUM(Enum)
};

/**
 * Web view entity: a web page the UI shows on a profile page tile (fullscreen when opened) or in the web view widget of
 * an activity page. The UI executes its commands (WebViewController); the integration driver, if any, only provides
 * the definition.
 *
 * The options are the core's, with the user overrides of zoom, reload interval, navigation, buttons and the device API
 * applied (`zoom`, `reloadInterval`, `showNavigation`, `buttons` and `deviceApi` are the effective values). Values the
 * core would refuse are tolerated (ADR 0018): an invalid address leaves `url` empty, an out-of-range zoom or reload
 * interval falls back to its default, an invalid button action is left out.
 *
 * Secrets: the UI's session receives the password, token, header value and cookie values in get_entity and
 * get_entities, entity_change events never carry them (core-api "Secret options"). They live in `site` only, which QML
 * hands to the WebView item without being able to read them; nothing here logs them. An options update without the
 * secrets (an event) is not applied here: EntityController fetches the entity again (secretsInOptions()).
 */
class Webview : public Base {
    Q_OBJECT

    // the start page, http or https; empty when the option is missing or invalid
    Q_PROPERTY(QUrl url READ getUrl NOTIFY optionsChanged)
    // the effective page zoom in percent (25..400): the user override, else the option, else 100
    Q_PROPERTY(int zoom READ getZoom NOTIFY optionsChanged)
    // "#RRGGBB": the option, else white, so that a page without a background of its own stays readable
    Q_PROPERTY(QString backgroundColor READ getBackgroundColor NOTIFY optionsChanged)
    // the effective navigation controls (back, forward, reload, close) of the fullscreen web view
    Q_PROPERTY(bool showNavigation READ getShowNavigation NOTIFY optionsChanged)
    // the effective reload interval in seconds while the page is shown, 0 = off
    Q_PROPERTY(int reloadInterval READ getReloadInterval NOTIFY optionsChanged)
    // the effective device state API for the pages (`device_api`, default false; docs/webview.md "Device state for web
    // pages"): the user override, else the option
    Q_PROPERTY(bool deviceApi READ getDeviceApi NOTIFY optionsChanged)
    // the buttons this web view changes in the fullscreen web view (core-api "Buttons"): button identifier -> action,
    // the user overrides over the option; a button that is not in it has the action of the global setting
    Q_PROPERTY(QVariantMap buttons READ getButtons NOTIFY optionsChanged)
    // user-defined (true) or from an integration driver (false)
    Q_PROPERTY(bool editable READ isEditable NOTIFY optionsChanged)
    // what the WebView item applies beyond address, zoom and background (credentials, cookies, TLS, user agent)
    Q_PROPERTY(uc::webview::WebViewSite site READ getSite NOTIFY siteChanged)

 public:
    static constexpr int MIN_ZOOM = 25;
    static constexpr int MAX_ZOOM = 400;
    static constexpr int DEFAULT_ZOOM = 100;
    static constexpr const char *DEFAULT_BACKGROUND_COLOR = "#FFFFFF";
    static constexpr int MIN_RELOAD_INTERVAL = 10;
    static constexpr int MAX_RELOAD_INTERVAL = 86400;
    static constexpr int MAX_URL_LENGTH = 2048;

    explicit Webview(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                     const QString &area, bool enabled, QVariantMap attributes, QVariantMap options,
                     const QString &integrationId, QObject *parent);
    ~Webview();

    QUrl                     getUrl() const { return m_url; }
    int                      getZoom() const { return m_userZoom > 0 ? m_userZoom : m_zoom; }
    QString                  getBackgroundColor() const { return m_backgroundColor; }
    bool                     getShowNavigation() const;
    int                      getReloadInterval() const;
    bool                     getDeviceApi() const { return m_hasUserDeviceApi ? m_userDeviceApi : m_deviceApi; }
    QVariantMap              getButtons() const { return m_buttons; }
    bool                     isEditable() const { return m_editable; }
    uc::webview::WebViewSite getSite() const { return m_site; }

    bool updateAttribute(const QString &attribute, QVariant data) override;
    bool updateOptions(QVariant data) override;

    QString getStateInfo() override;
    void    onLanguageChangedTypeSpecific() override;

    /**
     * @brief Whether options carry the secret values, or only the `has_*` flags of a session without them (every
     * entity_change event): true when no secret is flagged without its value.
     */
    static bool secretsInOptions(const QVariantMap &options);

    // an absolute http or https URL of at most MAX_URL_LENGTH characters, without user name and password; else empty
    static QUrl validUrl(const QString &url);
    // a zoom in percent within MIN_ZOOM..MAX_ZOOM, else -1
    static int validZoom(const QVariant &zoom);
    // 0 or MIN_RELOAD_INTERVAL..MAX_RELOAD_INTERVAL seconds, else -1
    static int validReloadInterval(const QVariant &seconds);

 signals:
    void optionsChanged();
    // credentials, cookies, TLS exception or user agent changed: an open page loads again to apply them
    void siteChanged();

 private:
    void applyOptions(const QVariantMap &options);

    QUrl    m_url;
    int     m_zoom = DEFAULT_ZOOM;
    QString m_backgroundColor;
    bool    m_showNavigation = false;
    int     m_reloadInterval = 0;
    bool    m_deviceApi = false;
    bool    m_editable = false;
    // the option's buttons with the user overrides applied, valid actions only
    QVariantMap m_buttons;

    // user overrides, -1 / false when not set
    int  m_userZoom = -1;
    int  m_userReloadInterval = -1;
    bool m_hasUserShowNavigation = false;
    bool m_userShowNavigation = false;
    bool m_hasUserDeviceApi = false;
    bool m_userDeviceApi = false;

    uc::webview::WebViewSite m_site;
};

}  // namespace entity
}  // namespace ui
}  // namespace uc
