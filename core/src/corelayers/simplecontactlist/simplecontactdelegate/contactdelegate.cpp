/****************************************************************************
**
** qutIM - instant messenger
**
** Copyright © 2011 Ruslan Nigmatullin <euroelessar@yandex.ru>
**
*****************************************************************************
**
** $QUTIM_BEGIN_LICENSE$
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see http://www.gnu.org/licenses/.
** $QUTIM_END_LICENSE$
**
****************************************************************************/
#include "contactdelegate.h"
#include <qutim/buddy.h>
#include <qutim/tooltip.h>
#include <QApplication>
#include <QHelpEvent>
#include <QTreeView>
#include <qutim/avatarfilter.h>
#include <QPainter>
#include <QStringBuilder>
#include <qutim/config.h>
#include <qutim/settingslayer.h>
#include "settings/simplecontactlistsettings.h"
#include <qutim/icon.h>
#include <qutim/debug.h>

namespace Core
{

struct ContactDelegatePrivate
{
	const QWidget *getWidget(const QStyleOptionViewItem &option)
	{
		if (const QStyleOptionViewItemV3 *v3 = qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option))
			return v3->widget;

		return 0;
	}
	QStyle *getStyle(const QStyleOptionViewItem& option)
	{
		if (const QStyleOptionViewItemV3 *v3 = qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option))
			return v3->widget ? v3->widget->style() : QApplication::style();

		return QApplication::style();
	}
	int verticalPadding;
	int horizontalPadding;
	ContactDelegate::ShowFlags showFlags;
	QHash<QString, bool> extInfo;
	int statusIconSize;
	int extIconSize;
	bool liteMode;
	QFont headerFont;
	QFont contactFont;
	QFont statusFont;
};

struct ContactInfoComparator
{
	ContactInfoComparator() : propertyName(QLatin1String("priorityInContactList"))
	{
	}

	bool operator()(const QVariantHash &a, const QVariantHash &b)
	{
		int p1 = a.value(propertyName).toInt();
		int p2 = b.value(propertyName).toInt();
		return p1 > p2;
	}

	const QString propertyName;
};

ContactDelegatePlugin::ContactDelegatePlugin()
{
}

void ContactDelegatePlugin::init()
{
	qutim_sdk_0_3::ExtensionIcon icon(QLatin1String(""));
	qutim_sdk_0_3::LocalizedString name = QT_TRANSLATE_NOOP("Plugin", "ContactDelegate");
	qutim_sdk_0_3::LocalizedString description = QT_TRANSLATE_NOOP("Plugin", "Just simple");
	setInfo(name, description, QUTIM_VERSION, icon);
	addExtension<Core::ContactDelegate>(name, description, icon);
	addExtension<Core::SimpleContactlistSettings, ContactListSettingsExtention>(name, description, icon);
}

bool ContactDelegatePlugin::load()
{
	return true;
}

bool ContactDelegatePlugin::unload()
{
	return false;
}

ContactDelegate::ContactDelegate(QObject *parent) :
	QAbstractItemDelegate(parent),p(new ContactDelegatePrivate)
{
	p->horizontalPadding = 5;
	p->verticalPadding = 3;
	reloadSettings();
	if (1) {} else Q_UNUSED(QT_TRANSLATE_NOOP("ContactList", "Default style"));
}

ContactDelegate::~ContactDelegate()
{
}

const QIcon extract_icon(const QVariant &data, int extIconSize, bool *ok)
{;
	QIcon icon;
	if (data.canConvert<ExtensionIcon>())
		icon = data.value<ExtensionIcon>().toIcon();
	else if (data.canConvert(QVariant::Icon))
		icon = data.value<QIcon>();

	*ok = !icon.pixmap(extIconSize).isNull();

	return icon;
}

void ContactDelegate::paint(QPainter *painter,
							const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// ajust contact font
	painter->setFont(p->contactFont);
	QStyleOptionViewItemV4 opt(option);
	painter->save();
	QStyle *style = p->getStyle(option);
	const QWidget *widget = opt.widget;

	ContactItemType type = static_cast<ContactItemType>(index.data(ItemTypeRole).toInt());

	QString name = index.data(Qt::DisplayRole).toString();

	QRect title_rect = option.rect;
	title_rect.adjust(p->horizontalPadding,
					  p->verticalPadding,
					  0,
					  -p->verticalPadding);

	switch (type) {
	case AccountType:
		// TODO: implement me
		// fall through for now
	case TagType: {
		if (!p->liteMode) {
			QStyleOptionButton buttonOption;
			buttonOption.state = option.state;
#ifdef Q_OS_MAC
			buttonOption.features = QStyleOptionButton::Flat;
			buttonOption.state |= QStyle::State_Raised;
			buttonOption.state &= ~QStyle::State_HasFocus;
#endif

			buttonOption.rect = option.rect;
			buttonOption.palette = option.palette;
			style->drawControl(QStyle::CE_PushButton, &buttonOption, painter, opt.widget);
		}

		if (const QTreeView *view = qobject_cast<const QTreeView*>(widget)) {
			QStyleOption branchOption;
			branchOption.rect = QRect(option.rect.left() + p->horizontalPadding,
									  option.rect.top(),
									  p->statusIconSize,
									  option.rect.height());
			branchOption.palette = option.palette;
			branchOption.state = QStyle::State_Children;
			title_rect.adjust(branchOption.rect.width() + p->horizontalPadding, 0, 0, 0);

			if (view->isExpanded(index))
				branchOption.state |= QStyle::State_Open;

			style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, painter, view);
		}
		// Ajust header font
		QFont font = opt.font;
		font = p->headerFont;
		painter->setFont(font);
		if (type == TagType) {
			QString count = index.data(ContactsCountRole).toString();
			QString online_count = index.data(OnlineContactsCountRole).toString();

			QString txt = name % QLatin1Literal(" (")
						  % online_count
						  % QLatin1Char('/')
						  % count
						  % QLatin1Char(')');

			txt = QFontMetrics(font).elidedText(txt, Qt::ElideMiddle, title_rect.width());
			painter->drawText(title_rect,
							  Qt::AlignVCenter,
							  txt
							  );
		} else {
			QString txt = QFontMetrics(font).elidedText(name, Qt::ElideMiddle, title_rect.width());
			// TODO: remove that when AccountType will be properly supported
			painter->drawText(title_rect,
							  Qt::AlignVCenter,
							  txt
							  );
		}
		break;
	}
	case ContactType: {
		style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
		QRect bounding;
		Status status = index.data(StatusRole).value<Status>();
		if (p->showFlags & ShowExtendedInfoIcons) {
			QList<QVariantHash> list = status.extendedInfos().values();
			ContactInfoComparator comparator;
			qSort(list.begin(), list.end(), comparator);

			const QString iconId = QStringLiteral("icon");
			const QString fallbackIconId = QStringLiteral("fallbackIcon");
			const QString showIcon = QStringLiteral("showIcon");
			const QString id = QStringLiteral("id");

			foreach (const QVariantHash &hash, list) {
				if (!p->extInfo.value(hash.value(id).toString(), true) || !hash.value(showIcon, true).toBool())
					continue;

				bool ok = false;
				QIcon icon = extract_icon(hash.value(iconId), p->extIconSize, &ok);
				if (!ok) {
					icon = extract_icon(hash.value(fallbackIconId), p->extIconSize, &ok);
					if (!ok)
						continue;
				}

				icon.paint(painter,
						   title_rect.right() - p->extIconSize,
						   option.rect.top() + p->verticalPadding,
						   p->extIconSize,
						   p->extIconSize,
						   Qt::AlignBottom | Qt::AlignRight);
				title_rect.adjust(0, 0, -p->extIconSize - p->horizontalPadding / 2, 0);
			}
		}
		title_rect.adjust(p->statusIconSize + p->horizontalPadding,
						  0,
						  0,
						  0);
		bool isStatusText = (p->showFlags & ShowStatusText) && !status.text().isEmpty();
		name = QFontMetrics(opt.font).elidedText(name,Qt::ElideRight,title_rect.width());
		painter->drawText(title_rect,
						  isStatusText ? Qt::AlignTop : Qt::AlignVCenter,
						  name,
						  &bounding
						  );

		if (isStatusText) {
			QRect status_rect = title_rect;
			status_rect.setTop(status_rect.top() + bounding.height());
			QFont font = opt.font;
			// ajust status font
			font = p->statusFont;
			QString text = status.text().remove(QLatin1Char('\n'));
			text = QFontMetrics(font).elidedText(text,Qt::ElideRight,status_rect.width());
			painter->setFont(font);
			painter->drawText(status_rect,
							  Qt::AlignTop,
							  text
							  );
		}

		QIcon itemIcon = index.data(Qt::DecorationRole).value<QIcon>();
		if (p->showFlags & ShowAvatars) {
			QString avatar = index.data(AvatarRole).toString();
			itemIcon = AvatarFilter::icon(avatar,itemIcon);
		}
		QPixmap pixmap = itemIcon.pixmap(p->statusIconSize, p->statusIconSize);
		painter->drawPixmap(option.rect.left() + p->horizontalPadding,
							option.rect.top() + p->verticalPadding,
							pixmap);
		break;
	}
	default:
		break;
	}
	painter->restore();
}

QSize ContactDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QVariant value = index.data(Qt::SizeHintRole);
	if (value.isValid())
		return value.toSize();

	const QWidget *widget = p->getWidget(option);
	QRect rect = widget->geometry();
	rect.adjust(2*p->horizontalPadding + p->statusIconSize,0,0,0);
	QFontMetrics metrics = option.fontMetrics;
	int height = metrics.boundingRect(rect, Qt::TextSingleLine,
									  index.data(Qt::DisplayRole).toString()).height();

	Status status = index.data(StatusRole).value<Status>();

	ContactItemType type = static_cast<ContactItemType>(index.data(ItemTypeRole).toInt());

	bool isContact = (type == ContactType);

	if (isContact && (p->showFlags & ShowStatusText) && !status.text().isEmpty()) {
		QFont desc_font = option.font;
		desc_font.setPointSize(desc_font.pointSize() -1);
		metrics = QFontMetrics(desc_font);
		height += metrics.boundingRect(rect,
									   Qt::TextSingleLine,
									   // There is no differ what text to use, but shotter is better
									   QLatin1String(".")
									   //											status.text().remove(QLatin1Char('\n'))
									   ).height();
	}
	if (isContact)
		height = qMax(p->statusIconSize,height);

	height += 2*p->verticalPadding;
	QSize size (option.rect.width(),height);
	return size;
}

bool ContactDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view,
								const QStyleOptionViewItem &option,
								const QModelIndex &index)
{
	if (event->type() == QEvent::ToolTip) {
#ifdef Q_WS_MAEMO_5
		return true;
#else
		Buddy *buddy = index.data(BuddyRole).value<Buddy*>();
		if (buddy)
			ToolTip::instance()->showText(event->globalPos(), buddy, view);
		return true;
#endif
	} else {
		return QAbstractItemDelegate::helpEvent(event, view, option, index);
	}
}

void ContactDelegate::drawFocus(QPainter *painter, const QStyleOptionViewItem &option,
								const QRect &rect) const
{
	Q_UNUSED(rect);
	QStyle *style = p->getStyle(option);
	const QWidget *widget = p->getWidget(option);
	style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, widget);
}

void ContactDelegate::setExtInfo(const QHash<QString, bool> &info)
{
	p->extInfo = info;
}

void ContactDelegate::setFlag(ShowFlags flag, bool on)
{
	if (on)
		p->showFlags |= flag;
	else
		p->showFlags &= ~flag;
}

void ContactDelegate::reloadSettings()
{
	qDebug() << "reload settings";
	Config cfg("appearance");
	cfg = cfg.group("contactList");
	p->liteMode = cfg.value("liteMode", true);

#ifdef QUTIM_MOBILE_UI
	p->statusIconSize = cfg.value("statusIconSize",
								  32);
#else
	p->statusIconSize = cfg.value("statusIconSize", 22);
#endif
	p->extIconSize = cfg.value("extIconSize",
							   qApp->style()->pixelMetric(QStyle::PM_SmallIconSize));
	setFlag(ShowStatusText, cfg.value("showStatusText", true));
	setFlag(ShowExtendedInfoIcons, cfg.value("showExtendedInfoIcons", true));
	setFlag(ShowAvatars, cfg.value("showAvatars", true));
	// Load extended statuses.
	QHash<QString, bool> statuses;
	QFont tempFont(qApp->font());
	p->contactFont.fromString(cfg.value("ContactFont", tempFont.toString()));
	tempFont.setPointSize(tempFont.pointSize() - 2);
	p->statusFont.fromString(cfg.value("StatusFont", tempFont.toString()));
	tempFont.setBold(true);
	tempFont.setPointSize(tempFont.pointSize() + 2);
	p->headerFont.fromString(cfg.value("HeaderFont", tempFont.toString()));
	cfg.beginGroup("extendedStatuses");
	foreach (const QString &name, cfg.childKeys())
		statuses.insert(name, cfg.value(name, true));
	cfg.endGroup();
	setExtInfo(statuses);
}

}

Q_DECLARE_METATYPE(QTreeView*)
QUTIM_EXPORT_PLUGIN(Core::ContactDelegatePlugin)

