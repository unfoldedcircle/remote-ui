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

    void pages_swapData_viewAndDataAgree();
    void pageItems_swapData_viewAndDataAgree();
    void groupItems_swapData_viewAndDataAgree();

    void page_activities_showRunningEntitiesOfThePageAndItsGroups();
    void page_activities_followThePageItems();
    void page_activities_followTheGroupItems();
    void page_activities_keepTheirPosition();

 private:
    // the entity ids the activity bar of a page shows, in its order
    static QStringList activities(uc::ui::Page* page);

    // what a view makes of a rowsMoved signal: the row is taken out and inserted in front of the destination
    static void applyRowsMoved(QStringList* viewOrder, const QList<QVariant>& rowsMovedArguments);
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

void testUiModels::applyRowsMoved(QStringList* viewOrder, const QList<QVariant>& rowsMovedArguments) {
    const int start = rowsMovedArguments.at(1).toInt();
    const int end = rowsMovedArguments.at(2).toInt();
    const int destinationRow = rowsMovedArguments.at(4).toInt();
    QCOMPARE(start, end);

    const QString moved = viewOrder->at(start);
    viewOrder->insert(destinationRow, moved);
    viewOrder->removeAt(destinationRow > start ? start : start + 1);
}

/**
 * Reordering by drag and drop moves an item to the row it is dropped on. A view only learns about it through
 * rowsMoved, whose destination is the row the item is inserted in front of, which is not the index the item
 * ends up at when it moves down. Every move has to leave the view and the model data in the same order.
 */
void testUiModels::pages_swapData_viewAndDataAgree() {
    static const QStringList ids = {"a", "b", "c", "d", "e"};

    for (int from = 0; from < ids.size(); from++) {
        for (int to = 0; to < ids.size(); to++) {
            uc::ui::Pages pages;
            for (const QString& id : ids) {
                pages.append(new uc::ui::Page(id, id, QString(), &pages));
            }
            QSignalSpy rowsMoved(&pages, &QAbstractItemModel::rowsMoved);

            pages.swapData(from, to);

            QStringList expected = ids;
            expected.move(from, to);
            QStringList viewOrder = ids;
            if (from != to) {
                QCOMPARE(rowsMoved.count(), 1);
                applyRowsMoved(&viewOrder, rowsMoved.at(0));
            }

            QStringList dataOrder;
            for (int row = 0; row < pages.count(); row++) {
                dataOrder.append(pages.getPage(row)->pageId());
            }
            QCOMPARE(dataOrder, expected);
            QCOMPARE(viewOrder, expected);
        }
    }
}

void testUiModels::pageItems_swapData_viewAndDataAgree() {
    static const QStringList ids = {"a", "b", "c", "d", "e"};

    for (int from = 0; from < ids.size(); from++) {
        for (int to = 0; to < ids.size(); to++) {
            uc::ui::PageItemList items;
            for (const QString& id : ids) {
                items.addItem(id, uc::ui::PageItem::Entity);
            }
            QSignalSpy rowsMoved(&items, &QAbstractItemModel::rowsMoved);

            items.swapData(from, to);

            QStringList expected = ids;
            expected.move(from, to);
            QStringList viewOrder = ids;
            if (from != to) {
                QCOMPARE(rowsMoved.count(), 1);
                applyRowsMoved(&viewOrder, rowsMoved.at(0));
            }

            QStringList dataOrder;
            for (int row = 0; row < items.count(); row++) {
                dataOrder.append(items.getPageItem(row)->pageItemId());
            }
            QCOMPARE(dataOrder, expected);
            QCOMPARE(viewOrder, expected);
        }
    }
}

void testUiModels::groupItems_swapData_viewAndDataAgree() {
    static const QStringList ids = {"a", "b", "c", "d", "e"};

    for (int from = 0; from < ids.size(); from++) {
        for (int to = 0; to < ids.size(); to++) {
            uc::ui::GroupItemList items;
            for (const QString& id : ids) {
                items.addItem(id);
            }
            QSignalSpy rowsMoved(&items, &QAbstractItemModel::rowsMoved);

            items.swapData(from, to);

            QStringList expected = ids;
            expected.move(from, to);
            QStringList viewOrder = ids;
            if (from != to) {
                QCOMPARE(rowsMoved.count(), 1);
                applyRowsMoved(&viewOrder, rowsMoved.at(0));
            }

            QStringList dataOrder;
            for (int row = 0; row < items.count(); row++) {
                dataOrder.append(items.getGroupItem(row)->groupItemId());
            }
            QCOMPARE(dataOrder, expected);
            QCOMPARE(viewOrder, expected);
        }
    }
}

QStringList testUiModels::activities(uc::ui::Page* page) {
    QStringList ids;
    for (int i = 0; i < page->pageActivities()->count(); i++) {
        ids.append(page->pageActivities()->getPageItem(i)->pageItemId());
    }
    return ids;
}

void testUiModels::page_activities_showRunningEntitiesOfThePageAndItsGroups() {
    uc::ui::Page page("page-1", "Page 1");
    page.addEntity("activity.tv");
    page.addEntity("light.kitchen");
    page.addGroup("group.living");

    const QHash<QString, QStringList> groups = {{"group.living", {"media_player.speaker"}}};
    const auto                        groupHasEntity = [&groups](const QString& groupId, const QString& entityId) {
        return groups.value(groupId).contains(entityId);
    };

    // in the order they started: the activity of another page is not shown
    page.updateActivities({"media_player.speaker", "activity.other_page", "activity.tv"}, groupHasEntity);

    QCOMPARE(activities(&page), QStringList({"media_player.speaker", "activity.tv"}));

    page.updateActivities({}, groupHasEntity);

    QCOMPARE(activities(&page), QStringList());
}

void testUiModels::page_activities_followThePageItems() {
    uc::ui::Page page("page-1", "Page 1");
    page.addEntity("activity.tv");
    const auto        noGroups = [](const QString&, const QString&) { return false; };
    const QStringList running = {"activity.tv", "activity.music"};

    page.updateActivities(running, noGroups);
    QCOMPARE(activities(&page), QStringList({"activity.tv"}));

    // a page change event: the items are replaced while both activities keep running
    page.removeEntities();
    page.addEntity("activity.music");
    page.updateActivities(running, noGroups);

    QCOMPARE(activities(&page), QStringList({"activity.music"}));
}

void testUiModels::page_activities_followTheGroupItems() {
    uc::ui::Page page("page-1", "Page 1");
    page.addGroup("group.living");
    QHash<QString, QStringList> groups = {{"group.living", {"activity.tv"}}};
    const auto                  groupHasEntity = [&groups](const QString& groupId, const QString& entityId) {
        return groups.value(groupId).contains(entityId);
    };
    const QStringList running = {"activity.tv"};

    page.updateActivities(running, groupHasEntity);
    QCOMPARE(activities(&page), QStringList({"activity.tv"}));

    // the activity is taken out of the group while it runs
    groups["group.living"].clear();
    page.updateActivities(running, groupHasEntity);
    QCOMPARE(activities(&page), QStringList());

    // and put back
    groups["group.living"].append("activity.tv");
    page.updateActivities(running, groupHasEntity);
    QCOMPARE(activities(&page), QStringList({"activity.tv"}));
}

void testUiModels::page_activities_keepTheirPosition() {
    uc::ui::Page page("page-1", "Page 1");
    page.addEntity("activity.a");
    page.addEntity("activity.b");
    page.addEntity("activity.c");
    const auto noGroups = [](const QString&, const QString&) { return false; };

    page.updateActivities({"activity.b", "activity.a"}, noGroups);
    QCOMPARE(activities(&page), QStringList({"activity.b", "activity.a"}));

    // a newly started one is appended, the ones shown keep their place
    page.updateActivities({"activity.b", "activity.a", "activity.c"}, noGroups);
    QCOMPARE(activities(&page), QStringList({"activity.b", "activity.a", "activity.c"}));

    page.updateActivities({"activity.b", "activity.c"}, noGroups);
    QCOMPARE(activities(&page), QStringList({"activity.b", "activity.c"}));
}

QTEST_GUILESS_MAIN(testUiModels)

#include "test_models.moc"
