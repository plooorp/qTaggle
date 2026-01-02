#include "filetablemodel.h"

#include <QIcon>
#include <QStyle>
#include <QApplication>

FileTableModel::FileTableModel(QObject* parent)
	: QAbstractTableModel(parent)
{
	connect(db, &Database::closed, this, &FileTableModel::clear);
}

QVariant FileTableModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (index.row() < 0 || index.row() >= m_files.size())
		return QVariant();
	if (index.column() < 0 || index.column() >= columnCount(QModelIndex()))
		return QVariant();

	File file = m_files.at(index.row());
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case ID: return QString::number(file.id());
		case Name: return file.displayName();
		case Path: return file.dir();
		case Sha1digest: return QString(file.sha1().toHex());
		case Comment: return file.comment();
		case Source: return file.source();
		case Created: return QLocale().toString(file.created(), QLocale::ShortFormat);
		case Modified: return QLocale().toString(file.modified(), QLocale::ShortFormat);
		case Checked: return QLocale().toString(file.checked(), QLocale::ShortFormat);
		}
	}
	if (role == Qt::ToolTipRole)
	{
		switch (index.column())
		{
		case Name: return file.displayName();
		case Path: return file.dir();
		case Sha1digest: return QString(file.sha1().toHex());
		case Comment: return file.comment();
		case Source: return file.source();
		case Created: return QLocale().toString(file.created(), QLocale::LongFormat);
		case Modified: return QLocale().toString(file.modified(), QLocale::LongFormat);
		case Checked: return QLocale().toString(file.checked(), QLocale::LongFormat);
		}
	}
	if (role == Qt::TextAlignmentRole)
	{
		switch (index.column())
		{
		case ID: return Qt::AlignRight;
		}
	}
	if (role == Qt::DecorationRole && index.column() == Name)
	{
		switch (file.state())
		{
		case File::Ok: return QIcon(":/icons/file-ok.svg");
		case File::Error: return QIcon(":/icons/file-error.svg");
		case File::FileMissing: return QIcon(":/icons/file-notfound.svg");
		case File::ChecksumChanged: return QIcon(":/icons/file-checksumchanged.svg");
		}
	}
	return QVariant();
}

QVariant FileTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
	{
		return columnString[section];
		//switch (section)
		//{
		//case ID: return tr("ID");
		//case Name: return tr("Name");
		//case Path: return tr("Path");
		//case Comment: return tr("Comment");
		//case Source: return tr("Source");
		//case Sha1digest: return tr("SHA-1");
		//case Created: return tr("Date created");
		//case Modified: return tr("Last modified");
		//case Checked: return tr("Last checked");
		//}
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

void FileTableModel::addFile(const File& file)
{
	//connect(file.get(), &File::deleted, this, [this, file]() -> void { removeFile(file); });
	beginInsertRows(QModelIndex(), m_files.size(), m_files.size());
	m_files.append(file);
	endInsertRows();
}

bool FileTableModel::removeFile(const File& file)
{
	if (qsizetype i = m_files.indexOf(file); i >= 0)
	{
		//disconnect(m_files[i].get(), &File::deleted, this, nullptr);
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
	//disconnect(m_files[row].get(), &File::deleted, this, nullptr);
	beginRemoveRows(QModelIndex(), row, row);
	m_files.removeAt(row);
	endRemoveRows();
	return true;
}

File FileTableModel::fileAt(int row) const
{
	if (row < 0 || row >= m_files.size())
		return File();
	return m_files.at(row);
}

bool FileTableModel::contains(const File file) const
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
		? std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.name().compare(b.name(), Qt::CaseInsensitive) < 0; })
		: std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.name().compare(b.name(), Qt::CaseInsensitive) > 0; });
	else if (column == ID)
		order == Qt::AscendingOrder
		? std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.id() < b.id(); })
		: std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.id() > b.id(); });
	else if (column == Path)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.path().compare(b.path(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.path().compare(b.path(), Qt::CaseInsensitive) > 0; });
	else if (column == Comment)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.comment().compare(b.comment(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.comment().compare(b.comment(), Qt::CaseInsensitive) > 0; });
	else if (column == Source)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.source().compare(b.source(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.source().compare(b.source(), Qt::CaseInsensitive) > 0; });
	else if (column == Sha1digest)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.sha1().toHex().compare(b.sha1().toHex(), Qt::CaseInsensitive) < 0; })
			: std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.sha1().toHex().compare(b.sha1().toHex(), Qt::CaseInsensitive) > 0; });
	else if (column == Created)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.created().toSecsSinceEpoch() < b.created().toSecsSinceEpoch(); })
			: std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.created().toSecsSinceEpoch() > b.created().toSecsSinceEpoch(); });
	else if (column == Modified)
		order == Qt::AscendingOrder
			? std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.modified().toSecsSinceEpoch() < b.modified().toSecsSinceEpoch(); })
			: std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) -> bool { return a.modified().toSecsSinceEpoch() > b.modified().toSecsSinceEpoch(); });
	emit layoutChanged();
}

const QStringList FileTableModel::columnString = QStringList
{
	"ID",
	"Name",
	"Path",
	"Comment",
	"Source",
	"SHA-1",
	"Date created",
	"Last modified",
	"Last checked"
};
