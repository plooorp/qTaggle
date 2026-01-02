#pragma once

#include <QTreeWidget>
#include "app/gui/mainwindow.h"
#include "app/gui/filelist.h"

class Filters : public QTreeWidget
{
	Q_OBJECT

public:
	explicit Filters(MainWindow* mainWindow, FileList* fileList, QWidget* parent = nullptr);
	virtual ~Filters() override;

private slots:
	void refresh();
	void handleItemDoubleClicked(QTreeWidgetItem* item, int column) const;
	void showContextMenu(const QPoint& pos);
	void handleIncludeTag() const;
	void handleExcludeTag() const;

private:
	QTreeWidgetItem* m_state;
	QTreeWidgetItem* m_stateOk;
	QTreeWidgetItem* m_stateFileMissing;
	QTreeWidgetItem* m_stateChecksumChanged;
	QTreeWidgetItem* m_stateError;
	QTreeWidgetItem* m_tag;
	QTreeWidgetItem* m_dir;
	QAction* m_actionRefresh;
	QAction* m_actionIncludeTag;
	QAction* m_actionExcludeTag;
	MainWindow* m_mainWindow;
	FileList* m_fileList;
	void populate();
	void depopulate();
	void readSettings();
	void writeSettings();
};
