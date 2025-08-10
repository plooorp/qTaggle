#include "taglist.h"
#include "ui_taglist.h"

#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include "app/gui/dialog/edittagdialog.h"
#include "app/gui/dialog/edittagdialogmulti.h"
#include "app/gui/dialog/newtagdialog.h"

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

	connect(db, &Database::opened, this, &TagList::populate);
	connect(db, &Database::closed, this, &TagList::depopulate);
	connect(m_ui->lineEdit, &QLineEdit::textChanged, this, &TagList::populate);

	m_ui->treeView->setModel(m_model);
	connect(m_ui->treeView, &QTreeView::doubleClicked, this, &TagList::editSelected);
	connect(m_ui->treeView, &QWidget::customContextMenuRequested, this, &TagList::showContextMenu);
	connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]()-> void {emit selectionChanged(); });

	readSettings();
}

TagList::~TagList()
{
	writeSettings();
	delete m_ui;
}

void TagList::populate()
{
	m_model->clear();
	for (QSharedPointer<Tag> tag : Tag::fromQuery(m_ui->lineEdit->text()))
		m_model->addTag(tag);
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
	QList<QSharedPointer<Tag>> tags = selectedTags();
	if (tags.isEmpty())
		return;
	QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Confirm action")
		, tags.size() > 1
			? tr("Are you sure you want to delete %1 tags?").arg(tags.size())
			: tr("Are you sure you want to delete '%1'?").arg(tags.first()->name())
	);
	if (btn == QMessageBox::Yes)
		for (QSharedPointer<Tag>& tag : tags)
			tag->remove();
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

void TagList::readSettings()
{
	QSettings settings;
	const QByteArray headerState = settings.value("GUI/TagList/headerState", QByteArray()).toByteArray();
	if (!headerState.isEmpty())
		m_ui->treeView->header()->restoreState(headerState);
}

void TagList::writeSettings()
{
	QSettings settings;
	settings.setValue("GUI/TagList/headerState", m_ui->treeView->header()->saveState());
}
