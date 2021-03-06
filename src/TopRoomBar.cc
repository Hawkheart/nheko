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

#include <QDebug>
#include <QStyleOption>

#include "Avatar.h"
#include "Config.h"
#include "FlatButton.h"
#include "Label.h"
#include "MainWindow.h"
#include "Menu.h"
#include "OverlayModal.h"
#include "TopRoomBar.h"
#include "Utils.h"

TopRoomBar::TopRoomBar(QWidget *parent)
  : QWidget(parent)
  , buttonSize_{32}
{
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(65);

        topLayout_ = new QHBoxLayout();
        topLayout_->setSpacing(10);
        topLayout_->setMargin(10);

        avatar_ = new Avatar(this);
        avatar_->setLetter("");
        avatar_->setSize(35);

        textLayout_ = new QVBoxLayout();
        textLayout_->setSpacing(0);
        textLayout_->setContentsMargins(0, 0, 0, 0);

        QFont roomFont("Open Sans SemiBold");
        roomFont.setPixelSize(conf::topRoomBar::fonts::roomName);

        nameLabel_ = new QLabel(this);
        nameLabel_->setFont(roomFont);

        QFont descriptionFont("Open Sans");
        descriptionFont.setPixelSize(conf::topRoomBar::fonts::roomDescription);

        topicLabel_ = new Label(this);
        topicLabel_->setFont(descriptionFont);
        topicLabel_->setTextFormat(Qt::RichText);
        topicLabel_->setTextInteractionFlags(Qt::TextBrowserInteraction);
        topicLabel_->setOpenExternalLinks(true);
        connect(topicLabel_, &Label::clicked, [this](QMouseEvent *e) {
                if (e->button() == Qt::LeftButton && !topicLabel_->hasSelectedText())
                        topicLabel_->setWordWrap(!topicLabel_->wordWrap());
        });

        textLayout_->addWidget(nameLabel_);
        textLayout_->addWidget(topicLabel_);

        settingsBtn_ = new FlatButton(this);
        settingsBtn_->setFixedSize(buttonSize_, buttonSize_);
        settingsBtn_->setCornerRadius(buttonSize_ / 2);

        QIcon settings_icon;
        settings_icon.addFile(":/icons/icons/ui/vertical-ellipsis.png");
        settingsBtn_->setIcon(settings_icon);
        settingsBtn_->setIconSize(QSize(buttonSize_ / 2, buttonSize_ / 2));

        topLayout_->addWidget(avatar_);
        topLayout_->addLayout(textLayout_, 1);
        topLayout_->addWidget(settingsBtn_, 0, Qt::AlignRight);

        menu_ = new Menu(this);

        inviteUsers_ = new QAction(tr("Invite users"), this);
        connect(inviteUsers_, &QAction::triggered, this, [this]() {
                MainWindow::instance()->openInviteUsersDialog(
                  [this](const QStringList &invitees) { emit inviteUsers(invitees); });
        });

        leaveRoom_ = new QAction(tr("Leave room"), this);
        connect(leaveRoom_, &QAction::triggered, this, []() {
                MainWindow::instance()->openLeaveRoomDialog();
        });

        menu_->addAction(inviteUsers_);
        menu_->addAction(leaveRoom_);

        connect(settingsBtn_, &QPushButton::clicked, this, [this]() {
                auto pos = mapToGlobal(settingsBtn_->pos());
                menu_->popup(
                  QPoint(pos.x() + buttonSize_ - menu_->sizeHint().width(), pos.y() + buttonSize_));
        });

        setLayout(topLayout_);
}

void
TopRoomBar::updateRoomAvatarFromName(const QString &name)
{
        avatar_->setLetter(utils::firstChar(name));
        update();
}

void
TopRoomBar::reset()
{
        nameLabel_->setText("");
        topicLabel_->setText("");
        avatar_->setLetter("");

        roomName_.clear();
        roomTopic_.clear();
}

void
TopRoomBar::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QStyleOption option;
        option.initFrom(this);

        QPainter painter(this);
        style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

        painter.setPen(QPen(borderColor()));
        painter.drawLine(QPointF(0, height()), QPointF(width(), height()));

        // Number of pixels that we can move sidebar splitter per frame. If label contains
        // text which fills entire it's width then label starts blocking it's layout from
        // shrinking. Making label little bit shorter leaves some space for it to shrink.
        const auto perFrameResize = 20;

        QString elidedText =
          QFontMetrics(nameLabel_->font())
            .elidedText(roomName_, Qt::ElideRight, nameLabel_->width() - perFrameResize);
        nameLabel_->setText(elidedText);

        if (topicLabel_->wordWrap())
                elidedText = roomTopic_;
        else
                elidedText =
                  QFontMetrics(topicLabel_->font())
                    .elidedText(roomTopic_, Qt::ElideRight, topicLabel_->width() - perFrameResize);
        elidedText.replace(conf::strings::url_regex, conf::strings::url_html);
        topicLabel_->setText(elidedText);
}

void
TopRoomBar::mousePressEvent(QMouseEvent *event)
{
        if (childAt(event->pos()) == topicLabel_) {
                event->accept();
        }
}

void
TopRoomBar::updateRoomAvatar(const QImage &avatar_image)
{
        avatar_->setImage(avatar_image);
        update();
}

void
TopRoomBar::updateRoomAvatar(const QIcon &icon)
{
        avatar_->setIcon(icon);
        update();
}

void
TopRoomBar::updateRoomName(const QString &name)
{
        roomName_ = name;
        update();
}

void
TopRoomBar::updateRoomTopic(QString topic)
{
        roomTopic_ = topic;
        update();
}
