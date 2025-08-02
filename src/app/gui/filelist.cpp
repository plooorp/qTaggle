#include "filelist.h"
#include "ui_filelist.h"

#include <QVBoxLayout>
#include <QMenu>
#include <QMessageBox>

#include "app/database.h"
#include "app/file.h"
#include "app/gui/dialog/newfiledialog.h"
#include "app/gui/dialog/editfiledialog.h"
#include "app/gui/dialog/editfiledialogmulti.h"

FileList::FileList(QWidget* parent)
	: QWidget(parent)
	, m_ui(new Ui::FileList)
	, m_model(new FileTableModel(this))
{
	m_ui->setupUi(this);
	
	// context menu actions
	connect(m_ui->actionAdd, &QAction::triggered, this, &FileList::actionAdd_triggered);
	connect(m_ui->actionEdit, &QAction::triggered, this, &FileList::actionEdit_triggered);
	connect(m_ui->actionCheck, &QAction::triggered, this, &FileList::actionCheck_triggered);
	connect(m_ui->actionDelete, &QAction::triggered, this, &FileList::actionDelete_triggered);

	connect(db, &Database::opened, this, &FileList::populate);
	connect(db, &Database::closed, this, &FileList::depopulate);
	connect(m_ui->lineEdit, &QLineEdit::textChanged, this, &FileList::populate);

	m_ui->treeView->setModel(m_model);
	connect(m_ui->treeView, &QTreeView::doubleClicked, this, &FileList::editSelected);
	connect(m_ui->treeView, &QWidget::customContextMenuRequested, this, &FileList::showContextMenu);
	connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]()-> void {emit selectionChanged(); });
}

FileList::~FileList()
{
	delete m_ui;
}

QList<QSharedPointer<File>> FileList::selectedFiles() const
{
	QList<QSharedPointer<File>> files;
	for (const QModelIndex& index : m_ui->treeView->selectionModel()->selectedRows())
		files.append(m_model->fileAt(index.row()));
	return files;
}

void FileList::editSelected()
{
	QList<QSharedPointer<File>> files = selectedFiles();
	if (files.isEmpty())
		return;
	QDialog* dialog;
	if (files.size() == 1)
		dialog = new EditFileDialog(files.first(), this);
	else
		dialog = new EditFileDialogMulti(files, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}

void FileList::checkSelected()
{
	for (const QSharedPointer<File>& file : selectedFiles())
		file->check();
}

void FileList::deleteSelected()
{
	QList<QSharedPointer<File>> files = selectedFiles();
	if (files.isEmpty())
		return;
	int btn = QMessageBox::question(this, qApp->applicationName(), tr("Are you sure you want to delete the selected file(s)?"));
	if (btn == QMessageBox::Yes)
		File::remove(files);
}

void FileList::populate()
{
	m_model->clear();
	for (const QSharedPointer<File>& file : File::fromQuery(m_ui->lineEdit->text()))
		m_model->addFile(file);
}

void FileList::depopulate()
{
	m_model->clear();
}

void FileList::actionAdd_triggered()
{
	NewFileDialog* dialog = new NewFileDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
}

void FileList::actionEdit_triggered()
{
	editSelected();
}
void FileList::actionDelete_triggered()
{
	deleteSelected();
}

void FileList::actionCheck_triggered()
{
	checkSelected();
}

void FileList::showContextMenu(const QPoint& pos)
{
	if (!m_ui->treeView->indexAt(pos).isValid())
		m_ui->treeView->clearSelection();

	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	if (!m_ui->treeView->selectionModel()->selectedRows().isEmpty())
	{
		menu->addAction(m_ui->actionEdit);
		menu->addAction(m_ui->actionCheck);
		menu->addAction(m_ui->actionDelete);
		menu->addSeparator();
		menu->addAction(m_ui->actionAdd);
	}
	else
	{
		menu->addAction(m_ui->actionAdd);
	}
	menu->popup(QCursor::pos());
}
