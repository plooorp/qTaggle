#pragma once

#include <QAbstractTableModel>
#include "app/tag.h"

class TagTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	explicit TagTableModel(QObject* parent = nullptr);
	enum Column
	{
		ID = 0,
		Name,
		Description,
		Degree,
		Created,
		Modified
	};
	static const QStringList columnString;
	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	void addTag(const Tag tag);
	bool removeTag(const Tag tag);
	bool removeTag(int row);
	Tag tagAt(int row) const;
	QList<Tag> tags() const;
	bool contains(const Tag tag) const;
	void clear();
	void sort(int column, Qt::SortOrder order) override;

private:
	QList<Tag> m_tags;
};
