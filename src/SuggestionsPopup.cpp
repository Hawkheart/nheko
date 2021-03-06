#include "Avatar.h"
#include "AvatarProvider.h"
#include "Cache.h"
#include "ChatPage.h"
#include "Config.h"
#include "DropShadow.h"
#include "SuggestionsPopup.hpp"
#include "Utils.h"

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>

constexpr int PopupHMargin    = 5;
constexpr int PopupItemMargin = 4;

PopupItem::PopupItem(QWidget *parent, const QString &user_id)
  : QWidget(parent)
  , avatar_{new Avatar(this)}
  , user_id_{user_id}
  , hovering_{false}
{
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover);

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setContentsMargins(
          PopupHMargin, PopupItemMargin, PopupHMargin, PopupItemMargin);

        QFont font;
        font.setPixelSize(conf::popup::font);

        auto displayName = Cache::displayName(ChatPage::instance()->currentRoom(), user_id);

        avatar_->setSize(conf::popup::avatar);
        avatar_->setLetter(utils::firstChar(displayName));

        // If it's a matrix id we use the second letter.
        if (displayName.size() > 1 && displayName.at(0) == '@')
                avatar_->setLetter(QChar(displayName.at(1)));

        userName_ = new QLabel(displayName, this);
        userName_->setFont(font);

        topLayout_->addWidget(avatar_);
        topLayout_->addWidget(userName_, 1);

        AvatarProvider::resolve(ChatPage::instance()->currentRoom(),
                                user_id,
                                this,
                                [this](const QImage &img) { avatar_->setImage(img); });
}

void
PopupItem::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        if (underMouse() || hovering_)
                p.fillRect(rect(), hoverColor_);
}

void
PopupItem::mousePressEvent(QMouseEvent *event)
{
        if (event->buttons() != Qt::RightButton)
                emit clicked(Cache::displayName(ChatPage::instance()->currentRoom(), user_id_));

        QWidget::mousePressEvent(event);
}

SuggestionsPopup::SuggestionsPopup(QWidget *parent)
  : QWidget(parent)
{
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setWindowFlags(Qt::ToolTip | Qt::NoDropShadowWindowHint);

        layout_ = new QVBoxLayout(this);
        layout_->setMargin(0);
        layout_->setSpacing(0);
}

void
SuggestionsPopup::addUsers(const QVector<SearchResult> &users)
{
        // Remove all items from the layout.
        QLayoutItem *item;
        while ((item = layout_->takeAt(0)) != 0) {
                delete item->widget();
                delete item;
        }

        if (users.isEmpty()) {
                hide();
                return;
        }

        for (const auto &u : users) {
                auto user = new PopupItem(this, u.user_id);
                layout_->addWidget(user);
                connect(user, &PopupItem::clicked, this, &SuggestionsPopup::itemSelected);
        }

        resetSelection();
        adjustSize();

        resize(geometry().width(), 40 * users.size());
}

void
SuggestionsPopup::hoverSelection()
{
        resetHovering();
        setHovering(selectedItem_);
        update();
}

void
SuggestionsPopup::selectNextSuggestion()
{
        selectedItem_++;
        if (selectedItem_ >= layout_->count())
                selectFirstItem();

        hoverSelection();
}

void
SuggestionsPopup::selectPreviousSuggestion()
{
        selectedItem_--;
        if (selectedItem_ < 0)
                selectLastItem();

        hoverSelection();
}

void
SuggestionsPopup::resetHovering()
{
        for (int i = 0; i < layout_->count(); ++i) {
                const auto item = qobject_cast<PopupItem *>(layout_->itemAt(i)->widget());

                if (item)
                        item->setHovering(false);
        }
}

void
SuggestionsPopup::setHovering(int pos)
{
        const auto &item   = layout_->itemAt(pos);
        const auto &widget = qobject_cast<PopupItem *>(item->widget());

        if (widget)
                widget->setHovering(true);
}

void
SuggestionsPopup::selectHoveredSuggestion()
{
        const auto item = layout_->itemAt(selectedItem_);
        if (!item)
                return;

        const auto &widget = qobject_cast<PopupItem *>(item->widget());
        emit itemSelected(Cache::displayName(ChatPage::instance()->currentRoom(), widget->user()));

        resetSelection();
}

void
SuggestionsPopup::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
