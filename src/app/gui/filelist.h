#pragma once

#include <QWidget>
#include <QTimer>

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
	virtual ~FileList() override;
	QList<File> selectedFiles() const;
	void editSelected();
	void checkSelected();
	void deleteSelected();
	void appendToTagQuery(const QString& text);

signals:
	void selectionChanged(QList<File> selected);

private slots:
	void actionAdd_triggered();
	void actionEdit_triggered();
	void actionOpen_triggered();
	void actionCheck_triggered();
	void actionDelete_triggered();
	void showTableContextMenu(const QPoint& pos);
	void showHeaderContextMenu(const QPoint& pos);

private:
	Ui::FileList* m_ui;
	FileTableModel* m_model;
	void populate();
	QString parseTags(const QString& query, QByteArrayList& include, QByteArrayList& exclude);
	void clearQuery();
	void readSettings();
	void writeSettings();
};
