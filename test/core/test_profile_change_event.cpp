// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonDocument>
#include <QtTest>

#include "core/core.h"

using uc::core::Api;
using uc::core::Profile;

/**
 * Parsing of the profile object of a `profile_change` event. The `NEW` and the `CHANGE` event carry the profile
 * data in the same `new_state.profile` object, only the profile identifier is a property of the event itself.
 * A restricted profile must stay restricted, no matter which of the two events announced it.
 */
class testProfileChangeEvent : public QObject {
    Q_OBJECT

 private slots:
    void parse_newEvent_restrictedProfile();
    void parse_changeEvent_restrictedProfile();
    void parse_changeEvent_unrestrictedProfile();
    void parse_restrictedOutsideProfileObjectIsIgnored();
    void parse_optionalFields();
    void parse_missingProfileObject();

 private:
    static QVariantMap fromJson(const char *json) { return QJsonDocument::fromJson(json).toVariant().toMap(); }
};

void testProfileChangeEvent::parse_newEvent_restrictedProfile() {
    Profile profile = Api::parseProfileChange(fromJson(R"({
        "event_type": "NEW", "profile_id": "guest",
        "new_state": {"profile": {"profile_id": "guest", "name": "Guest", "icon": "uc:ghost", "restricted": true}}
    })"));

    QCOMPARE(profile.id, QStringLiteral("guest"));
    QCOMPARE(profile.name, QStringLiteral("Guest"));
    QCOMPARE(profile.icon, QStringLiteral("uc:ghost"));
    QCOMPARE(profile.restricted, true);
}

void testProfileChangeEvent::parse_changeEvent_restrictedProfile() {
    Profile profile = Api::parseProfileChange(fromJson(R"({
        "event_type": "CHANGE", "profile_id": "guest",
        "new_state": {"profile": {"profile_id": "guest", "name": "Guest", "icon": "uc:ghost", "restricted": true}}
    })"));

    QCOMPARE(profile.id, QStringLiteral("guest"));
    QCOMPARE(profile.restricted, true);
}

void testProfileChangeEvent::parse_changeEvent_unrestrictedProfile() {
    Profile profile = Api::parseProfileChange(fromJson(R"({
        "event_type": "CHANGE", "profile_id": "default",
        "new_state": {"profile": {"profile_id": "default", "name": "Default", "restricted": false}}
    })"));

    QCOMPARE(profile.id, QStringLiteral("default"));
    QCOMPARE(profile.name, QStringLiteral("Default"));
    QCOMPARE(profile.icon, QString());
    QCOMPARE(profile.restricted, false);
}

/// The flag lives in the profile object: an event property of the same name is not the restriction.
void testProfileChangeEvent::parse_restrictedOutsideProfileObjectIsIgnored() {
    Profile profile = Api::parseProfileChange(fromJson(R"({
        "event_type": "NEW", "profile_id": "guest", "restricted": true,
        "new_state": {"restricted": true, "profile": {"profile_id": "guest", "name": "Guest"}}
    })"));

    QCOMPARE(profile.restricted, false);
}

void testProfileChangeEvent::parse_optionalFields() {
    Profile profile = Api::parseProfileChange(fromJson(R"({
        "event_type": "CHANGE", "profile_id": "default",
        "new_state": {"profile": {"profile_id": "default", "name": "Default", "restricted": false,
                                  "description": "the default profile", "pages": ["page1", "page2"]}}
    })"));

    QCOMPARE(profile.description, QStringLiteral("the default profile"));
    QCOMPARE(profile.pages, QStringList({QStringLiteral("page1"), QStringLiteral("page2")}));
}

/// The core sends a change event without profile data when only the pages of the profile changed.
void testProfileChangeEvent::parse_missingProfileObject() {
    Profile profile = Api::parseProfileChange(fromJson(R"({
        "event_type": "CHANGE", "profile_id": "default"
    })"));

    QCOMPARE(profile.id, QStringLiteral("default"));
    QCOMPARE(profile.name, QString());
    QCOMPARE(profile.restricted, false);
}

QTEST_GUILESS_MAIN(testProfileChangeEvent)

#include "test_profile_change_event.moc"
