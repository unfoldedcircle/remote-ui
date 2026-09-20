// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "ui/group/group.h"
#include "ui/notification.h"
#include "ui/page/page.h"
#include "ui/page/pages.h"
#include "ui/profile/profiles.h"

/**
 * The list models are looked up by key from QML and from core events, with ids that are not necessarily
 * in the model: a page that was deleted in the meantime, a profile of another user, a notification that
 * was already dismissed. A key that is not found has to be a no-op, not an access outside the model data.
 */
class testUiModels : public QObject {
    Q_OBJECT

 private slots:
    void pages_unknownKeyReturnsNull();
    void pages_rowOutOfRangeReturnsNull();
    void pages_removeUnknownKeyKeepsModel();
    void pages_updateUnknownKeyKeepsModel();
    void pages_removeKnownKeyRemovesRow();

    void profiles_unknownKeyReturnsNull();
    void profiles_profileIdOutOfRangeReturnsEmpty();
    void profiles_removeUnknownKeyKeepsModel();
    void profiles_updateUnknownKeyKeepsModel();

    void notifications_unknownKeyReturnsNull();
    void notifications_removeUnknownKeyKeepsModel();
    void notifications_removeKnownKeyRemovesRow();

    void pageItems_unknownKeyReturnsNull();
    void pageItems_removeUnknownKeyKeepsModel();

    void groupItems_unknownKeyReturnsNull();
    void groupItems_removeUnknownKeyKeepsModel();
};

void testUiModels::pages_unknownKeyReturnsNull() {
    uc::ui::Pages pages;
    pages.append(new uc::ui::Page("page-1", "Page 1", QString(), &pages));

    QVERIFY(pages.getPage(QStringLiteral("nope")) == nullptr);
    QVERIFY(pages.get(QStringLiteral("nope")) == nullptr);
    QVERIFY(pages.getPage(QStringLiteral("page-1")) != nullptr);
}

void testUiModels::pages_rowOutOfRangeReturnsNull() {
    uc::ui::Pages pages;
    pages.append(new uc::ui::Page("page-1", "Page 1", QString(), &pages));

    QVERIFY(pages.getPage(-1) == nullptr);
    QVERIFY(pages.getPage(1) == nullptr);
    QVERIFY(pages.getPage(0) != nullptr);
}

void testUiModels::pages_removeUnknownKeyKeepsModel() {
    uc::ui::Pages pages;
    pages.append(new uc::ui::Page("page-1", "Page 1", QString(), &pages));

    pages.removeItem(QStringLiteral("nope"));

    QCOMPARE(pages.count(), 1);
    QVERIFY(pages.getPage(QStringLiteral("page-1")) != nullptr);
}

void testUiModels::pages_updateUnknownKeyKeepsModel() {
    uc::ui::Pages pages;
    pages.append(new uc::ui::Page("page-1", "Page 1", QString(), &pages));

    pages.updatePageName(QStringLiteral("nope"), QStringLiteral("Renamed"));
    pages.updatePageImage(QStringLiteral("nope"), QStringLiteral("image"));

    QCOMPARE(pages.count(), 1);
    QCOMPARE(pages.getPage(QStringLiteral("page-1"))->pageName(), QStringLiteral("Page 1"));
}

void testUiModels::pages_removeKnownKeyRemovesRow() {
    uc::ui::Pages pages;
    pages.append(new uc::ui::Page("page-1", "Page 1", QString(), &pages));
    pages.append(new uc::ui::Page("page-2", "Page 2", QString(), &pages));

    pages.removeItem(QStringLiteral("page-1"));

    QCOMPARE(pages.count(), 1);
    QVERIFY(pages.getPage(QStringLiteral("page-1")) == nullptr);
    QVERIFY(pages.getPage(QStringLiteral("page-2")) != nullptr);
}

void testUiModels::profiles_unknownKeyReturnsNull() {
    uc::ui::Profiles profiles;
    profiles.append(new uc::ui::Profile("profile-1", "Profile 1", false, QString(), &profiles));

    QVERIFY(profiles.getProfile(QStringLiteral("nope")) == nullptr);
    QVERIFY(profiles.getProfile(-1) == nullptr);
    QVERIFY(profiles.getProfile(1) == nullptr);
    QVERIFY(profiles.getProfile(QStringLiteral("profile-1")) != nullptr);
}

void testUiModels::profiles_profileIdOutOfRangeReturnsEmpty() {
    uc::ui::Profiles profiles;
    profiles.append(new uc::ui::Profile("profile-1", "Profile 1", false, QString(), &profiles));

    QCOMPARE(profiles.getProfileId(0), QStringLiteral("profile-1"));
    QCOMPARE(profiles.getProfileId(-1), QString());
    QCOMPARE(profiles.getProfileId(1), QString());
}

void testUiModels::profiles_removeUnknownKeyKeepsModel() {
    uc::ui::Profiles profiles;
    profiles.append(new uc::ui::Profile("profile-1", "Profile 1", false, QString(), &profiles));

    profiles.removeItem(QStringLiteral("nope"));

    QCOMPARE(profiles.count(), 1);
    QVERIFY(profiles.getProfile(QStringLiteral("profile-1")) != nullptr);
}

void testUiModels::profiles_updateUnknownKeyKeepsModel() {
    uc::ui::Profiles profiles;
    profiles.append(new uc::ui::Profile("profile-1", "Profile 1", false, QString(), &profiles));

    profiles.updateProfileName(QStringLiteral("nope"), QStringLiteral("Renamed"));
    profiles.updateProfileIcon(QStringLiteral("nope"), QStringLiteral("uc:user"));
    profiles.updateProfileRestricted(QStringLiteral("nope"), true);

    QCOMPARE(profiles.count(), 1);
    QCOMPARE(profiles.getProfile(QStringLiteral("profile-1"))->getName(), QStringLiteral("Profile 1"));
    QCOMPARE(profiles.getProfile(QStringLiteral("profile-1"))->restricted(), false);
}

void testUiModels::notifications_unknownKeyReturnsNull() {
    uc::ui::NotificationsModel notifications;
    notifications.append(new uc::ui::NotificationItem("notification-1", QDateTime::currentDateTime(), "Title",
                                                      "Message", QString(), nullptr, QVariant(), QString(), false,
                                                      &notifications));

    QVERIFY(notifications.get(QStringLiteral("nope")) == nullptr);
    QVERIFY(notifications.get(-1) == nullptr);
    QVERIFY(notifications.get(1) == nullptr);
    QVERIFY(notifications.get(QStringLiteral("notification-1")) != nullptr);
}

void testUiModels::notifications_removeUnknownKeyKeepsModel() {
    uc::ui::NotificationsModel notifications;
    notifications.append(new uc::ui::NotificationItem("notification-1", QDateTime::currentDateTime(), "Title",
                                                      "Message", QString(), nullptr, QVariant(), QString(), false,
                                                      &notifications));

    notifications.removeItem(QStringLiteral("nope"));

    QCOMPARE(notifications.count(), 1);
}

void testUiModels::notifications_removeKnownKeyRemovesRow() {
    uc::ui::NotificationsModel notifications;
    notifications.append(new uc::ui::NotificationItem("notification-1", QDateTime::currentDateTime(), "Title",
                                                      "Message", QString(), nullptr, QVariant(), QString(), false,
                                                      &notifications));

    notifications.removeItem(QStringLiteral("notification-1"));

    QCOMPARE(notifications.count(), 0);
    QVERIFY(notifications.get(QStringLiteral("notification-1")) == nullptr);
}

void testUiModels::pageItems_unknownKeyReturnsNull() {
    uc::ui::PageItemList items;
    items.addItem(QStringLiteral("entity-1"), uc::ui::PageItem::Entity);

    QVERIFY(items.getPageItem(QStringLiteral("nope")) == nullptr);
    QVERIFY(items.getPageItem(-1) == nullptr);
    QVERIFY(items.getPageItem(1) == nullptr);
    QVERIFY(items.getPageItem(QStringLiteral("entity-1")) != nullptr);
}

void testUiModels::pageItems_removeUnknownKeyKeepsModel() {
    uc::ui::PageItemList items;
    items.addItem(QStringLiteral("entity-1"), uc::ui::PageItem::Entity);

    items.removeItem(QStringLiteral("nope"));

    QCOMPARE(items.count(), 1);
    QVERIFY(items.getPageItem(QStringLiteral("entity-1")) != nullptr);
}

void testUiModels::groupItems_unknownKeyReturnsNull() {
    uc::ui::GroupItemList items;
    items.addItem(QStringLiteral("entity-1"));

    QVERIFY(items.getGroupItem(QStringLiteral("nope")) == nullptr);
    QVERIFY(items.getGroupItem(-1) == nullptr);
    QVERIFY(items.getGroupItem(1) == nullptr);
    QVERIFY(items.getGroupItem(QStringLiteral("entity-1")) != nullptr);
}

void testUiModels::groupItems_removeUnknownKeyKeepsModel() {
    uc::ui::GroupItemList items;
    items.addItem(QStringLiteral("entity-1"));

    items.removeItem(QStringLiteral("nope"));

    QCOMPARE(items.count(), 1);
    QVERIFY(items.getGroupItem(QStringLiteral("entity-1")) != nullptr);
}

QTEST_GUILESS_MAIN(testUiModels)

#include "test_models.moc"
