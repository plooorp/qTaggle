#pragma once

#include <QAbstractTableModel>
#include "app/file.h"

class FileTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	explicit FileTableModel(QObject* parent = nullptr);
	enum Column
	{
		ID = 0,
		Name,
		Path,
		Comment,
		Source,
		Sha1digest,
		Created,
		Modified,
		Checked
	};
	static const QStringList columnString;
	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	void addFile(const File& file);
	bool removeFile(const File& file);
	bool removeFile(int row);
	File fileAt(int row) const;
	bool contains(const File file) const;
	void clear();
	void sort(int column, Qt::SortOrder order) override;

private:
	QList<File> m_files;
};
