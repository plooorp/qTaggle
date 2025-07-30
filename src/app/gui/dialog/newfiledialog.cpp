#include "newfiledialog.h"
#include "ui_newfiledialog.h"

#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include "app/file.h"

NewFileDialog::NewFileDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
	, m_ui(new Ui::NewFileDialog)
{
	m_ui->setupUi(this);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &NewFileDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &NewFileDialog::reject);

	connect(m_ui->chooseFile_pushButton, &QPushButton::pressed, this, &NewFileDialog::openFileDialog_file);
	connect(m_ui->chooseDir_pushButton, &QPushButton::pressed, this, &NewFileDialog::openFileDialog_dir);
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
			m_ui->path_lineEdit->setText(files.first());
		else
		{
			//m_ui->pathLineEdit->clear();
			m_ui->pathList_plainTextEdit->setValues(files);
		}
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
		m_ui->path_lineEdit->setText(dir);
		//QStringList fileNames;
		//for (QString path : m_selectedFiles)
		//	fileNames.append(QFileInfo(path).fileName());
		//m_ui->name_lineEdit->setPlaceholderText(fileNames.join(", "));
	}
}

void NewFileDialog::create(const QDir& directory, bool recursive, bool ignoreHidden)
{
	QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
	if (!ignoreHidden)
		filters |= QDir::Hidden;
	for (QFileInfo entry : directory.entryInfoList(filters, QDir::Name | QDir::DirsFirst))
	{
		if (entry.isDir() && recursive)
			create(QDir(entry.absoluteFilePath()), recursive, ignoreHidden);
		else
		{
			QSharedPointer<File> file = File::create(entry.absoluteFilePath(), m_ui->name_lineEdit->text()
				, m_ui->comment_plainTextEdit->toPlainText(), m_ui->source_lineEdit->text());
			if (file)
				for (const QSharedPointer<Tag>& tag : m_ui->tagSelect->tags())
					file->addTag(tag);
		}
	}
}

void NewFileDialog::accept()
{
	QStringList paths = m_ui->pathList_plainTextEdit->values();
	if (!m_ui->path_lineEdit->text().trimmed().isEmpty())
		paths.insert(0, m_ui->path_lineEdit->text());

	for (QString path : paths)
	{
		QFileInfo pathInfo(path);
		if (pathInfo.isFile())
		{
			// single file
			QSharedPointer<File> file = File::create(pathInfo.absoluteFilePath(), m_ui->name_lineEdit->text()
				, m_ui->comment_plainTextEdit->toPlainText(), m_ui->source_lineEdit->text());
			if (file)
				for (const QSharedPointer<Tag>& tag : m_ui->tagSelect->tags())
					file->addTag(tag);
			
		}
		else if (pathInfo.isDir())
			create(QDir(path), m_ui->recursive_checkBox->isChecked(), m_ui->ignoreHidden_checkBox->isChecked());
	}
	QDialog::accept();
}

void NewFileDialog::reject()
{
	QDialog::reject();
}

