#pragma once

#include <QWidget>

#include "app/gui/model/filetablemodel.h"

namespace Ui
{
	class FileList;
}

class FileList : public QWidget
{
	Q_OBJECT

public:
	explicit FileList(QWidget* parent = nullptr);
	~FileList();
	QList<QSharedPointer<File>> selectedFiles() const;
	void editSelected();
	void checkSelected();
	void deleteSelected();

private slots:
	void actionAdd_triggered();
	void actionEdit_triggered();
	void actionCheck_triggered();
	void actionDelete_triggered();
	void showContextMenu(const QPoint& pos);

signals:
	void selectionChanged();

private:
	Ui::FileList* m_ui;
	FileTableModel* m_model;
	void populate();
	void readSettings();
	void writeSettings();
};
