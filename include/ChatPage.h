/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QFrame>
#include <QHBoxLayout>
#include <QMap>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "Cache.h"
#include "CommunitiesList.h"
#include "Community.h"

#include <mtx.hpp>

class MatrixClient;
class OverlayModal;
class QuickSwitcher;
class RoomList;
class SideBarActions;
class Splitter;
class TextInputWidget;
class TimelineViewManager;
class TopRoomBar;
class TypingDisplay;
class UserInfoWidget;
class UserSettings;

namespace dialogs {
class ReadReceipts;
}

constexpr int CONSENSUS_TIMEOUT      = 1000;
constexpr int SHOW_CONTENT_TIMEOUT   = 3000;
constexpr int TYPING_REFRESH_TIMEOUT = 10000;

Q_DECLARE_METATYPE(mtx::responses::Rooms)
Q_DECLARE_METATYPE(std::vector<std::string>)

class ChatPage : public QWidget
{
        Q_OBJECT

public:
        ChatPage(QSharedPointer<MatrixClient> client,
                 QSharedPointer<UserSettings> userSettings,
                 QWidget *parent = 0);

        // Initialize all the components of the UI.
        void bootstrap(QString userid, QString homeserver, QString token);
        void showQuickSwitcher();
        void showReadReceipts(const QString &event_id);
        QString currentRoom() const { return current_room_; }

        static ChatPage *instance() { return instance_; }
        void readEvent(const QString &room_id, const QString &event_id)
        {
                client_->readEvent(room_id, event_id);
        }
        void redactEvent(const QString &room_id, const QString &event_id)
        {
                client_->redactEvent(room_id, event_id);
        }

        QSharedPointer<UserSettings> userSettings() { return userSettings_; }

signals:
        void contentLoaded();
        void closing();
        void changeWindowTitle(const QString &msg);
        void unreadMessages(int count);
        void showNotification(const QString &msg);
        void showLoginPage(const QString &msg);
        void showUserSettingsPage();
        void showOverlayProgressBar();
        void startConsesusTimer();

        void initializeRoomList(QMap<QString, RoomInfo>);
        void initializeViews(const mtx::responses::Rooms &rooms);
        void initializeEmptyViews(const std::vector<std::string> &rooms);
        void syncUI(const mtx::responses::Rooms &rooms);
        void continueSync(const QString &next_batch);
        void syncRoomlist(const std::map<QString, RoomInfo> &updates);

private slots:
        void showUnreadMessageNotification(int count);
        void updateTopBarAvatar(const QString &roomid, const QPixmap &img);
        void updateOwnProfileInfo(const QUrl &avatar_url, const QString &display_name);
        void updateOwnCommunitiesInfo(const QList<QString> &own_communities);
        void initialSyncCompleted(const mtx::responses::Sync &response);
        void syncCompleted(const mtx::responses::Sync &response);
        void changeTopRoomInfo(const QString &room_id);
        void logout();
        void removeRoom(const QString &room_id);
        //! Handles initial sync failures.
        void retryInitialSync(int status_code = -1);

private:
        static ChatPage *instance_;

        using UserID      = QString;
        using Membership  = mtx::events::StateEvent<mtx::events::state::Member>;
        using Memberships = std::map<std::string, Membership>;

        using LeftRooms = std::map<std::string, mtx::responses::LeftRoom>;
        void removeLeftRooms(const LeftRooms &rooms);

        void updateTypingUsers(const QString &roomid, const std::vector<std::string> &user_ids);

        void loadStateFromCache();
        void deleteConfigs();
        void resetUI();
        //! Decides whether or not to hide the group's sidebar.
        void setGroupViewState(bool isEnabled);

        template<class Collection>
        Memberships getMemberships(const std::vector<Collection> &events) const;

        //! Update the room with the new notification count.
        void updateRoomNotificationCount(const QString &room_id, uint16_t notification_count);

        QStringList generateTypingUsers(const QString &room_id,
                                        const std::vector<std::string> &typing_users);

        QHBoxLayout *topLayout_;
        Splitter *splitter;

        QWidget *sideBar_;
        QWidget *communitiesSideBar_;
        QVBoxLayout *communitiesSideBarLayout_;
        QVBoxLayout *sideBarLayout_;
        QWidget *sideBarTopWidget_;
        QVBoxLayout *sideBarTopWidgetLayout_;

        QFrame *content_;
        QVBoxLayout *contentLayout_;

        CommunitiesList *communitiesList_;
        RoomList *room_list_;

        TimelineViewManager *view_manager_;
        SideBarActions *sidebarActions_;

        TopRoomBar *top_bar_;
        TextInputWidget *text_input_;
        TypingDisplay *typingDisplay_;

        // Safety net if consensus is not possible or too slow.
        QTimer *showContentTimer_;
        QTimer *consensusTimer_;
        QTimer *syncTimeoutTimer_;
        QTimer *initialSyncTimer_;

        QString current_room_;
        QString current_community_;

        UserInfoWidget *user_info_widget_;

        std::map<QString, QSharedPointer<Community>> communities_;

        // Keeps track of the users currently typing on each room.
        std::map<QString, QList<QString>> typingUsers_;
        QTimer *typingRefresher_;

        QSharedPointer<QuickSwitcher> quickSwitcher_;
        QSharedPointer<OverlayModal> quickSwitcherModal_;

        QSharedPointer<dialogs::ReadReceipts> receiptsDialog_;
        QSharedPointer<OverlayModal> receiptsModal_;

        // Matrix Client API provider.
        QSharedPointer<MatrixClient> client_;

        // Global user settings.
        QSharedPointer<UserSettings> userSettings_;

        // LMDB wrapper.
        QSharedPointer<Cache> cache_;
};

template<class Collection>
std::map<std::string, mtx::events::StateEvent<mtx::events::state::Member>>
ChatPage::getMemberships(const std::vector<Collection> &collection) const
{
        std::map<std::string, mtx::events::StateEvent<mtx::events::state::Member>> memberships;

        using Member = mtx::events::StateEvent<mtx::events::state::Member>;

        for (const auto &event : collection) {
                if (mpark::holds_alternative<Member>(event)) {
                        auto member = mpark::get<Member>(event);
                        memberships.emplace(member.state_key, member);
                }
        }

        return memberships;
}
