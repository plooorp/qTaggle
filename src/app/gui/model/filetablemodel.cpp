#include "filetablemodel.h"

#include <QIcon>

FileTableModel::FileTableModel(QObject* parent)
	: QAbstractTableModel(parent)
{}

QVariant FileTableModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (index.row() < 0 || index.row() >= m_files.size())
		return QVariant();
	if (index.column() < 0 || index.column() >= columnCount(QModelIndex()))
		return QVariant();

	QSharedPointer<File> file = m_files.at(index.row());
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case Column::ID: return QString::number(file->id());
		case Column::Name: return file->name();
		case Column::Path: return file->path();
		case Column::State: return File::stateString(file->state());
		case Column::Sha1digest: return QString(file->sha1().toHex());
		case Column::Comment: return file->comment();
		case Column::Source: return file->source();
		case Column::Created: return QLocale().toString(file->created(), QLocale::ShortFormat);
		case Column::Modified: return QLocale().toString(file->modified(), QLocale::ShortFormat);
		}
	}
	if (role == Qt::ToolTipRole)
	{
		switch (index.column())
		{
		case Column::Sha1digest:
			return QString(file->sha1().toHex());
		case Column::Created:
			return QLocale().toString(file->created(), QLocale::LongFormat);
		case Column::Modified:
			return QLocale().toString(file->modified(), QLocale::LongFormat);
		}
	}
	if (role == Qt::TextAlignmentRole)
	{
		switch (index.column())
		{
		case Column::ID: return Qt::AlignRight;
		}
	}
	//if (role == Qt::DecorationRole)
	//{
	//	return QIcon::fromTheme(QIcon::ThemeIcon::);
	//}

	return QVariant();
}

QVariant FileTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
	{
		switch (section)
		{
			// Column::State
		case Column::ID: return tr("ID");
		case Column::Name: return tr("Name");
		case Column::Path: return tr("Path");
		case Column::Comment: return tr("Comment");
		case Column::Source: return tr("Source");
		case Column::Sha1digest: return tr("SHA-1");
		case Column::Created: return tr("Created");
		case Column::Modified: return tr("Modified");
		}
	}
	return QVariant();
}

int FileTableModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return m_files.size();
}

int FileTableModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return 9;
}

void FileTableModel::addFile(const QSharedPointer<File> file)
{
	beginInsertRows(QModelIndex(), m_files.size(), m_files.size());
	m_files.append(file);
	endInsertRows();
}

void FileTableModel::removeFile(int row)
{
	if (row < 0 || row >= m_files.size())
		return;
	beginRemoveRows(QModelIndex(), row, row);
	m_files.removeAt(row);
	endRemoveRows();
}

QSharedPointer<File> FileTableModel::fileAt(int row) const
{
	if (row < 0 || row >= m_files.size())
		return QSharedPointer<File>();
	return m_files.at(row);
}

bool FileTableModel::contains(const QSharedPointer<File>& file) const
{
	return m_files.contains(file);
}

void FileTableModel::clear()
{
	if (m_files.isEmpty())
		return;
	beginRemoveRows(QModelIndex(), 0, m_files.size() - 1);
	m_files.clear();
	endRemoveRows();
}
