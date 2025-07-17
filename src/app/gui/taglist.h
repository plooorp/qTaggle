#pragma once

#include <QWidget>
#include <QStandardItemModel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTreeView>
#include "app/tag.h"
#include "app/gui/model/tagtablemodel.h"

namespace Ui
{
	class TagList;
}

class TagList : public QWidget
{
	Q_OBJECT

public:
	explicit TagList(QWidget* parent = nullptr);
	~TagList();
	QList<QSharedPointer<Tag>> selectedTags() const;
	void editSelected();
	void deleteSelected();

private slots:
	void actionCreate_triggered();
	void actionDelete_triggered();
	void actionEdit_triggered();
	void showContextMenu(const QPoint& pos);

private:
	Ui::TagList* m_ui;
	TagTableModel* m_model;
	void populate();
	void depopulate();
};
