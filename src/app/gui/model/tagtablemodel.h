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
	QSharedPointer<Tag> tagAt(int row) const;
	QList<QSharedPointer<Tag>> tags();
	bool contains(const QSharedPointer<Tag>& tag) const;
	void clear();
	void sort(int column, Qt::SortOrder order) override;

private:
	QList<QSharedPointer<Tag>> m_tags;
	enum Column
	{
		Name = 0,
		Description,
		ID,
		URLs,
		Degree,
		Created,
		Modified
	};
};
