#pragma once

#include <QAbstractTableModel>
#include "app/tag.h"

class TagTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	explicit TagTableModel(QObject* parent = nullptr);
	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	void addTag(const QSharedPointer<Tag> tag);
	void removeTag(int row);
	QSharedPointer<Tag> tagAt(int row);
	QList<QSharedPointer<Tag>> tags();
	void clear();

private:
	QList<QSharedPointer<Tag>> m_tags;
	enum Column
	{
		Name,
		Description,
		ID,
		URLs,
		Degree,
		Created,
		Modified
	};
};