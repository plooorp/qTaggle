#include "filters.h"

#include <QSettings>
#include <QMenu>

#include "app/tag.h"
#include "app/file.h"
#include "app/utils.h"

Filters::Filters(QWidget* parent)
	: QTreeWidget(parent)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setHeaderHidden(true);
	setColumnCount(1);

	connect(db, &Database::opened, this, &Filters::populate);
	connect(db, &Database::closed, this, &Filters::depopulate);

	m_actionRefresh = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh), tr("Refresh"), this);
	connect(this, &QWidget::customContextMenuRequested, this, &Filters::showContextMenu);
	connect(m_actionRefresh, &QAction::triggered, this, &Filters::refresh);

	m_state = new QTreeWidgetItem(this, QStringList{ "States" });
	new QTreeWidgetItem(m_state, QStringList{ File::stateString(File::Ok) });
	new QTreeWidgetItem(m_state, QStringList{ File::stateString(File::Error) });
	new QTreeWidgetItem(m_state, QStringList{ File::stateString(File::FileMissing) });
	new QTreeWidgetItem(m_state, QStringList{ File::stateString(File::ChecksumChanged) });

	m_tag = new QTreeWidgetItem(this, QStringList{ "Tags" });

	m_dir = new QTreeWidgetItem(this, QStringList{ "Locations" });

	populate();
	readSettings();
}

Filters::~Filters()
{
	writeSettings();
}

void Filters::populate()
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM tag ORDER BY degree DESC LIMIT 24;", -1, &stmt, nullptr);
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const QSharedPointer<Tag> tag = Tag::fromStmt(stmt);
		QTreeWidgetItem* item = new QTreeWidgetItem(m_tag, QStringList{ tag->name() + " " + friendlyNumber(tag->degree()) });
		item->setToolTip(0, tag->name() + " " + QLocale().toString(tag->degree()));
	}
	sqlite3_finalize(stmt);
	sqlite3_prepare_v2(db->con(), "SELECT DISTINCT dir FROM file ORDER BY dir;", -1, &stmt, nullptr);
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const QString dir = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
		QTreeWidgetItem* item = new QTreeWidgetItem(m_dir, QStringList{ dir });
		item->setToolTip(0, dir);
	}
	sqlite3_finalize(stmt);
}

void Filters::depopulate()
{
	for (const QTreeWidgetItem* item : m_tag->takeChildren())
		delete item;
	for (const QTreeWidgetItem* item : m_dir->takeChildren())
		delete item;
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

void Filters::refresh()
{
	depopulate();
	populate();
}

void Filters::showContextMenu(const QPoint& pos)
{
	if (indexAt(pos).isValid())
		clearSelection();
	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	menu->addAction(m_actionRefresh);
	menu->popup(QCursor::pos());
}
