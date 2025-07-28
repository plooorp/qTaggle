#include "filters.h"

#include <QSettings>

#include "app/tag.h"
#include "app/file.h"
#include "app/utils.h"

Filters::Filters(QWidget* parent)
	: QTreeWidget(parent)
{
	setHeaderHidden(true);
	setColumnCount(1);

	connect(db, &Database::opened, this, &Filters::populate);
	connect(db, &Database::closed, this, &Filters::clear);

	m_state = new QTreeWidgetItem(this, QStringList{ "States" });
	new QTreeWidgetItem(m_state, QStringList{ File::stateString(File::Ok) });
	new QTreeWidgetItem(m_state, QStringList{ File::stateString(File::Error) });
	new QTreeWidgetItem(m_state, QStringList{ File::stateString(File::FileMissing) });
	new QTreeWidgetItem(m_state, QStringList{ File::stateString(File::ChecksumChanged) });

	m_tag = new QTreeWidgetItem(this, QStringList{ "Tags" });

	m_dir = new QTreeWidgetItem(this, QStringList{ "Locations" });

	populate();
}

void Filters::populate()
{
	sqlite3_stmt* stmt;

	for (const QTreeWidgetItem* item : m_tag->takeChildren())
		delete item;

	sqlite3_prepare_v2(db->con(), "SELECT * FROM tag ORDER BY degree DESC LIMIT 20;", -1, &stmt, nullptr);
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const QSharedPointer<Tag> tag = Tag::fromStmt(stmt);
		QTreeWidgetItem* item = new QTreeWidgetItem(m_tag, QStringList{ tag->name() + " " + friendlyNumber(tag->degree()) });
		item->setToolTip(0, tag->name() + " " + QLocale().toString(tag->degree()));
	}
	sqlite3_finalize(stmt);

	for (const QTreeWidgetItem* item : m_dir->takeChildren())
		delete item;

	sqlite3_prepare_v2(db->con(), "SELECT DISTINCT dir FROM file ORDER BY dir;", -1, &stmt, nullptr);
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const QString dir = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
		QTreeWidgetItem* item = new QTreeWidgetItem(m_dir, QStringList{ dir });
		item->setToolTip(0, dir);
	}
	sqlite3_finalize(stmt);
}

void Filters::readSettings()
{
	QSettings settings;
	m_state->setExpanded(settings.value("GUI/Filters/stateExpanded", true).toBool());
	m_tag->setExpanded(settings.value("GUI/Filters/tagExpanded", true).toBool());
	m_dir->setExpanded(settings.value("GUI/Filters/dirExpanded", true).toBool());
}

void Filters::writeSettings()
{
	QSettings settings;
	settings.setValue("GUI/Filters/stateExpanded", m_state->isExpanded());
	settings.setValue("GUI/Filters/tagExpanded", m_tag->isExpanded());
	settings.setValue("GUI/Filters/dirExpanded", m_dir->isExpanded());
}
