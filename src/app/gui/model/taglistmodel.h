#pragma once

#include <QAbstractListModel>
#include "app/tag.h"

class TagListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	explicit TagListModel(QObject* parent = nullptr);
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	void addTag(const QSharedPointer<Tag> tag);
	void removeTag(int row);
	//bool removeTag(QSharedPointer<Tag> tag);
	void clear();
	QSharedPointer<Tag> tagAt(int row);

private:
	QList<QSharedPointer<Tag>> m_tags;
};