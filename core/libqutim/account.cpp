/****************************************************************************
 *  account.cpp
 *
 *  Copyright (c) 2010 by Nigmatullin Ruslan <euroelessar@gmail.com>
 *
 ***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*****************************************************************************/

#include <QPointer>
#include <QStringBuilder>
#include "account_p.h"
#include "contact.h"
#include "debug.h"
#include "notification.h"
#include "groupchatmanager.h"

namespace qutim_sdk_0_3
{
AccountHook::AccountHook(AccountPrivate &p, Protocol *protocol)
    : MenuController(p, protocol)
{
}

const QMetaObject *AccountHook::metaObject() const
{
	return &Account::staticMetaObject;
}

void *AccountHook::qt_metacast(const char *name)
{
	Account * const that = static_cast<Account*>(this);
    if (!strcmp(name, qobject_interface_iid<GroupChatManager*>()))
        return static_cast<void*>(that->groupChatManager());
    if (!strcmp(name, qobject_interface_iid<ContactsFactory*>()))
        return static_cast<void*>(that->contactsFactory());
    if (!strcmp(name, qobject_interface_iid<InfoRequestFactory*>()))
        return static_cast<void*>(that->infoRequestFactory());
	return MenuController::qt_metacast(name);
}

int AccountHook::qt_metacall(QMetaObject::Call call, int id, void **data)
{
	return MenuController::qt_metacall(call, id, data);
}

Account::Account(const QString &id, Protocol *protocol)
    : AccountHook(*new AccountPrivate(this), protocol)
{
	Q_D(Account);
	d->protocol = protocol;
	d->id = id;
	d->groupChatManager = 0;
	d->contactsFactory = 0;
	d->infoRequestFactory = 0;
}

Account::Account(AccountPrivate &p, Protocol *protocol)
    : AccountHook(p, protocol)
{
	Q_D(Account);
	d->protocol = protocol;
	d->groupChatManager = 0;
	d->contactsFactory = 0;
	d->infoRequestFactory = 0;
}

Account::~Account()
{
	//		ConfigGroup data = config("properties");
	//		foreach(const QByteArray &name, dynamicPropertyNames())
	//			data.setValue(name, property(name));
	//		data.sync();
}

QString Account::id() const
{
	return d_func()->id;
}

QString Account::name() const
{
	return d_func()->id;
}

Config Account::config()
{
	Q_D(Account);
	QStringList paths;
	paths << d->protocol->id() % QLatin1Char('.') % d->id % QLatin1Literal("/account");
	paths << d->protocol->id();
	return Config(paths);
}

ConfigGroup Account::config(const QString &name)
{
	return config().group(name);
}

Status Account::status() const
{
	return d_func()->status;
}

Protocol *Account::protocol()
{
	return d_func()->protocol;
}

const Protocol *Account::protocol() const
{
	return d_func()->protocol;
}

void Account::setStatus(Status status)
{
	Q_D(Account);

	Status::ChangeReason reason = static_cast<Status::ChangeReason>(status.property("changeReason",static_cast<int>(Status::ByUser)));

	switch(reason) {
	case Status::ByUser:
	case Status::ByIdle:
	case Status::ByNetworkError:
	case Status::ByFatalError:
		break;
	case Status::ByAuthorizationFailed: {
		NotificationRequest request(Notification::System);
		request.setText(QT_TRANSLATE_NOOP("Account", "Authorization failed"));
		request.send();
		break;
	}
	}

	Status old = d->status;
	d->status = status;
	emit statusChanged(status,old);
}

ChatUnit *Account::getUnitForSession(ChatUnit *unit)
{
	return unit;
}

QVariantMap Account::parameters() const
{
	QVariantMap result;
	virtual_hook(ReadParametersHook, &result);
	return result;
}

QStringList Account::updateParameters(const QVariantMap &parameters)
{
	UpdateParametersArgument argument = { parameters, QStringList() };
	virtual_hook(UpdateParametersHook, &argument);
	return argument.reconnectionRequired;
}

AccountList Account::all()
{
	AccountList list;
	foreach(Protocol *proto, Protocol::all())
		foreach(Account *account, proto->accounts())
			list.append(account);
	return list;
}

GroupChatManager *Account::groupChatManager()
{
	return d_func()->groupChatManager;
}

ContactsFactory *Account::contactsFactory()
{
	return d_func()->contactsFactory;
}

InfoRequestFactory *Account::infoRequestFactory() const
{
	return d_func()->infoRequestFactory;
}

void Account::resetGroupChatManager(GroupChatManager *manager)
{
	Q_D(Account);
	if (manager == d->groupChatManager)
		return;
	if (manager && d->groupChatManager)
		warning() << "Account::resetGroupChatManager: the group chat manager is already set";
	Q_ASSERT((!manager || manager->account() == this) && "trying to set the group manager that was created for another account");
	if (manager)
		GroupChatManagersList::instance()->addManager(manager);
	else if (d->groupChatManager)
		GroupChatManagersList::instance()->removeManager(d->groupChatManager);
	d->groupChatManager = manager;
	emit groupChatManagerChanged(manager);
}

void Account::setContactsFactory(ContactsFactory *factory)
{
	d_func()->contactsFactory = factory;
}

void Account::setInfoRequestFactory(InfoRequestFactory *factory)
{
	d_func()->infoRequestFactory = factory;
}

}
