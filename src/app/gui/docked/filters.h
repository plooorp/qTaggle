#pragma once

#include <QTreeWidget>

class Filters : public QTreeWidget
{
	Q_OBJECT

public:
	explicit Filters(QWidget* parent);
	~Filters();

private slots:
	void refresh();
	void showContextMenu(const QPoint& pos);

private:
	QTreeWidgetItem* m_state;
	QTreeWidgetItem* m_tag;
	QTreeWidgetItem* m_dir;
	QAction* m_actionRefresh;
	void populate();
	void depopulate();
	void readSettings();
	void writeSettings();
};
