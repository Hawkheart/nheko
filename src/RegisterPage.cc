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

#include <QStyleOption>
#include <QTimer>

#include "Config.h"
#include "FlatButton.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "RaisedButton.h"
#include "RegisterPage.h"
#include "TextField.h"

#include "dialogs/ReCaptcha.hpp"

RegisterPage::RegisterPage(QSharedPointer<MatrixClient> client, QWidget *parent)
  : QWidget(parent)
  , client_(client)
{
        top_layout_ = new QVBoxLayout();

        back_layout_ = new QHBoxLayout();
        back_layout_->setSpacing(0);
        back_layout_->setContentsMargins(5, 5, -1, -1);

        back_button_ = new FlatButton(this);
        back_button_->setMinimumSize(QSize(30, 30));

        QIcon icon;
        icon.addFile(":/icons/icons/ui/angle-pointing-to-left.png");

        back_button_->setIcon(icon);
        back_button_->setIconSize(QSize(32, 32));

        back_layout_->addWidget(back_button_, 0, Qt::AlignLeft | Qt::AlignVCenter);
        back_layout_->addStretch(1);

        QIcon logo;
        logo.addFile(":/logos/register.png");

        logo_ = new QLabel(this);
        logo_->setPixmap(logo.pixmap(128));

        logo_layout_ = new QHBoxLayout();
        logo_layout_->setMargin(0);
        logo_layout_->addWidget(logo_, 0, Qt::AlignHCenter);

        form_wrapper_ = new QHBoxLayout();
        form_widget_  = new QWidget();
        form_widget_->setMinimumSize(QSize(350, 300));

        form_layout_ = new QVBoxLayout();
        form_layout_->setSpacing(20);
        form_layout_->setContentsMargins(0, 0, 0, 40);
        form_widget_->setLayout(form_layout_);

        form_wrapper_->addStretch(1);
        form_wrapper_->addWidget(form_widget_);
        form_wrapper_->addStretch(1);

        username_input_ = new TextField();
        username_input_->setLabel(tr("Username"));

        password_input_ = new TextField();
        password_input_->setLabel(tr("Password"));
        password_input_->setEchoMode(QLineEdit::Password);

        password_confirmation_ = new TextField();
        password_confirmation_->setLabel(tr("Password confirmation"));
        password_confirmation_->setEchoMode(QLineEdit::Password);

        server_input_ = new TextField();
        server_input_->setLabel(tr("Home Server"));

        form_layout_->addWidget(username_input_, Qt::AlignHCenter, 0);
        form_layout_->addWidget(password_input_, Qt::AlignHCenter, 0);
        form_layout_->addWidget(password_confirmation_, Qt::AlignHCenter, 0);
        form_layout_->addWidget(server_input_, Qt::AlignHCenter, 0);

        button_layout_ = new QHBoxLayout();
        button_layout_->setSpacing(0);
        button_layout_->setMargin(0);

        QFont font;
        font.setPixelSize(conf::fontSize);

        error_label_ = new QLabel(this);
        error_label_->setFont(font);

        register_button_ = new RaisedButton(tr("REGISTER"), this);
        register_button_->setMinimumSize(350, 65);
        register_button_->setFontSize(conf::btn::fontSize);
        register_button_->setCornerRadius(conf::btn::cornerRadius);

        button_layout_->addStretch(1);
        button_layout_->addWidget(register_button_);
        button_layout_->addStretch(1);

        top_layout_->addLayout(back_layout_);
        top_layout_->addLayout(logo_layout_);
        top_layout_->addLayout(form_wrapper_);
        top_layout_->addStretch(1);
        top_layout_->addLayout(button_layout_);
        top_layout_->addStretch(1);
        top_layout_->addWidget(error_label_, 0, Qt::AlignHCenter);

        connect(back_button_, SIGNAL(clicked()), this, SLOT(onBackButtonClicked()));
        connect(register_button_, SIGNAL(clicked()), this, SLOT(onRegisterButtonClicked()));

        connect(username_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
        connect(password_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
        connect(password_confirmation_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
        connect(server_input_, SIGNAL(returnPressed()), register_button_, SLOT(click()));
        connect(client_.data(),
                SIGNAL(registerError(const QString &)),
                this,
                SLOT(registerError(const QString &)));
        connect(client_.data(),
                &MatrixClient::registrationFlow,
                this,
                [this](const QString &user,
                       const QString &pass,
                       const QString &server,
                       const QString &session) {
                        emit errorOccurred();

                        if (!captchaDialog_) {
                                captchaDialog_ =
                                  std::make_shared<dialogs::ReCaptcha>(server, session, this);
                                connect(captchaDialog_.get(),
                                        &dialogs::ReCaptcha::closing,
                                        this,
                                        [this, user, pass, server, session]() {
                                                captchaDialog_->close();
                                                emit registering();
                                                client_->registerUser(user, pass, server, session);
                                        });
                        }

                        QTimer::singleShot(1000, this, [this]() { captchaDialog_->show(); });
                });

        setLayout(top_layout_);
}

void
RegisterPage::onBackButtonClicked()
{
        emit backButtonClicked();
}

void
RegisterPage::registerError(const QString &msg)
{
        emit errorOccurred();
        error_label_->setText(msg);
}

void
RegisterPage::onRegisterButtonClicked()
{
        error_label_->setText("");

        if (!username_input_->hasAcceptableInput()) {
                registerError(tr("Invalid username"));
        } else if (!password_input_->hasAcceptableInput()) {
                registerError(tr("Password is not long enough (min 8 chars)"));
        } else if (password_input_->text() != password_confirmation_->text()) {
                registerError(tr("Passwords don't match"));
        } else if (!server_input_->hasAcceptableInput()) {
                registerError(tr("Invalid server name"));
        } else {
                QString username = username_input_->text();
                QString password = password_input_->text();
                QString server   = server_input_->text();

                client_->registerUser(username, password, server);
                emit registering();
        }
}

void
RegisterPage::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
