#include "taglistmodel.h"

#include "app/utils.h"

TagListModel::TagListModel(QObject* parent)
	: QAbstractListModel(parent)
{}

int TagListModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return m_tags.size();
}

QVariant TagListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (index.row() >= m_tags.size() || index.row() < 0)
		return QVariant();
	
	QSharedPointer<Tag> tag = m_tags.at(index.row());
	if (role == Qt::DisplayRole || role == Qt::EditRole)
		return QVariant(tag->name() + " " + friendlyNumber(tag->degree()));
	if (role == Qt::ToolTipRole)
		return QVariant(tag->description());
	if (role == Qt::UserRole)
		return QVariant::fromValue(m_tags.at(index.row()));
	
	return QVariant();
}

void TagListModel::addTag(const QSharedPointer<Tag> tag)
{
	beginInsertRows(QModelIndex(), m_tags.count(), m_tags.count());
	m_tags.append(tag);
	endInsertRows();
}

void TagListModel::removeTag(int row)
{
	if (row < 0 || row >= m_tags.count())
		return;
	beginRemoveRows(QModelIndex(), row, row);
	m_tags.removeAt(row);
	endRemoveRows();
}

//bool TagListModel::removeTag(QSharedPointer<Tag> tag)
//{
//	for (int i = 0; i < m_tags.size(); i++)
//		if (m_tags.at(i) == tag)
//		{
//			beginRemoveRows(QModelIndex(), i, i);
//			m_tags.removeAt(i);
//			endRemoveRows();
//			return true;
//		}
//	return false;
//}

void TagListModel::clear()
{
	if (m_tags.isEmpty())
		return;
	beginRemoveRows(QModelIndex(), 0, m_tags.count() - 1);
	m_tags.clear();
	endRemoveRows();
}

QSharedPointer<Tag> TagListModel::tagAt(int row)
{
	if (row < 0 || row >= m_tags.size())
		return QSharedPointer<Tag>();
	return m_tags.at(row);
}