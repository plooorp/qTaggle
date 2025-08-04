#include "editfiledialogmulti.h"
#include "ui_editfiledialogmulti.h"

#include <QFileDialog>
#include <QMessageBox>

EditFileDialogMulti::EditFileDialogMulti(const QList<QSharedPointer<File>>& files, QWidget* parent)
	: QDialog(parent)
	, m_ui(new Ui::EditFileDialogMulti)
	, m_files(files)
{
	m_ui->setupUi(this);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &EditFileDialogMulti::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &EditFileDialogMulti::reject);

	connect(m_ui->aliasCheckBox, &QCheckBox::checkStateChanged, this
		, [this]() -> void { m_ui->alias->setEnabled(m_ui->aliasCheckBox->isChecked()); });
	connect(m_ui->directoryCheckBox, &QCheckBox::checkStateChanged, this
		, [this]() -> void
		{
			bool checked = m_ui->directoryCheckBox->isChecked();
			m_ui->directory->setEnabled(checked);
			m_ui->btnChooseDir->setEnabled(checked);
		});
	connect(m_ui->btnChooseDir, &QPushButton::clicked, this, &EditFileDialogMulti::openFileDialog);
	connect(m_ui->sourceCheckBox, &QCheckBox::checkStateChanged, this
		, [this]() -> void { m_ui->source->setEnabled(m_ui->sourceCheckBox->isChecked()); });
	connect(m_ui->commentCheckBox, &QCheckBox::checkStateChanged, this
		, [this]() -> void { m_ui->comment->setEnabled(m_ui->commentCheckBox->isChecked()); });
}

EditFileDialogMulti::~EditFileDialogMulti()
{
	delete m_ui;
}

void EditFileDialogMulti::accept()
{
	db->begin();
	DBResult error;
	if (m_ui->directoryCheckBox->isChecked())
	{
		for (QSharedPointer<File>& file : m_files)
		{
			QString dir = m_ui->directory->text();
			QString fileName = QFileInfo(file->path()).fileName();
			if (error = file->setPath(QFileInfo(dir, fileName).absoluteFilePath()))
				goto error;
			file->check();
		}
	}
	if (m_ui->aliasCheckBox->isChecked())
		for (QSharedPointer<File>& file : m_files)
			if (error = file->setAlias(m_ui->alias->text()))
				goto error;
	if (m_ui->sourceCheckBox->isChecked())
		for (QSharedPointer<File>& file : m_files)
			if (error = file->setSource(m_ui->source->text()))
				goto error;
	if (m_ui->commentCheckBox->isChecked())
		for (QSharedPointer<File>& file : m_files)
			if (error = file->setComment(m_ui->comment->toPlainText()))
				goto error;
	for (const QSharedPointer<Tag> tag : m_ui->tagSelectRemove->tags())
		for (QSharedPointer<File>& file : m_files)
			if (error = file->removeTag(tag))
				goto error;
	for (const QSharedPointer<Tag> tag : m_ui->tagSelectAdd->tags())
		for (QSharedPointer<File>& file : m_files)
			if (error = file->addTag(tag))
				goto error;
	db->commit();
	return QDialog::accept();

error:
	db->rollback();
	QMessageBox::warning(this, qApp->applicationName()
		, tr("Failed to update files: ") + error.msg);
}

void EditFileDialogMulti::openFileDialog()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setFileMode(QFileDialog::Directory);
	if (dialog.exec())
	{
		QString dir(dialog.selectedFiles().first());
		m_ui->directory->setText(dir);
	}
}
