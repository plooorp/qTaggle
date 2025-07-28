#include "tagtablemodel.h"

#include "app/utils.h"

TagTableModel::TagTableModel(QObject* parent)
	: QAbstractTableModel(parent)
{}

QVariant TagTableModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (index.row() < 0 || index.row() >= m_tags.size())
		return QVariant();
	if (index.column() < 0 || index.column() >= columnCount(QModelIndex()))
		return QVariant();

	QSharedPointer<Tag> tag = m_tags.at(index.row());
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case Column::ID:
			return QString::number(tag->id());
		case Column::Name:
			return tag->name();
		case Column::Description:
			return tag->description();
		case Column::URLs:
			return tag->urls().join(' ');
		case Column::Degree:
			return friendlyNumber(tag->degree());
		case Column::Created:
			return QLocale().toString(tag->created(), QLocale::ShortFormat);
		case Column::Modified:
			return QLocale().toString(tag->modified(), QLocale::ShortFormat);
		}
	}
	if (role == Qt::ToolTipRole)
	{
		switch (index.column())
		{
		case Column::Name:
			return QVariant(tag->name());
		case Column::Description:
			return QVariant(tag->description());
		case Column::Degree:
			return QVariant(QLocale().toString(tag->degree()));
		case Column::Created:
			return QVariant(QLocale().toString(tag->created(), QLocale::LongFormat));
		case Column::Modified:
			return QVariant(QLocale().toString(tag->modified(), QLocale::LongFormat));
		}
	}
	if (role == Qt::UserRole)
		return QVariant::fromValue(m_tags.at(index.row()));

	return QVariant();
}

QVariant TagTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
	{
		switch (section)
		{
		case Column::ID: return tr("ID");
		case Column::Name: return tr("Name");
		case Column::Description: return tr("Description");
		case Column::URLs: return tr("URLs");
		case Column::Degree: return tr("Degree");
		case Column::Created: return tr("Created");
		case Column::Modified: return tr("Modified");
		}
	}
	return QVariant();
}

int TagTableModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return m_tags.size();
}

int TagTableModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return 7;
}

void TagTableModel::addTag(const QSharedPointer<Tag> tag)
{
	beginInsertRows(QModelIndex(), m_tags.count(), m_tags.count());
	m_tags.append(tag);
	endInsertRows();
}

void TagTableModel::removeTag(int row)
{
	if (row < 0 || row >= m_tags.count())
		return;
	beginRemoveRows(QModelIndex(), row, row);
	m_tags.removeAt(row);
	endRemoveRows();
}

QSharedPointer<Tag> TagTableModel::tagAt(int row)
{
	if (row < 0 || row >= m_tags.size())
		return QSharedPointer<Tag>();
	return m_tags.at(row);
}

QList<QSharedPointer<Tag>> TagTableModel::tags()
{
	return m_tags;
}

bool TagTableModel::contains(const QSharedPointer<Tag>& tag)
{
	return m_tags.contains(tag);
}

void TagTableModel::clear()
{
	if (m_tags.isEmpty())
		return;
	beginRemoveRows(QModelIndex(), 0, m_tags.count() - 1);
	m_tags.clear();
	endRemoveRows();
}