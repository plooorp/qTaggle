#include "editfiledialog.h"
#include "ui_editfiledialog.h"

#include <QFileDialog>
#include <QMessageBox>

EditFileDialog::EditFileDialog(QSharedPointer<File> file, QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
	, m_ui(new Ui::EditFileDialog)
	, m_file(file)
{
	m_ui->setupUi(this);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &EditFileDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &EditFileDialog::reject);

	connect(m_ui->btnChooseFile, &QPushButton::clicked, this, &EditFileDialog::openFileDialog_dir);
	connect(m_ui->btnChooseDir, &QPushButton::clicked, this, &EditFileDialog::openFileDialog_dir);

	m_ui->lineEdit_alias->setText(m_file->alias());
	m_ui->lineEdit_fileName->setText(QFileInfo(file->path()).fileName());
	m_ui->lineEdit_dir->setText(QFileInfo(file->path()).dir().path());
	m_ui->lineEdit_source->setText(m_file->source());
	m_ui->plainTextEdit_comment->setPlainText(m_file->comment());
	
	QList<QSharedPointer<Tag>> tags;
	for (const FileTag& ft : file->tags())
		tags.append(ft.tag());
	m_ui->tagSelect->setTags(tags);
}

EditFileDialog::~EditFileDialog()
{
	delete m_ui;
}

void EditFileDialog::accept()
{
	QList<QSharedPointer<Tag>> existingTags;
	for (const FileTag& ft : m_file->tags())
		existingTags.append(ft.tag());
	QList<QSharedPointer<Tag>> newTags = m_ui->tagSelect->tags();

	DBResult error;
	db->begin();

	QFileInfo newPath(QDir(m_ui->lineEdit_dir->text()), m_ui->lineEdit_fileName->text());

	if (newPath.absoluteFilePath() != m_file->path())
	{
		if (error = m_file->setPath(newPath.absoluteFilePath()))
			goto error;
		m_file->check();
	}
	if (m_ui->lineEdit_alias->text() != m_file->alias())
		if (error = m_file->setAlias(m_ui->lineEdit_alias->text()))
			goto error;
	if (m_ui->plainTextEdit_comment->toPlainText() != m_file->comment())
		if (error = m_file->setComment(m_ui->plainTextEdit_comment->toPlainText()))
			goto error;
	if (m_ui->lineEdit_source->text() != m_file->source())
		if (error = m_file->setSource(m_ui->lineEdit_source->text()))
			goto error;

	for (const QSharedPointer<Tag>& tag : newTags)
		if (!existingTags.contains(tag))
			if (error = m_file->addTag(tag))
				goto error;
	for (const QSharedPointer<Tag>& tag : existingTags)
		if (!newTags.contains(tag))
			if (error = m_file->removeTag(tag))
				goto error;
	db->commit();
	return QDialog::accept();

error:
	db->rollback();
	QMessageBox::warning(this, qApp->applicationName()
		, tr("Failed to update file: ") + error.msg);
}

void EditFileDialog::keyPressEvent(QKeyEvent* evt)
{
	// prevent hitting enter on tag select from submitting the dialog
	if (evt->key() == Qt::Key_Enter || evt->key() == Qt::Key_Return)
		return;
	QDialog::keyPressEvent(evt);
}

void EditFileDialog::openFileDialog_file()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setFileMode(QFileDialog::ExistingFile);
	if (dialog.exec())
	{
		QFileInfo fileInfo(dialog.selectedFiles().first());
		m_ui->lineEdit_fileName->setText(fileInfo.fileName());
		m_ui->lineEdit_dir->setText(fileInfo.dir().path());
	}
}

void EditFileDialog::openFileDialog_dir()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setFileMode(QFileDialog::Directory);
	if (dialog.exec())
	{
		QString dir(dialog.selectedFiles().first());
		m_ui->lineEdit_dir->setText(dir);
	}
}
