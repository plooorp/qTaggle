#include "taglist.h"
#include "ui_taglist.h"

#include <QLocale>
#include <QStandardItem>
#include <QHeaderView>
#include <QMenu>
#include <QApplication>
#include <QVBoxLayout>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QMessageBox>
#include "app/gui/dialog/edittagdialog.h"
#include "app/gui/dialog/newtagdialog.h"

TagList::TagList(QWidget* parent)
	: QWidget(parent)
	, m_ui(new Ui::TagList)
	, m_model(new TagTableModel(this))
{
	m_ui->setupUi(this);

	// context menu actions
	connect(m_ui->actionCreate, &QAction::triggered, this, &TagList::actionCreate_triggered);
	connect(m_ui->actionEdit, &QAction::triggered, this, &TagList::actionEdit_triggered);
	connect(m_ui->actionDelete, &QAction::triggered, this, &TagList::actionDelete_triggered);

	connect(db, &Database::opened, this, &TagList::populate);
	connect(db, &Database::closed, this, &TagList::depopulate);
	connect(m_ui->lineEdit, &QLineEdit::textChanged, this, &TagList::populate);

	m_ui->treeView->setModel(m_model);
	connect(m_ui->treeView, &QTreeView::doubleClicked, this, &TagList::editSelected);
	connect(m_ui->treeView, &QWidget::customContextMenuRequested, this, &TagList::showContextMenu);
	connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]()-> void {emit selectionChanged(); });
}

TagList::~TagList()
{
	delete m_ui;
}

void TagList::populate()
{
	m_model->clear();
	for (QSharedPointer<Tag> tag : Tag::fromQuery(m_ui->lineEdit->text()))
		m_model->addTag(tag);
	//QString DATETIME_SHORTFORMAT = QLocale::system().dateTimeFormat(QLocale::ShortFormat);
	//QString DATETIME_LONGFORMAT = QLocale::system().dateTimeFormat(QLocale::LongFormat);
	//m_tags.clear();
	//m_model->clear();
	//for (QSharedPointer<Tag> tag : Tag::fromQuery(m_lineEdit->text()))
	//{
	//	QStandardItem* id = new QStandardItem(QString::number(tag->id()));
	//	id->setEditable(false);
	//	QStandardItem* name = new QStandardItem(tag->name());
	//	name->setToolTip(tag->name());
	//	name->setEditable(false);
	//	QStandardItem* description = new QStandardItem(tag->description());
	//	description->setToolTip(tag->description());
	//	description->setEditable(false);
	//	QString urls_str = tag->urls().join(';');
	//	QStandardItem* urls = new QStandardItem(urls_str);
	//	urls->setToolTip(urls_str);
	//	urls->setEditable(false);
	//	QStandardItem* degree = new QStandardItem(QString::number(tag->degree()));
	//	degree->setToolTip(QString::number(tag->degree()));
	//	degree->setEditable(false);
	//	QStandardItem* modified = new QStandardItem(tag->modified().toString(DATETIME_SHORTFORMAT));
	//	modified->setToolTip(tag->modified().toString(DATETIME_LONGFORMAT));
	//	modified->setEditable(false);
	//	QStandardItem* created = new QStandardItem(tag->created().toString(DATETIME_SHORTFORMAT));
	//	created->setToolTip(tag->created().toString(DATETIME_LONGFORMAT));
	//	created->setEditable(false);

	//	m_model->appendRow(QList<QStandardItem*>{id, name, description, urls, degree, modified, created});
	//	m_tags.append(tag);
	//}
}

void TagList::depopulate()
{
	m_model->clear();
}

QList<QSharedPointer<Tag>> TagList::selectedTags() const
{
	QList<QSharedPointer<Tag>> tags;
	for (const QModelIndex& index : m_ui->treeView->selectionModel()->selectedRows())
		tags.append(m_model->tagAt(index.row()));
	return tags;
}

void TagList::editSelected()
{
	QList<QSharedPointer<Tag>> tags = selectedTags();
	if (tags.isEmpty())
		return;
	EditTagDialog* dialog = new EditTagDialog(tags, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}

void TagList::deleteSelected()
{
	QList<QSharedPointer<Tag>> tags = selectedTags();
	if (tags.isEmpty())
		return;
	int btn = QMessageBox::question(this, qApp->applicationName(), tr("Are you sure you want to delete the selected tag(s)?"));
	if (btn == QMessageBox::Yes)
		Tag::remove(tags);
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
