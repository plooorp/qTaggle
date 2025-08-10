#include "filetablemodel.h"

#include <QIcon>
#include <QStyle>
#include <QApplication>

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
	if (role == Qt::DecorationRole && index.column() == Name)
	{
		switch (file->state())
		{
		case File::Ok: return qApp->style()->standardIcon(QStyle::SP_DialogApplyButton);
		case File::ChecksumChanged: return QIcon::fromTheme(QIcon::ThemeIcon::SyncError);
		case File::FileMissing: return qApp->style()->standardIcon(QStyle::SP_MessageBoxWarning);
		case File::Error: return qApp->style()->standardIcon(QStyle::SP_MessageBoxCritical);
		}
	}

	return QVariant();
}

QVariant FileTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
	{
		switch (section)
		{
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
	return 8;
}

void FileTableModel::addFile(const QSharedPointer<File>& file)
{
	connect(file.get(), &File::deleted, this, [this, file]() -> void { removeFile(file); });
	beginInsertRows(QModelIndex(), m_files.size(), m_files.size());
	m_files.append(file);
	endInsertRows();
}

bool FileTableModel::removeFile(const QSharedPointer<File>& file)
{
	if (qsizetype i = m_files.indexOf(file); i >= 0)
	{
		disconnect(m_files[i].get(), &File::deleted, this, nullptr);
		beginRemoveRows(QModelIndex(), i, i);
		m_files.removeAt(i);
		endRemoveRows();
		return true;
	}
	return false;
}

bool FileTableModel::removeFile(int row)
{
	if (row < 0 || row >= m_files.size())
		return false;
	disconnect(m_files[row].get(), &File::deleted, this, nullptr);
	beginRemoveRows(QModelIndex(), row, row);
	m_files.removeAt(row);
	endRemoveRows();
	return true;
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
	for (int i = m_files.size() - 1; i >= 0; i--)
		removeFile(i);
}

void FileTableModel::sort(int column, Qt::SortOrder order)
{
	if (column == Name)
		order == Qt::AscendingOrder
		? std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->name().compare(b->name(), Qt::CaseInsensitive) < 0; })
		: std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->name().compare(b->name(), Qt::CaseInsensitive) > 0; });
	else if (column == ID)
		order == Qt::AscendingOrder
		? std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->id() < b->id(); })
		: std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->id() > b->id(); });
	else if (column == Path)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->path().compare(b->path(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->path().compare(b->path(), Qt::CaseInsensitive) > 0; });
	else if (column == Comment)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->comment().compare(b->comment(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->comment().compare(b->comment(), Qt::CaseInsensitive) > 0; });
	else if (column == Source)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->source().compare(b->source(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->source().compare(b->source(), Qt::CaseInsensitive) > 0; });
	else if (column == Sha1digest)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->sha1().toHex().compare(b->sha1().toHex(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->sha1().toHex().compare(b->sha1().toHex(), Qt::CaseInsensitive) > 0; });
	else if (column == Created)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->created().toSecsSinceEpoch() < b->created().toSecsSinceEpoch(); })
			: std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->created().toSecsSinceEpoch() > b->created().toSecsSinceEpoch(); });
	else if (column == Modified)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->modified().toSecsSinceEpoch() < b->modified().toSecsSinceEpoch(); })
			: std::sort(m_files.begin(), m_files.end(), [](const QSharedPointer<File>& a, const QSharedPointer<File>& b) -> bool { return a->modified().toSecsSinceEpoch() > b->modified().toSecsSinceEpoch(); });
	emit layoutChanged();
}
