#include "tagselect.h"
#include "ui_tagselect.h"

#include <QApplication>
#include <QCompleter>
#include <QStringListModel>
#include <QMenu>
#include <QClipboard>

#include "app/database.h"
#include "app/utils.h"

TagSelect::TagSelect(QWidget *parent)
	: TagSelect(QList<QSharedPointer<Tag>>(), parent)
{}

TagSelect::TagSelect(QList<QSharedPointer<Tag>> tags, QWidget* parent)
	: m_ui(new Ui::TagSelect)
	, m_model(new TagTableModel(this))
	//, m_completerModel(new QStringListModel(this))
	//, m_completer(new QCompleter(m_completerModel, this))
{
	m_ui->setupUi(this);

	connect(m_ui->buttonAdd, &QPushButton::pressed, this, &TagSelect::add);
	connect(m_ui->actionRemove, &QAction::triggered, this, &TagSelect::remove);
	connect(m_ui->lineEdit, &QLineEdit::returnPressed, this, &TagSelect::add);
	connect(m_ui->treeView, &QTreeView::doubleClicked, this, &TagSelect::remove);
	connect(m_ui->treeView, &QWidget::customContextMenuRequested, this, &TagSelect::showContextMenu);

	connect(m_ui->buttonImport, &QPushButton::pressed, this, &TagSelect::importTags);
	connect(m_ui->buttonExport, &QPushButton::pressed, this, &TagSelect::exportTags);

	//QCompleter completer = new 
	//m_ui->lineEdit->setCompleter()
	if (!tags.isEmpty())
		setTags(tags);
	m_ui->treeView->setModel(m_model);
}

TagSelect::~TagSelect()
{
	delete m_ui;
}

void TagSelect::populate()
{
	//for (int i = m_modelDeselected->rowCount() - 1; i >= 0; i--)
	//	m_modelDeselected->removeTag(i);
	//for (QSharedPointer<Tag> tag : Tag::fromQuery(m_ui->lineEditFilter->text()))
	//{
	//	for (int i = 0; i < m_modelSelected->rowCount(); i++)
	//		if (tag->id() == m_modelSelected->tagAt(i)->id())
	//			continue;
	//	m_modelDeselected->addTag(tag);
	//}
}

void TagSelect::add()
{
	QByteArray query = m_ui->lineEdit->text()
		.trimmed()
		.toLower()
		.toUtf8();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM tag WHERE name = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, query.constData(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		QSharedPointer<Tag> tag = Tag::fromStmt(stmt);
		m_model->addTag(tag);
		m_ui->lineEdit->clear();
	}
	sqlite3_finalize(stmt);
	//QModelIndexList selected = m_ui->listView_deselected->selectionModel()->selectedRows();
	//// greatest to least by row index
	//std::sort(selected.begin(), selected.end()
	//	, [](const QModelIndex& a, const QModelIndex& b) -> bool
	//	{
	//		return a.row() > b.row();
	//	});
	//for (const QModelIndex& index : selected)
	//{
	//	QSharedPointer<Tag> tag = m_modelDeselected->tagAt(index.row());
	//	//m_modelDeselected->removeTag(tag);
	//	m_modelDeselected->removeTag(index.row());
	//	m_modelSelected->addTag(tag);
	//}
}

void TagSelect::remove()
{
	QModelIndexList selected = m_ui->treeView->selectionModel()->selectedRows();
	std::sort(selected.begin(), selected.end()
		, [](const QModelIndex& a, const QModelIndex& b) -> bool
		{
			return a.row() > b.row();
		});
	for (const QModelIndex& index : selected)
		m_model->removeTag(index.row());
}

QList<QSharedPointer<Tag>> TagSelect::tags()
{
	return m_model->tags();
}

void TagSelect::setTags(const QList<QSharedPointer<Tag>> tags)
{
	m_model->clear();
	for (const QSharedPointer<Tag>& tag : tags)
		m_model->addTag(tag);
}

void TagSelect::showContextMenu(const QPoint& pos)
{
	if (!m_ui->treeView->indexAt(pos).isValid())
		m_ui->treeView->clearSelection();
	if (m_ui->treeView->selectionModel()->selectedRows().isEmpty())
		return;
	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	menu->addAction(m_ui->actionRemove);
	menu->popup(QCursor::pos());
}

void TagSelect::importTags()
{
	
}

void TagSelect::exportTags()
{
	QClipboard* clipboard = qApp->clipboard();
	QStringList tags;
	for (const QSharedPointer<Tag> tag : m_model->tags())
		tags.append(tag->name());
	clipboard->setText(tags.join(' '));
}
