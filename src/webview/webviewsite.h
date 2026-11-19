// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QUrl>

namespace uc {
namespace webview {

/**
 * What a web view applies for the page of a web view entity beyond its address, zoom and background colour: the
 * credentials, cookies and TLS exception for the origin of the start page, and the user agent (core-api
 * doc/entities/entity_webview.md, "Options"). The web view entity builds it from its options; QML hands it to the
 * WebView item as one value (`site: entityObj.site`).
 *
 * A value type without a library dependency: built into every binary, also with CONFIG+=no-webview, because the entity
 * layer builds it. The secrets (password, token, header value, cookie values) are no Q_PROPERTY: QML can pass the value
 * on but never read them, and nothing here writes them to a log.
 *
 * Origin: scheme, host and port of the start page (`url`). Credentials, cookies and the TLS exception are never applied
 * to another origin (sameOrigin()).
 */
class WebViewSite {
    Q_GADGET
    // the start page's origin, "scheme://host[:port]" (the port only when it is not the scheme's default)
    Q_PROPERTY(QString origin READ origin)

 public:
    enum class Auth { None, Basic, Bearer, Header };

    struct Cookie {
        QString   name;
        QString   value;   // secret
        QString   domain;  // empty: host-only cookie of the start page's host
        QString   path = QStringLiteral("/");
        bool      secure = false;
        bool      httpOnly = false;
        QDateTime expires;  // invalid: session cookie

        bool operator==(const Cookie &other) const;
        bool operator!=(const Cookie &other) const { return !(*this == other); }
    };

    WebViewSite() = default;

    QUrl    url() const { return m_url; }
    void    setUrl(const QUrl &url) { m_url = url; }
    QString origin() const { return originOf(m_url); }

    Auth    auth() const { return m_auth; }
    QString authUser() const { return m_user; }
    // Basic: the user name and password. Bearer: the token (user and headerName empty). Header: the header's name and
    // value (user empty).
    void setAuth(Auth auth, const QString &user, const QString &secret, const QString &headerName = QString());
    bool hasAuthSecret() const { return !m_secret.isEmpty(); }

    // The request header `bearer` and `header` add to every request to the origin: its name and value. Empty for the
    // other methods or without a secret.
    QString authHeaderName() const;
    QString authHeaderValue() const;

    QList<Cookie> cookies() const { return m_cookies; }
    void          setCookies(const QList<Cookie> &cookies) { m_cookies = cookies; }

    // the exception for an invalid certificate of the start page's host: the expected fingerprint (64 hex digits,
    // lower case, no separators), or any invalid certificate when the fingerprint is empty and allowInvalid is set
    QString tlsCertificateSha256() const { return m_tlsSha256; }
    bool    tlsAllowInvalidCertificate() const { return m_tlsAllowInvalid; }
    bool    hasTlsException() const { return !m_tlsSha256.isEmpty() || m_tlsAllowInvalid; }
    void    setTlsException(const QString &certificateSha256, bool allowInvalidCertificate);

    // "default" (the remote's own), "mobile", "desktop" or a custom user agent
    QString userAgent() const { return m_userAgent; }
    void    setUserAgent(const QString &userAgent);
    bool    hasCustomUserAgent() const { return m_userAgent != QLatin1String("default"); }

    /**
     * @brief The configured credentials answer a login request of the page (`basic`, core-api "Authentication"):
     * only from the start page's origin, never a proxy's, only for HTTP Basic and Digest, and not for a retry (the
     * server rejected them: the user gets the login dialog). Not preemptive: the first request goes without them.
     * @param origin the requesting origin, "scheme://host[:port]"
     * @param basicOrDigest the scheme is HTTP Basic or Digest
     * @return true with user and password set when the configured credentials answer the request
     */
    bool loginFor(const QString &origin, bool basicOrDigest, bool proxy, bool retry, QString *user,
                  QString *password) const;

    /**
     * @brief The options the site sets, by their Core-API names: "auth.bearer", "auth.header", "cookies", "tls",
     * "user_agent". Basic authentication is left out: the login answers it. For the log line of what a web view
     * library older than 0.8.0 cannot apply; the names carry no value.
     */
    QStringList requestOptions() const;

    // "scheme://host[:port]", the port only when it is not the scheme's default; empty for a URL without a host
    static QString originOf(const QUrl &url);
    // scheme, host (case-insensitive) and port (the scheme's default when absent) are equal
    static bool sameOrigin(const QUrl &a, const QUrl &b);
    // a SHA-256 fingerprint as the Core-API allows it (64 hex digits, optionally separated by ':', any case) in lower
    // case without separators; empty when it is not one
    static QString normalizedSha256(const QString &fingerprint);

    bool operator==(const WebViewSite &other) const;
    bool operator!=(const WebViewSite &other) const { return !(*this == other); }

 private:
    QUrl          m_url;
    Auth          m_auth = Auth::None;
    QString       m_user;
    QString       m_secret;
    QString       m_headerName;
    QList<Cookie> m_cookies;
    QString       m_tlsSha256;
    bool          m_tlsAllowInvalid = false;
    QString       m_userAgent = QStringLiteral("default");
};

}  // namespace webview
}  // namespace uc

Q_DECLARE_METATYPE(uc::webview::WebViewSite)
