#include "taglist.h"
#include "ui_taglist.h"

#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include "app/gui/dialog/edittagdialog.h"
#include "app/gui/dialog/edittagdialogmulti.h"
#include "app/gui/dialog/newtagdialog.h"
#include "app/globals.h"

TagList::TagList(QWidget* parent)
	: QWidget(parent)
	, m_ui(new Ui::TagList)
	, m_model(new TagTableModel(this))
{
	m_ui->setupUi(this);

	// context menu actions
	connect(m_ui->actionCreate, &QAction::triggered, this, &TagList::actionCreate_triggered);
	connect(db, &Database::opened, this, [this]() -> void { m_ui->actionCreate->setEnabled(true); });
	connect(db, &Database::closed, this, [this]() -> void { m_ui->actionCreate->setEnabled(false); });
	connect(m_ui->actionEdit, &QAction::triggered, this, &TagList::actionEdit_triggered);
	connect(m_ui->actionDelete, &QAction::triggered, this, &TagList::actionDelete_triggered);

	// hamburger menu
	m_ui->menuButton->setIcon(QIcon(":/icons/menu.svg"));
	QMenu* menu = new QMenu(this);
	menu->addAction(m_ui->actionSortByRelevancy);
	m_ui->menuButton->setMenu(menu);

	QHeaderView* header = m_ui->treeView->header();
	header->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(header, &QHeaderView::customContextMenuRequested, this, &TagList::showHeaderContextMenu);

	m_ui->treeView->setModel(m_model);
	connect(m_ui->treeView, &QTreeView::doubleClicked, this, &TagList::editSelected);
	connect(m_ui->treeView, &QWidget::customContextMenuRequested, this, &TagList::showContextMenu);
	connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() -> void { emit selectionChanged(selectedTags()); });

	m_ui->sortBy->addItem(tr("Name"), u"tag.name"_s);
	m_ui->sortBy->addItem(tr("Times used"), u"tag.degree"_s);
	m_ui->sortBy->addItem(tr("Date created"), u"tag.created"_s);
	m_ui->sortBy->addItem(tr("Last modified"), u"tag.modified"_s);

	m_ui->sortOrder->addItem(tr("Ascending"), u"ASC"_s);
	m_ui->sortOrder->addItem(tr("Descending"), u"DESC"_s);

	readSettings();

	connect(db, &Database::opened, this, &TagList::populate);
	connect(db, &Database::updated, this, &TagList::populate);
	connect(m_ui->lineEdit, &QLineEdit::textEdited, this, &TagList::populate);
	connect(m_ui->paginator, &Paginator::pageChangedByUser, this, &TagList::populate);
	connect(m_ui->resultsPerPage, &QSpinBox::editingFinished, this, &TagList::populate);
	connect(m_ui->sortBy, &QComboBox::currentIndexChanged, this, &TagList::populate);
	connect(m_ui->sortOrder, &QComboBox::currentIndexChanged, this, &TagList::populate);
	connect(m_ui->actionSortByRelevancy, &QAction::toggled, this, &TagList::populate);
}

TagList::~TagList()
{
	writeSettings();
	delete m_ui;
}

void TagList::populate()
{
	const QString query = m_ui->lineEdit->text();
	QString sortBy = m_ui->sortBy->currentData().toString();
	QString sortOrder = m_ui->sortOrder->currentData().toString();
	const int limit = m_ui->resultsPerPage->value();
	const int offset = m_ui->paginator->page() * limit;
	sqlite3_stmt* stmt;
	int64_t count = 0;
	m_model->clear();

	if (query.trimmed().isEmpty())
	{
		QString sql = uR"(
			SELECT id FROM tag
			ORDER BY %1 %2, name ASC
			LIMIT ? OFFSET ?;
		)"_s.arg(sortBy, sortOrder);
		QByteArray sql_bytes = sql.toUtf8();
		sqlite3_prepare_v2(db->con(), sql_bytes, -1, &stmt, nullptr);
		sqlite3_bind_int(stmt, 1, limit);
		sqlite3_bind_int(stmt, 2, offset);
		while (sqlite3_step(stmt) == SQLITE_ROW)
			m_model->addTag(Tag(sqlite3_column_int64(stmt, 0)));
		sqlite3_finalize(stmt);

		sqlite3_prepare_v2(db->con(), "SELECT COUNT(*) FROM tag;", -1, &stmt, nullptr);
		if (sqlite3_step(stmt) == SQLITE_ROW)
			count = sqlite3_column_int64(stmt, 0);
		sqlite3_finalize(stmt);
	}
	else
	{
		if (m_ui->actionSortByRelevancy->isChecked())
		{
			sortBy = "relevancy";
			sortOrder = "ASC";
		}
		QStringList query_parts = query.split(' ', Qt::SkipEmptyParts);
		for (QString& part : query_parts)
			part = part + u"*"_s;
		QByteArray query_bytes = query_parts.join(' ').toUtf8();
		QString sql = uR"(
			SELECT tag.id, bm25(tag_search.tag_search, 10.0, 5.0) AS relevancy
			FROM tag
			INNER JOIN tag_search ON tag_search.ROWID = tag.id
			WHERE tag_search MATCH ?
			ORDER BY %1 %2, tag.name ASC
			LIMIT ? OFFSET ?;
		)"_s.arg(sortBy, sortOrder);
		sqlite3_prepare_v2(db->con(), sql.toUtf8(), -1, &stmt, nullptr);
		sqlite3_bind_text(stmt, 1, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 2, limit);
		sqlite3_bind_int(stmt, 3, offset);
		while (sqlite3_step(stmt) == SQLITE_ROW)
			m_model->addTag(Tag(sqlite3_column_int64(stmt, 0)));
		sqlite3_finalize(stmt);

		const char* sql_count = R"(
			SELECT COUNT(*) FROM tag WHERE id IN(
				SELECT ROWID FROM tag_search
				WHERE tag_search MATCH ?
			);
		)";
		sqlite3_prepare_v2(db->con(), sql_count, -1, &stmt, nullptr);
		sqlite3_bind_text(stmt, 1, query_bytes.constData(), -1, SQLITE_STATIC);
		if (sqlite3_step(stmt) == SQLITE_ROW)
			count = sqlite3_column_int64(stmt, 0);
		sqlite3_finalize(stmt);
	}
	int resultsPerPage = m_ui->resultsPerPage->value();
	int maxPage = std::max(0, static_cast<int>(std::ceil(static_cast<double>(count) / resultsPerPage)) - 1);
	m_ui->paginator->setMaxPage(maxPage);
}

QList<Tag> TagList::selectedTags() const
{
	QList<Tag> tags;
	for (const QModelIndex& index : m_ui->treeView->selectionModel()->selectedRows())
		tags.append(m_model->tagAt(index.row()));
	return tags;
}

void TagList::editSelected()
{
	QList<Tag> tags = selectedTags();
	if (tags.isEmpty())
		return;
	QDialog* dialog;
	if (tags.size() == 1)
		dialog = new EditTagDialog(tags.first(), this);
	else
		dialog = new EditTagDialogMulti(tags, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}

void TagList::deleteSelected()
{
	QList<Tag> tags = selectedTags();
	if (tags.isEmpty())
		return;
	QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Confirm action")
		, tags.size() > 1
			? tr("Are you sure you want to delete %1 tags?").arg(tags.size())
			: tr("Are you sure you want to delete '%1'?").arg(tags.first().name())
	);
	if (btn == QMessageBox::Yes)
		for (const Tag& tag : tags)
			tag.remove();
}

void TagList::actionCreate_triggered()
{
	NewTagDialog* dialog = new NewTagDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}

void TagList::actionDelete_triggered()
{
	deleteSelected();
}

void TagList::actionEdit_triggered()
{
	editSelected();
}

void TagList::showContextMenu(const QPoint& pos)
{
	if (!m_ui->treeView->indexAt(pos).isValid())
		m_ui->treeView->clearSelection();

	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	if (!m_ui->treeView->selectionModel()->selectedRows().isEmpty())
	{
		menu->addAction(m_ui->actionEdit);
		menu->addAction(m_ui->actionDelete);
		menu->addSeparator();
		menu->addAction(m_ui->actionCreate);
	}
	else
	{
		menu->addAction(m_ui->actionCreate);
	}
	menu->popup(QCursor::pos());
}

void TagList::showHeaderContextMenu(const QPoint& pos)
{
	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	for (int column = 0; column < TagTableModel::columnString.size(); ++column)
	{
		QAction* action = new QAction(TagTableModel::columnString[column], menu);
		action->setCheckable(true);
		action->setChecked(!m_ui->treeView->isColumnHidden(column));
		connect(action, &QAction::toggled, this, [this, column](bool checked) -> void
			{
				m_ui->treeView->setColumnHidden(column, !checked);
			});
		menu->addAction(action);
	}
	menu->popup(QCursor::pos());
}

void TagList::readSettings()
{
	QSettings settings;
	const QByteArray headerState = settings.value("GUI/TagList/headerState", QByteArray()).toByteArray();
	if (headerState.isEmpty())
		m_ui->treeView->setColumnHidden(TagTableModel::ID, true);
	else
		m_ui->treeView->header()->restoreState(headerState);
	m_ui->resultsPerPage->setValue(settings.value("GUI/TagList/resultsPerPage", 64).toInt());
	m_ui->sortBy->setCurrentIndex(settings.value("GUI/TagList/sortBy", 0).toInt());
	m_ui->sortOrder->setCurrentIndex(settings.value("GUI/TagList/sortOrder", 0).toInt());
	m_ui->actionSortByRelevancy->setChecked(settings.value("GUI/TagList/sortByRelevancy", true).toBool());
}

void TagList::writeSettings()
{
	QSettings settings;
	settings.setValue("GUI/TagList/headerState", m_ui->treeView->header()->saveState());
	settings.setValue("GUI/TagList/resultsPerPage", m_ui->resultsPerPage->value());
	settings.setValue("GUI/TagList/sortBy", m_ui->sortBy->currentIndex());
	settings.setValue("GUI/TagList/sortOrder", m_ui->sortBy->currentIndex());
	settings.setValue("GUI/TagList/sortByRelevancy", m_ui->actionSortByRelevancy->isChecked());
}
