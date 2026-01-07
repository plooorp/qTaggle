#include "filelist.h"
#include "ui_filelist.h"

#include <QVBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QCompleter>
#include <QLabel>
#include <QTreeWidget>
#include <QProgressDialog>
#include <QDesktopServices>

#include "app/database.h"
#include "app/file.h"
#include "app/gui/dialog/newfiledialog.h"
#include "app/gui/dialog/editfiledialog.h"
#include "app/gui/dialog/editfiledialogmulti.h"
#include "app/gui/helper/taglineedit.h"
#include "app/globals.h"

FileList::FileList(QWidget* parent)
	: QWidget(parent)
	, m_ui(new Ui::FileList)
	, m_model(new FileTableModel(this))
{
	m_ui->setupUi(this);
	
	// context menu actions
	connect(m_ui->actionAdd, &QAction::triggered, this, &FileList::actionAdd_triggered);
	connect(db, &Database::opened, this, [this]() -> void { m_ui->actionAdd->setEnabled(true); });
	connect(db, &Database::closed, this, [this]() -> void { m_ui->actionAdd->setEnabled(false); });
	connect(m_ui->actionEdit, &QAction::triggered, this, &FileList::actionEdit_triggered);
	connect(m_ui->actionOpen, &QAction::triggered, this, &FileList::actionOpen_triggered);
	connect(m_ui->actionCheck, &QAction::triggered, this, &FileList::actionCheck_triggered);
	connect(m_ui->actionDelete, &QAction::triggered, this, &FileList::actionDelete_triggered);

	// hamburger menu
	m_ui->menuButton->setIcon(QIcon(":/icons/menu.svg"));
	QMenu* menu = new QMenu(this);
	menu->addAction(m_ui->actionSortByRelevancy);
	m_ui->menuButton->setMenu(menu);

	QHeaderView* header = m_ui->treeView->header();
	header->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(header, &QHeaderView::customContextMenuRequested, this, &FileList::showHeaderContextMenu);

	m_ui->treeView->setModel(m_model);
	connect(m_ui->treeView, &QTreeView::doubleClicked, this, &FileList::editSelected);
	connect(m_ui->treeView, &QWidget::customContextMenuRequested, this, &FileList::showTableContextMenu);
	connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]()-> void {emit selectionChanged(selectedFiles()); });

	m_ui->sortBy->addItem(tr("Name"), u"displayName"_s);
	m_ui->sortBy->addItem(tr("Path"), u"file.path"_s);
	m_ui->sortBy->addItem(tr("State"), u"file.state"_s);
	m_ui->sortBy->addItem(tr("Date created"), u"file.created"_s);
	m_ui->sortBy->addItem(tr("Last modified"), u"file.modified"_s);
	m_ui->sortBy->addItem(tr("Last checked"), u"file.checked"_s);

	m_ui->sortOrder->addItem(tr("Ascending"), u"ASC"_s);
	m_ui->sortOrder->addItem(tr("Descending"), u"DESC"_s);

	connect(db, &Database::opened, this, &FileList::populate);
	connect(db, &Database::updated, this, &FileList::populate);
	connect(m_ui->nameLineEdit, &QLineEdit::textChanged, this, &FileList::populate);
	connect(m_ui->tagLineEdit, &QLineEdit::textChanged, this, &FileList::populate);
	connect(m_ui->paginator, &Paginator::pageChangedByUser, this, &FileList::populate);
	connect(m_ui->resultsPerPage, &QSpinBox::editingFinished, this, &FileList::populate);
	connect(m_ui->sortBy, &QComboBox::currentIndexChanged, this, &FileList::populate);
	connect(m_ui->sortOrder, &QComboBox::currentIndexChanged, this, &FileList::populate);
	connect(m_ui->actionSortByRelevancy, &QAction::toggled, this, &FileList::populate);

	connect(m_ui->clearQuery, &QToolButton::clicked, this, &FileList::clearQuery);

	readSettings();
}

FileList::~FileList()
{
	writeSettings();
	delete m_ui;
}

QList<File> FileList::selectedFiles() const
{
	QList<File> files;
	for (const QModelIndex& index : m_ui->treeView->selectionModel()->selectedRows())
		files.append(m_model->fileAt(index.row()));
	return files;
}

void FileList::editSelected()
{
	QList<File> files = selectedFiles();
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
	QList<File> files = selectedFiles();
	QList<std::tuple<File, CheckError>> checksumErrors;
	
	QProgressDialog progress(tr("Checking files..."), tr("Abort"), 0, files.size(), this);
	progress.setWindowModality(Qt::ApplicationModal);
	for (int i = 0; i < files.size(); ++i)
	{
		File file = files[i];
		progress.setValue(i);
		QString path = file.path();
		progress.setLabelText(tr("Checking file: %1").arg(
			path.size() > 32
			? path.first(8) + u"..."_s + path.last(24)
			: path
		));
		if (progress.wasCanceled())
			break;
		CheckError error = file.check();
		if (!error && file.state() == File::ChecksumChanged)
			checksumErrors.append({ file, error });
	}
	progress.setValue(files.size());

	if (!checksumErrors.isEmpty())
	{
		QDialog* dialog = new QDialog(this);
		dialog->setWindowTitle(tr("Checksum conflict found"));
		QLabel* label = new QLabel(tr("The following files have had their contents changed since they were last checked. Do you want to overwrite their existing checksums?"), dialog);
		label->setWordWrap(true);
		QTreeWidget* table = new QTreeWidget(dialog);
		table->setHeaderLabels({ "Name", "Path", "Old SHA-1", "New SHA-1" });
		for (const std::tuple<File, CheckError>& tuple : checksumErrors)
		{
			File file = std::get<0>(tuple);
			new QTreeWidgetItem(table, { file.name(), file.path(), file.sha1().toHex(), std::get<1>(tuple).sha1.toHex() });
		}
		QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No, Qt::Horizontal, dialog);
		connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
		QVBoxLayout* layout = new QVBoxLayout(dialog);
		layout->addWidget(label);
		layout->addWidget(table);
		layout->addWidget(buttonBox);
		dialog->setLayout(layout);
		if (dialog->exec())
			for (const std::tuple<File, CheckError>& tuple : checksumErrors)
			{
				File file = std::get<0>(tuple);
				file.setSHA1(std::get<1>(tuple).sha1);
				file.setState(File::Ok);
			}
	}
}

void FileList::deleteSelected()
{
	QList<File> files = selectedFiles();
	if (files.isEmpty())
		return;
	QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Confirm action")
		, files.size() > 1
			? tr("Are you sure you want to delete %1 files? This will not delete the files from disk.").arg(files.size())
			: tr("Are you sure you want to delete '%1'? This will not delete the file from disk.").arg(files.first().name())
	);
	if (btn == QMessageBox::Yes)
		for (const File& file : files)
			file.remove();
}

void FileList::populate()
{
	const QString tags = m_ui->tagLineEdit->text().trimmed();
	const QString query = m_ui->nameLineEdit->text().trimmed();
	const int resultsPerPage = m_ui->resultsPerPage->value();
	const int offset = m_ui->paginator->page() * resultsPerPage;
	QString sortBy = m_ui->sortBy->currentData().toString();
	QString sortOrder = m_ui->sortOrder->currentData().toString();

	QString sql, sqlCount;
	if (query.isEmpty())
	{
		sql = uR"(
			SELECT DISTINCT
				file.id,
				CASE
					WHEN LENGTH(file.alias) > 0 THEN file.alias
					ELSE file.name
				END AS displayName
			FROM file
			LEFT JOIN file_tag ON file_tag.file_id = file.id
			WHERE :tags
			ORDER BY %1 %2, file.name ASC
			LIMIT ? OFFSET ?;
		)"_s.arg(sortBy, sortOrder);
		sqlCount = uR"(
			SELECT COUNT(*) FROM file
			LEFT JOIN file_tag ON file_tag.file_id = file.id
			WHERE :tags;
		)"_s;
	}
	else
	{
		if (m_ui->actionSortByRelevancy->isChecked())
		{
			sortBy = "relevancy";
			sortOrder = "ASC";
		}
		sql = uR"(
			SELECT DISTINCT
				file.id,
				CASE
					WHEN LENGTH(file.alias) > 0 THEN file.alias
					ELSE file.name
				END AS displayName,
				bm25(file_search, 15.0, 15.0, 10.0, 5.0) AS relevancy
			FROM file
			INNER JOIN file_search ON file_search.ROWID = file.id
			LEFT JOIN file_tag ON file_tag.file_id = file.id
			WHERE :tags AND file_search MATCH ?
			ORDER BY %1 %2, file.name ASC
			LIMIT ? OFFSET ?;
		)"_s.arg(sortBy, sortOrder);
		sqlCount = uR"(
			SELECT COUNT(*) FROM file
			INNER JOIN file_search ON file_search.ROWID = file.id
			LEFT JOIN file_tag ON file_tag.file_id = file.id
			WHERE :tags AND file_search MATCH ?;
		)"_s;
	}

	QByteArrayList include, exclude;
	QString condition = parseTags(tags, include, exclude);
	sql.replace(":tags", condition);
	sqlCount.replace(":tags", condition);

	QStringList query_parts = query.split(' ', Qt::SkipEmptyParts);
	for (QString& part : query_parts)
		part = part + u"*"_s;
	QByteArray query_bytes = query_parts.join(' ').toUtf8();

	int i = 0;
	sqlite3_stmt* stmt;
	QByteArray sql_bytes = sql.toUtf8();
	sqlite3_prepare_v2(db->con(), sql_bytes.constData(), -1, &stmt, nullptr);
	for (const QByteArray& tag : include)
		sqlite3_bind_text(stmt, ++i, tag.constData(), -1, SQLITE_STATIC);
	for (const QByteArray& tag : exclude)
		sqlite3_bind_text(stmt, ++i, tag.constData(), -1, SQLITE_STATIC);
	if (!query.isEmpty())
		sqlite3_bind_text(stmt, ++i, query_bytes, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, ++i, resultsPerPage);
	sqlite3_bind_int(stmt, ++i, offset);
	m_model->clear();
	while (sqlite3_step(stmt) == SQLITE_ROW)
		m_model->addFile(File(sqlite3_column_int64(stmt, 0)));
	sqlite3_finalize(stmt);
	
	i = 0;
	QByteArray sqlCount_bytes = sqlCount.toUtf8();
	sqlite3_prepare_v2(db->con(), sqlCount_bytes.constData(), -1, &stmt, nullptr);
	for (const QByteArray& tag : include)
		sqlite3_bind_text(stmt, ++i, tag.constData(), -1, SQLITE_STATIC);
	for (const QByteArray& tag : exclude)
		sqlite3_bind_text(stmt, ++i, tag.constData(), -1, SQLITE_STATIC);
	if (!query.isEmpty())
		sqlite3_bind_text(stmt, ++i, query_bytes, -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int64_t count = sqlite3_column_int64(stmt, 0);
		int resultsPerPage = m_ui->resultsPerPage->value();
		int maxPage = std::max(0, static_cast<int>(std::ceil(static_cast<double>(count) / resultsPerPage)) - 1);
		m_ui->paginator->setMaxPage(maxPage);
	}
	sqlite3_finalize(stmt);
}

QString FileList::parseTags(const QString& query, QByteArrayList& include, QByteArrayList& exclude)
{
	if (query.isEmpty())
		return u"1"_s;
	for (const QString& tag : query.split(' ', Qt::SkipEmptyParts))
	{
		if (tag == u"!"_s)
			continue;
		else if (tag.startsWith('!'))
			exclude.append(tag.mid(1).toUtf8());
		else
			include.append(tag.toUtf8());
	}
	if (include.isEmpty() && exclude.isEmpty())
		return u"1"_s;

	QString sql = uR"(
		(file_tag.tag_id IN (
			SELECT id FROM tag :include_cond
			EXCEPT
			SELECT id FROM tag WHERE name IN (:exclude)
		) :null_cond)
	)"_s;
	if (include.isEmpty())
	{
		sql.remove(u":include_cond"_s);
		sql.replace(u":null_cond"_s, u"OR file_tag.tag_id IS NULL"_s);
	}
	else
	{
		QStringList placeholders(include.size(), u"?"_s);
		QString condition = u"WHERE name IN ("_s + placeholders.join(u", "_s) + u")"_s;
		sql.replace(u":include_cond"_s, condition);
		sql.remove(u":null_cond"_s);
	}
	if (exclude.isEmpty())
		sql.remove(u":exclude"_s);
	else
	{
		QString placeholders = QStringList(exclude.size(), u"?"_s).join(u", ");
		sql.replace(u":exclude"_s, placeholders);
	}
	return sql;
}

void FileList::appendToTagQuery(const QString& text)
{
	QString existingText = m_ui->tagLineEdit->text();
	if (existingText.isEmpty())
		m_ui->tagLineEdit->setText(text);
	else if (existingText.last(1) == ' ')
		m_ui->tagLineEdit->setText(existingText + text);
	else
		m_ui->tagLineEdit->setText(existingText + u" "_s + text);
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

void FileList::actionOpen_triggered()
{
	QList<File> files = selectedFiles();
	if (files.size() > 8)
	{
		QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Confirm action")
			, tr("Are you sure you want to open %1 files?").arg(files.size())
		);
		if (btn != QMessageBox::Yes)
			return;
	}
	for (const File& file : files)
		QDesktopServices::openUrl(QUrl::fromLocalFile(file.path()));
}

void FileList::actionDelete_triggered()
{
	deleteSelected();
}

void FileList::actionCheck_triggered()
{
	checkSelected();
}

void FileList::showTableContextMenu(const QPoint& pos)
{
	if (!m_ui->treeView->indexAt(pos).isValid())
		m_ui->treeView->clearSelection();

	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	if (!m_ui->treeView->selectionModel()->selectedRows().isEmpty())
	{
		menu->addAction(m_ui->actionEdit);
		menu->addAction(m_ui->actionOpen);
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

void FileList::showHeaderContextMenu(const QPoint& pos)
{
	QMenu* menu = new QMenu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	for (int column = 0; column < FileTableModel::columnString.size(); ++column)
	{
		QAction* action = new QAction(FileTableModel::columnString[column], menu);
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

void FileList::clearQuery()
{
	m_ui->tagLineEdit->clear();
	m_ui->nameLineEdit->clear();
}

void FileList::readSettings()
{
	QSettings settings;
	const QByteArray headerState = settings.value("GUI/FileList/headerState").toByteArray();
	if (headerState.isEmpty())
		m_ui->treeView->setColumnHidden(FileTableModel::ID, true);
	else
		m_ui->treeView->header()->restoreState(headerState);
	m_ui->resultsPerPage->setValue(settings.value("GUI/FileList/resultsPerPage", 64).toInt());
	m_ui->sortBy->setCurrentIndex(settings.value("GUI/FileList/sortBy", 0).toInt());
	m_ui->sortOrder->setCurrentIndex(settings.value("GUI/FileList/sortOrder", 0).toInt());
	m_ui->actionSortByRelevancy->setChecked(settings.value("GUI/FileList/sortByRelevancy", true).toBool());
}

void FileList::writeSettings()
{
	QSettings settings;
	settings.setValue("GUI/FileList/headerState", m_ui->treeView->header()->saveState());
	settings.setValue("GUI/FileList/resultsPerPage", m_ui->resultsPerPage->value());
	settings.setValue("GUI/FileList/sortBy", m_ui->sortBy->currentIndex());
	settings.setValue("GUI/FileList/sortOrder", m_ui->sortOrder->currentIndex());
	settings.setValue("GUI/FileList/sortByRelevancy", m_ui->actionSortByRelevancy->isChecked());
}
