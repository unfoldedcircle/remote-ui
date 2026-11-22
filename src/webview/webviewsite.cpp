// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webviewsite.h"

#include <QRegularExpression>

namespace uc {
namespace webview {

bool WebViewSite::Cookie::operator==(const Cookie &other) const {
    return name == other.name && value == other.value && domain == other.domain && path == other.path &&
           secure == other.secure && httpOnly == other.httpOnly && expires == other.expires;
}

void WebViewSite::setAuth(Auth auth, const QString &user, const QString &secret, const QString &headerName) {
    m_auth = auth;
    m_user = auth == Auth::Basic ? user : QString();
    m_secret = auth == Auth::None ? QString() : secret;
    m_headerName = auth == Auth::Header ? headerName : QString();
}

QString WebViewSite::authHeaderName() const {
    if (m_secret.isEmpty()) {
        return QString();
    }
    switch (m_auth) {
        case Auth::Bearer:
            return QStringLiteral("Authorization");
        case Auth::Header:
            return m_headerName;
        default:
            return QString();
    }
}

QString WebViewSite::authHeaderValue() const {
    if (m_secret.isEmpty()) {
        return QString();
    }
    switch (m_auth) {
        case Auth::Bearer:
            return QStringLiteral("Bearer ") + m_secret;
        case Auth::Header:
            return m_headerName.isEmpty() ? QString() : m_secret;
        default:
            return QString();
    }
}

void WebViewSite::setTlsException(const QString &certificateSha256, bool allowInvalidCertificate) {
    m_tlsSha256 = normalizedSha256(certificateSha256);
    // the fingerprint wins: allow_invalid_certificate is only used without one (core-api "TLS certificates")
    m_tlsAllowInvalid = m_tlsSha256.isEmpty() && allowInvalidCertificate;
}

void WebViewSite::setUserAgent(const QString &userAgent) {
    m_userAgent = userAgent.trimmed().isEmpty() ? QStringLiteral("default") : userAgent;
}

bool WebViewSite::loginFor(const QString &origin, bool basicOrDigest, bool proxy, bool retry, QString *user,
                           QString *password) const {
    if (m_auth != Auth::Basic || m_user.isEmpty() || !basicOrDigest || proxy || retry) {
        return false;
    }
    if (!sameOrigin(QUrl(origin), m_url)) {
        return false;
    }
    if (user) {
        *user = m_user;
    }
    if (password) {
        *password = m_secret;
    }
    return true;
}

QStringList WebViewSite::requestOptions() const {
    QStringList options;
    if (!authHeaderName().isEmpty()) {
        options.append(m_auth == Auth::Bearer ? QStringLiteral("auth.bearer") : QStringLiteral("auth.header"));
    }
    if (!m_cookies.isEmpty()) {
        options.append(QStringLiteral("cookies"));
    }
    if (hasTlsException()) {
        options.append(QStringLiteral("tls"));
    }
    if (hasCustomUserAgent()) {
        options.append(QStringLiteral("user_agent"));
    }
    return options;
}

QString WebViewSite::originOf(const QUrl &url) {
    if (!url.isValid() || url.host().isEmpty()) {
        return QString();
    }
    const QString scheme = url.scheme().toLower();
    QString       origin = scheme + QStringLiteral("://") + url.host(QUrl::FullyEncoded).toLower();
    const int     defaultPort = scheme == QLatin1String("https") ? 443 : (scheme == QLatin1String("http") ? 80 : -1);
    if (url.port() != -1 && url.port() != defaultPort) {
        origin += QLatin1Char(':') + QString::number(url.port());
    }
    return origin;
}

bool WebViewSite::sameOrigin(const QUrl &a, const QUrl &b) {
    const QString origin = originOf(a);
    return !origin.isEmpty() && origin == originOf(b);
}

QString WebViewSite::normalizedSha256(const QString &fingerprint) {
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-fA-F]{2}(:?[0-9a-fA-F]{2}){31}$"));
    if (!pattern.match(fingerprint).hasMatch()) {
        return QString();
    }
    return QString(fingerprint).remove(QLatin1Char(':')).toLower();
}

bool WebViewSite::operator==(const WebViewSite &other) const {
    return m_url == other.m_url && m_auth == other.m_auth && m_user == other.m_user && m_secret == other.m_secret &&
           m_headerName == other.m_headerName && m_cookies == other.m_cookies && m_tlsSha256 == other.m_tlsSha256 &&
           m_tlsAllowInvalid == other.m_tlsAllowInvalid && m_userAgent == other.m_userAgent;
}

}  // namespace webview
}  // namespace uc
