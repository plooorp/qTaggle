#include "edittagdialog.h"
#include "ui_edittagdialog.h"

#include <QMessageBox>

EditTagDialog::EditTagDialog(const QSharedPointer<Tag>& tag, QWidget* parent)
	: QDialog(parent)
	, m_ui(new Ui::EditTagDialog)
	, m_tag(tag)
{
	m_ui->setupUi(this);

	setWindowTitle(tr("Editing %1").arg(m_tag->name()));

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &EditTagDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &EditTagDialog::reject);

	m_ui->name->setText(m_tag->name());
	m_ui->description->setPlainText(m_tag->description());
	m_ui->urls->setValues(m_tag->urls());
}

EditTagDialog::~EditTagDialog()
{
	delete m_ui;
}

void EditTagDialog::accept()
{
	db->begin();
	DBError error;
	if (m_ui->name->text() != m_tag->name())
		if (error = m_tag->setName(m_ui->name->text()))
			goto error;
	if (m_ui->description->toPlainText() != m_tag->description())
		if (error = m_tag->setDescription(m_ui->description->toPlainText()))
			goto error;
	if (m_ui->urls->values() != m_tag->urls())
		if (error = m_tag->setURLs(m_ui->urls->values()))
			goto error;
	db->commit();
	return QDialog::accept();

error:
	db->rollback();
	QMessageBox::warning(this, qApp->applicationName()
		, tr("Failed to update tag: ") + error.message);
}
