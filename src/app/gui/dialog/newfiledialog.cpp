#include "newfiledialog.h"
#include "ui_newfiledialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include "app/file.h"
#include "app/globals.h"

NewFileDialog::NewFileDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
	, m_ui(new Ui::NewFileDialog)
{
	m_ui->setupUi(this);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &NewFileDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &NewFileDialog::reject);

	connect(m_ui->chooseFile_pushButton, &QPushButton::clicked, this, &NewFileDialog::openFileDialog_file);
	connect(m_ui->chooseDir_pushButton, &QPushButton::clicked, this, &NewFileDialog::openFileDialog_dir);
}

NewFileDialog::~NewFileDialog()
{
	delete m_ui;
}

void NewFileDialog::openFileDialog_file()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	if (dialog.exec())
	{
		QStringList files = dialog.selectedFiles();
		if (files.size() == 1)
			m_ui->path->setText(files.first());
		else
			m_ui->paths->setValues(files);
	}
}

void NewFileDialog::openFileDialog_dir()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setFileMode(QFileDialog::Directory);
	if (dialog.exec())
	{
		QString dir = dialog.selectedFiles().first();
		m_ui->path->setText(dir);
	}
}

void NewFileDialog::accept()
{
	QStringList paths = m_ui->paths->values();
	if (!m_ui->path->text().trimmed().isEmpty())
		paths.insert(0, m_ui->path->text());

	QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
	if (!m_ui->ignoreHidden->isChecked())
		filters |= QDir::Hidden;
	QStringList filePathList;
	for (const QString& path : paths)
	{
		if (QFileInfo(path).isFile())
			filePathList.append(path);
		else
			walkDirectory(path, filters, m_ui->recursive_checkBox->isChecked(), filePathList);
	}

	if (filePathList.size() > 128)
	{
		QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Confirm action")
			, tr("This action will import %1 files, and may take some time. Continue?").arg(QLocale().toString(filePathList.size())));
		if (btn != QMessageBox::Yes)
			return;
	}

	QProgressDialog progress(tr("Adding files..."), tr("Abort"), 0, filePathList.size(), this);
	progress.setWindowModality(Qt::ApplicationModal);
		
	db->begin();
	DBError error;
	for (int i = 0; i < filePathList.size(); ++i)
	{
		QString& filePath = filePathList[i];
		progress.setValue(i);
		progress.setLabelText(tr("Adding file: %1").arg(
			filePath.size() > 32
				? filePath.first(8) + u"..."_s + filePath.last(24)
				: filePath
		));
		if (progress.wasCanceled())
			goto error;
		File file;
		if (error = File::create(filePath, m_ui->alias->text(), m_ui->comment->toPlainText(), m_ui->source->text(), &file))
			continue;
		if (error = file.setTags(m_ui->tagSelect->tags()))
			continue;
	}
	progress.setValue(filePathList.size());
	db->commit();
	return QDialog::accept();

error:
	db->rollback();
	QMessageBox::warning(this, tr("Failed to add files"), error.message());
}

void NewFileDialog::walkDirectory(const QDir& dir, QDir::Filters filters, bool recursive, QStringList& paths)
{
	for (QFileInfo entry : dir.entryInfoList(filters, QDir::Name | QDir::DirsFirst))
	{
		if (entry.isFile())
			paths.append(entry.absoluteFilePath());
		else if (recursive)
			walkDirectory(QDir(entry.absoluteFilePath()), filters, recursive, paths);
	}
}
