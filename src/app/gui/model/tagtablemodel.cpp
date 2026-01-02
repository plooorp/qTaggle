#include "tagtablemodel.h"

#include "app/utils.h"

TagTableModel::TagTableModel(QObject* parent)
	: QAbstractTableModel(parent)
{
	connect(db, &Database::closed, this, &TagTableModel::clear);
}

QVariant TagTableModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (index.row() < 0 || index.row() >= m_tags.size())
		return QVariant();
	if (index.column() < 0 || index.column() >= columnCount(QModelIndex()))
		return QVariant();

	Tag tag = m_tags.at(index.row());
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case Column::ID:
			return QString::number(tag.id());
		case Column::Name:
			return tag.name();
		case Column::Description:
			return tag.description();
		case Column::Degree:
			return friendlyNumber(tag.degree());
		case Column::Created:
			return QLocale().toString(tag.created(), QLocale::ShortFormat);
		case Column::Modified:
			return QLocale().toString(tag.modified(), QLocale::ShortFormat);
		}
	}
	if (role == Qt::ToolTipRole)
	{
		switch (index.column())
		{
		case Column::Name:
			return QVariant(tag.name());
		case Column::Description:
			return QVariant(tag.description());
		case Column::Degree:
			return QVariant(QLocale().toString(tag.degree()));
		case Column::Created:
			return QVariant(QLocale().toString(tag.created(), QLocale::LongFormat));
		case Column::Modified:
			return QVariant(QLocale().toString(tag.modified(), QLocale::LongFormat));
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
		return columnString[section];
		//switch (section)
		//{
		//case Column::ID: return tr("ID");
		//case Column::Name: return tr("Name");
		//case Column::Description: return tr("Description");
		//case Column::Degree: return tr("Times used");
		//case Column::Created: return tr("Date created");
		//case Column::Modified: return tr("Date modified");
		//}
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
	return 6;
}

void TagTableModel::addTag(const Tag tag)
{
	//connect(tag.get(), &Tag::deleted, this, [this, tag]() -> void { removeTag(tag); });
	beginInsertRows(QModelIndex(), m_tags.count(), m_tags.count());
	m_tags.append(tag);
	endInsertRows();
}

bool TagTableModel::removeTag(const Tag tag)
{
	if (qsizetype i = m_tags.indexOf(tag); i >= 0)
	{
		//disconnect(m_tags[i].get(), &Tag::deleted, this, nullptr);
		beginRemoveRows(QModelIndex(), i, i);
		m_tags.removeAt(i);
		endRemoveRows();
		return true;
	}
	return false;
}

bool TagTableModel::removeTag(int row)
{
	if (row < 0 || row >= m_tags.size())
		return false;
	//disconnect(m_tags[row].get(), &Tag::deleted, this, nullptr);
	beginRemoveRows(QModelIndex(), row, row);
	m_tags.removeAt(row);
	endRemoveRows();
	return true;
}

Tag TagTableModel::tagAt(int row) const
{
	if (row < 0 || row >= m_tags.size())
		return Tag();
	return m_tags.at(row);
}

QList<Tag> TagTableModel::tags() const
{
	return m_tags;
}

bool TagTableModel::contains(const Tag tag) const
{
	return m_tags.contains(tag);
}

void TagTableModel::clear()
{
	if (m_tags.isEmpty())
		return;
	for (int i = m_tags.size() - 1; i >= 0; i--)
		removeTag(i);
}

void TagTableModel::sort(int column, Qt::SortOrder order)
{
	if (column == ID)
		order == Qt::AscendingOrder
			? std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.id() < b.id(); })
			: std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.id() > b.id(); });
	else if (column == Name)
		order == Qt::AscendingOrder
			? std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.name().compare(b.name(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.name().compare(b.name(), Qt::CaseInsensitive) > 0; });
	else if (column == Description)
		order == Qt::AscendingOrder
			? std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.description().compare(b.description(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.description().compare(b.description(), Qt::CaseInsensitive) > 0; });
	else if (column == Degree)
		order == Qt::AscendingOrder
			? std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.degree() < b.degree(); })
			: std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.degree() > b.degree(); });
	else if (column == Created)
		order == Qt::AscendingOrder
			? std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.created().toSecsSinceEpoch() < b.created().toSecsSinceEpoch(); })
			: std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.created().toSecsSinceEpoch() > b.created().toSecsSinceEpoch(); });
	else if (column == Modified)
		order == Qt::AscendingOrder
			? std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.modified().toSecsSinceEpoch() < b.modified().toSecsSinceEpoch(); })
			: std::sort(m_tags.begin(), m_tags.end(), [](const Tag& a, const Tag& b) -> bool { return a.modified().toSecsSinceEpoch() > b.modified().toSecsSinceEpoch(); });
	emit layoutChanged();
}

const QStringList TagTableModel::columnString = QStringList
{
	"ID",
	"Name",
	"Description",
	"Times used",
	"Date created",
	"Last modified"
};
