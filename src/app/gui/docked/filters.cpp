#include "filters.h"

#include <QSettings>
#include <QMenu>

#include "app/tag.h"
#include "app/file.h"
#include "app/utils.h"

Filters::Filters(MainWindow* mainWindow, FileList* fileList, QWidget* parent)
	: QTreeWidget(parent)
	, m_mainWindow(mainWindow)
	, m_fileList(fileList)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	setHeaderHidden(true);
	setColumnCount(1);
	setRootIsDecorated(false);
	//setIndentation(0);

	m_state = new QTreeWidgetItem(this, QStringList{ tr("States") });
	QTreeWidgetItem* item;
	item = new QTreeWidgetItem(m_state, QStringList{ File::stateString[File::Ok] });
	item->setIcon(0, QIcon(":/icons/file-ok.svg"));
	item->setData(0, Qt::UserRole, File::Ok);
	item = new QTreeWidgetItem(m_state, QStringList{ File::stateString[File::FileMissing] });
	item->setIcon(0, QIcon(":/icons/file-notfound.svg"));
	item->setData(0, Qt::UserRole, File::FileMissing);
	item = new QTreeWidgetItem(m_state, QStringList{ File::stateString[File::ChecksumChanged] });
	item->setIcon(0, QIcon(":/icons/file-checksumchanged.svg"));
	item->setData(0, Qt::UserRole, File::ChecksumChanged);
	item = new QTreeWidgetItem(m_state, QStringList{ File::stateString[File::Error] });
	item->setIcon(0, QIcon(":/icons/file-error.svg"));
	item->setData(0, Qt::UserRole, File::Error);
	m_tag = new QTreeWidgetItem(this, QStringList{ tr("Tags") });

	m_actionIncludeTag = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd), u"Include in search"_s, this);
	m_actionExcludeTag = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove), u"Exclude from search"_s, this);
	connect(m_actionIncludeTag, &QAction::triggered, this, &Filters::handleIncludeTag);
	connect(m_actionExcludeTag, &QAction::triggered, this, &Filters::handleExcludeTag);
	connect(this, &QTreeWidget::itemDoubleClicked, this, &Filters::handleItemDoubleClicked);

	connect(db, &Database::opened, this, &Filters::populate);
	connect(db, &Database::closed, this, &Filters::depopulate);
	connect(db, &Database::updated, this, &Filters::refresh);

	m_actionRefresh = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh), tr("Refresh"), this);
	connect(this, &QWidget::customContextMenuRequested, this, &Filters::showContextMenu);
	connect(m_actionRefresh, &QAction::triggered, this, &Filters::refresh);

	readSettings();
	populate();
}

Filters::~Filters()
{
	writeSettings();
}

void Filters::populate()
{
	if (!db->isOpen())
		return;
	
	// update state counters
	for (int i = 0; i < m_state->childCount(); ++i)
	{
		QTreeWidgetItem* item = m_state->child(i);
		File::State state = static_cast<File::State>(item->data(0, Qt::UserRole).toInt());
		int64_t count = File::countByState(state);
		item->setText(0, u"%1 (%2)"_s.arg(File::stateString[state], friendlyNumber(count)));
		item->setToolTip(0, u"%1 (%2)"_s.arg(File::stateString[state], QString::number(count)));
	}

	// update popular tags
	sqlite3_stmt* stmt;
	const char* sql = "SELECT id FROM tag ORDER BY degree DESC, name ASC LIMIT 24;";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const Tag tag = Tag(sqlite3_column_int64(stmt, 0));
		QTreeWidgetItem* item = new QTreeWidgetItem(m_tag, QStringList{ u"%1 (%2)"_s.arg(tag.name(), friendlyNumber(tag.degree())) });
		item->setIcon(0, QIcon(":/icons/tag.svg"));
		item->setToolTip(0, tag.name() + " " + QLocale().toString(tag.degree()));
		item->setData(0, Qt::UserRole, tag.name());
	}
	sqlite3_finalize(stmt);
}

void Filters::depopulate()
{
	for (const QTreeWidgetItem* item : m_tag->takeChildren())
		delete item;
	//for (const QTreeWidgetItem* item : m_dir->takeChildren())
	//	delete item;
}

//void Filters::onItemDoubleClicked(QTreeWidgetItem* item, int column)
//{
//	item->data();
//}

void Filters::readSettings()
{
	QSettings settings;
	m_state->setExpanded(settings.value("GUI/Filters/stateExpanded", true).toBool());
	m_tag->setExpanded(settings.value("GUI/Filters/tagExpanded", true).toBool());
}

void Filters::writeSettings()
{
	QSettings settings;
	settings.setValue("GUI/Filters/stateExpanded", m_state->isExpanded());
	settings.setValue("GUI/Filters/tagExpanded", m_tag->isExpanded());
}

void Filters::refresh()
{
	depopulate();
	populate();
}

void Filters::showContextMenu(const QPoint& pos)
{
	if (!indexAt(pos).isValid())
		clearSelection();
	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	QList<QTreeWidgetItem*> selected = selectedItems();
	if (selected.size() > 0)
	{
		QTreeWidgetItem* item = selected.first();
		QTreeWidgetItem* parent = item->parent();
		if (parent == m_tag)
		{
			menu->addAction(m_actionIncludeTag);
			menu->addAction(m_actionExcludeTag);
		}
	}
	menu->addSeparator();
	menu->addAction(m_actionRefresh);
	menu->popup(QCursor::pos());
}

void Filters::handleItemDoubleClicked(QTreeWidgetItem* item, int column) const
{
	if (m_mainWindow->currentTab() != MainWindow::File)
		return;
	QTreeWidgetItem* parent = item->parent();
	if (!parent)
		return;
	if (parent == m_tag)
		m_fileList->appendToTagQuery(item->data(0, Qt::UserRole).toString());
}

void Filters::handleIncludeTag() const
{
	if (m_mainWindow->currentTab() != MainWindow::File)
		return;
	QList<QTreeWidgetItem*> selected = selectedItems();
	if (selected.isEmpty())
		return;
	QTreeWidgetItem* item = selected.first();
	QTreeWidgetItem* parent = item->parent();
	if (!parent)
		return;
	if (parent == m_tag)
		m_fileList->appendToTagQuery(item->data(0, Qt::UserRole).toString());
}

void Filters::handleExcludeTag() const
{
	if (m_mainWindow->currentTab() != MainWindow::File)
		return;
	QList<QTreeWidgetItem*> selected = selectedItems();
	if (selected.isEmpty())
		return;
	QTreeWidgetItem* item = selected.first();
	QTreeWidgetItem* parent = item->parent();
	if (!parent)
		return;
	if (parent == m_tag)
		m_fileList->appendToTagQuery(u"!"_s + item->data(0, Qt::UserRole).toString());
}
