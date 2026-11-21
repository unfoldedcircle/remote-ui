// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webview.h"

#include <QRegularExpression>

#include "../../logging.h"
#include "../../util.h"
#include "webviewButtons.h"

namespace uc {
namespace ui {
namespace entity {

using uc::webview::WebViewSite;

Webview::Webview(const QString &id, QVariantMap nameI18n, const QString &language, const QString &icon,
                 const QString &area, bool enabled, QVariantMap attributes, QVariantMap options,
                 const QString &integrationId, QObject *parent)
    : Base(id, nameI18n, language, icon, area, Type::Webview, enabled, attributes, integrationId, false, parent) {
    qCDebug(lcWebViewEntity()) << "Web view entity constructor" << id;

    // no device classes: one screen for every web view
    m_deviceClass = QVariant::fromValue(WebviewDeviceClass::Webview).toString();

    applyOptions(options);

    if (attributes.size() > 0) {
        for (QVariantMap::iterator i = attributes.begin(); i != attributes.end(); i++) {
            updateAttribute(uc::Util::FirstToUpper(i.key()), i.value());
        }
    }
}

Webview::~Webview() {
    qCDebug(lcWebViewEntity()) << "Web view entity destructor" << m_id;
}

bool Webview::getShowNavigation() const {
    return m_hasUserShowNavigation ? m_userShowNavigation : m_showNavigation;
}

int Webview::getReloadInterval() const {
    return m_userReloadInterval >= 0 ? m_userReloadInterval : m_reloadInterval;
}

bool Webview::updateAttribute(const QString &attribute, QVariant data) {
    bool ok = false;

    switch (Util::convertStringToEnum<WebviewAttributes::Enum>(attribute)) {
        case WebviewAttributes::State: {
            int newState = Util::convertStringToEnum<WebviewStates::Enum>(uc::Util::FirstToUpper(data.toString()));

            if (m_state != newState && newState != -1) {
                m_state = newState;
                ok = true;
                emit stateChanged(m_id, m_state);

                m_stateAsString = WebviewStates::getTranslatedString(static_cast<WebviewStates::Enum>(m_state));
                emit stateAsStringChanged();
                emit stateInfoChanged();
            }
            break;
        }
        default:
            break;
    }

    return ok;
}

bool Webview::updateOptions(QVariant data) {
    applyOptions(data.toMap());
    return true;
}

QString Webview::getStateInfo() {
    // the tile shows the icon and the name of an available web view, the state only when it cannot be shown
    return m_state == WebviewStates::On ? QString() : m_stateAsString;
}

void Webview::onLanguageChangedTypeSpecific() {
    QTimer::singleShot(500, this, [=]() {
        m_stateAsString = WebviewStates::getTranslatedString(static_cast<WebviewStates::Enum>(m_state));
        emit stateAsStringChanged();
        emit stateInfoChanged();
    });
}

bool Webview::secretsInOptions(const QVariantMap &options) {
    const QVariantMap auth = options.value("auth").toMap();
    const char *const secrets[][2] = {
        {"has_password", "password"}, {"has_token", "token"}, {"has_header_value", "header_value"}};
    for (const auto &secret : secrets) {
        if (auth.value(secret[0]).toBool() && !auth.contains(secret[1])) {
            return false;
        }
    }
    const QVariantList cookies = options.value("cookies").toList();
    for (const QVariant &cookie : cookies) {
        const QVariantMap map = cookie.toMap();
        if (map.value("has_value").toBool() && !map.contains("value")) {
            return false;
        }
    }
    return true;
}

QUrl Webview::validUrl(const QString &url) {
    if (url.isEmpty() || url.size() > MAX_URL_LENGTH) {
        return QUrl();
    }
    const QUrl    parsed(url, QUrl::StrictMode);
    const QString scheme = parsed.scheme().toLower();
    if (!parsed.isValid() || (scheme != QLatin1String("http") && scheme != QLatin1String("https")) ||
        parsed.host().isEmpty() || !parsed.userInfo().isEmpty()) {
        return QUrl();
    }
    return parsed;
}

int Webview::validZoom(const QVariant &zoom) {
    bool      ok = false;
    const int value = zoom.toInt(&ok);
    return ok && value >= MIN_ZOOM && value <= MAX_ZOOM ? value : -1;
}

int Webview::validReloadInterval(const QVariant &seconds) {
    bool      ok = false;
    const int value = seconds.toInt(&ok);
    if (!ok) {
        return -1;
    }
    return value == 0 || (value >= MIN_RELOAD_INTERVAL && value <= MAX_RELOAD_INTERVAL) ? value : -1;
}

void Webview::applyOptions(const QVariantMap &options) {
    // the core delivers the complete options: a missing option has its default value
    m_editable = options.value("editable").toBool();

    const QString url = options.value("url").toString();
    m_url = validUrl(url);
    if (m_url.isEmpty() && !url.isEmpty()) {
        // the address may carry a token in its query: never logged
        qCWarning(lcWebViewEntity()) << "Ignoring the invalid start page address of" << m_id;
    }

    m_zoom = options.contains("zoom") ? validZoom(options.value("zoom")) : DEFAULT_ZOOM;
    if (m_zoom < 0) {
        qCWarning(lcWebViewEntity()) << "Ignoring the invalid zoom of" << m_id << options.value("zoom");
        m_zoom = DEFAULT_ZOOM;
    }

    static const QRegularExpression colorPattern(QStringLiteral("^#[0-9a-fA-F]{6}$"));
    const QString                   color = options.value("background_color").toString();
    m_backgroundColor = colorPattern.match(color).hasMatch() ? color : DEFAULT_BACKGROUND_COLOR;

    m_showNavigation = options.value("show_navigation").toBool();
    m_deviceApi = options.value("device_api").toBool();

    m_reloadInterval = options.contains("reload_interval") ? validReloadInterval(options.value("reload_interval")) : 0;
    if (m_reloadInterval < 0) {
        qCWarning(lcWebViewEntity()) << "Ignoring the invalid reload interval of" << m_id
                                     << options.value("reload_interval");
        m_reloadInterval = 0;
    }

    // user overrides of a web view from an integration driver; a null value removes one
    const QVariantMap user = options.value("user").toMap();
    m_userZoom = user.value("zoom").isNull() ? -1 : validZoom(user.value("zoom"));
    m_userReloadInterval =
        user.value("reload_interval").isNull() ? -1 : validReloadInterval(user.value("reload_interval"));
    m_hasUserShowNavigation = !user.value("show_navigation").isNull();
    m_userShowNavigation = user.value("show_navigation").toBool();
    m_hasUserDeviceApi = !user.value("device_api").isNull();
    m_userDeviceApi = user.value("device_api").toBool();

    // the buttons of the option, with the user's override of a single button on top; an invalid entry is left out and
    // the global setting applies to that button
    m_buttons = WebviewButtons::parse(options.value("buttons"));
    if (m_buttons.size() < options.value("buttons").toMap().size()) {
        qCDebug(lcWebViewEntity()) << "Ignoring unknown buttons or invalid button actions of" << m_id;
    }
    const QVariantMap userButtons = WebviewButtons::parse(user.value("buttons"));
    for (auto it = userButtons.constBegin(); it != userButtons.constEnd(); ++it) {
        m_buttons.insert(it.key(), it.value());
    }

    WebViewSite site;
    site.setUrl(m_url);

    const QVariantMap auth = options.value("auth").toMap();
    const QString     authType = auth.value("type", QStringLiteral("none")).toString();
    if (authType == QLatin1String("basic")) {
        site.setAuth(WebViewSite::Auth::Basic, auth.value("username").toString(), auth.value("password").toString());
    } else if (authType == QLatin1String("bearer")) {
        site.setAuth(WebViewSite::Auth::Bearer, QString(), auth.value("token").toString());
    } else if (authType == QLatin1String("header")) {
        site.setAuth(WebViewSite::Auth::Header, QString(), auth.value("header_value").toString(),
                     auth.value("header_name").toString());
    } else if (authType != QLatin1String("none")) {
        qCWarning(lcWebViewEntity()) << "Ignoring the unknown authentication method of" << m_id << authType;
    }

    QList<WebViewSite::Cookie> cookies;
    const QVariantList         cookieList = options.value("cookies").toList();
    for (const QVariant &item : cookieList) {
        const QVariantMap map = item.toMap();
        // a cookie without its value (a session without the secrets) cannot be set
        if (map.value("name").toString().isEmpty() || !map.contains("value")) {
            continue;
        }
        WebViewSite::Cookie cookie;
        cookie.name = map.value("name").toString();
        cookie.value = map.value("value").toString();
        cookie.domain = map.value("domain").toString();
        cookie.path = map.value("path", QStringLiteral("/")).toString();
        cookie.secure = map.value("secure", m_url.scheme() == QLatin1String("https")).toBool();
        cookie.httpOnly = map.value("http_only").toBool();
        if (map.contains("expires")) {
            cookie.expires = QDateTime::fromString(map.value("expires").toString(), Qt::ISODate);
        }
        cookies.append(cookie);
    }
    site.setCookies(cookies);

    const QVariantMap tls = options.value("tls").toMap();
    site.setTlsException(tls.value("certificate_sha256").toString(), tls.value("allow_invalid_certificate").toBool());
    site.setUserAgent(options.value("user_agent").toString());

    emit optionsChanged();

    if (site != m_site) {
        m_site = site;
        emit siteChanged();
    }
}

}  // namespace entity
}  // namespace ui
}  // namespace uc
