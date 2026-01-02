#pragma once

#include <QWidget>
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
	virtual ~TagList() override;
	QList<Tag> selectedTags() const;
	void editSelected();
	void deleteSelected();

signals:
	void selectionChanged(QList<Tag> selected);

private slots:
	void actionCreate_triggered();
	void actionDelete_triggered();
	void actionEdit_triggered();
	void showContextMenu(const QPoint& pos);
	void showHeaderContextMenu(const QPoint& pos);

private:
	Ui::TagList* m_ui;
	TagTableModel* m_model;
	void populate();
	void readSettings();
	void writeSettings();
};
