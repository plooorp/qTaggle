#include "editfiledialog.h"
#include "ui_editfiledialog.h"

#include <QMessageBox>

EditFileDialog::EditFileDialog(QSharedPointer<File> file, QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
	, m_ui(new Ui::EditFileDialog)
	, m_file(file)
{
	m_ui->setupUi(this);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &EditFileDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &EditFileDialog::reject);

	m_ui->lineEdit_name->setText(m_file->name());
	m_ui->lineEdit_path->setText(m_file->path());
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

	if (m_ui->lineEdit_name->text() != m_file->name())
		if (error = m_file->setName(m_ui->lineEdit_name->text()))
			goto error;
	if (m_ui->lineEdit_path->text() != m_file->path())
		if (error = m_file->setPath(m_ui->lineEdit_path->text()))
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
	m_file->fetch();
	QDialog::accept();
	return;

error:
	db->rollback();
	QMessageBox::warning(this, qApp->applicationName(), tr("Unable to update file: ") + error.msg);
}

void EditFileDialog::reject()
{
	QDialog::reject();
}

void EditFileDialog::keyPressEvent(QKeyEvent* evt)
{
	// prevent hitting enter on tag select from submitting the dialog
	if (evt->key() == Qt::Key_Enter || evt->key() == Qt::Key_Return)
		return;
	QDialog::keyPressEvent(evt);
}