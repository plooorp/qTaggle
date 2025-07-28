#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItem>

class Filters : public QTreeWidget
{
	Q_OBJECT

public:
	explicit Filters(QWidget* parent);

private:
	QTreeWidgetItem* m_state;
	QTreeWidgetItem* m_tag;
	QTreeWidgetItem* m_dir;
	void populate();
	void readSettings();
	void writeSettings();
};
