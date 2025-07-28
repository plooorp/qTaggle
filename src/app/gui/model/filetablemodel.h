#pragma once

#include <QAbstractTableModel>
#include "app/file.h"

class FileTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	explicit FileTableModel(QObject* parent = nullptr);
	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	void addFile(const QSharedPointer<File> file);
	void removeFile(int row);
	QSharedPointer<File> fileAt(int row);
	bool contains(const QSharedPointer<File>& file);
	void clear();

private:
	QList<QSharedPointer<File>> m_files;
	enum Column
	{
		Name = 0,
		ID,
		Path,
		State,
		Comment,
		Source,
		Sha1digest,
		Created,
		Modified
	};
};
