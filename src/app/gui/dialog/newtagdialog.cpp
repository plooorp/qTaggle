#include "newtagdialog.h"
#include "ui_newtagdialog.h"

#include <QMessageBox>

#include "app/tag.h"

NewTagDialog::NewTagDialog(QWidget *parent, Qt::WindowFlags f)
	: QDialog(parent, f)
	, m_ui(new Ui::NewTagDialog)
{
	m_ui->setupUi(this);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &NewTagDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &NewTagDialog::reject);
}

NewTagDialog::~NewTagDialog()
{
	delete m_ui;
}

void NewTagDialog::accept()
{
	db->begin();
	DBError error;
	if (error = Tag::create(m_ui->name->text(), m_ui->description->toPlainText(), m_ui->urls->values()))
		goto error;
	db->commit();
	return QDialog::accept();

error:
	db->rollback();
	QMessageBox::warning(this, tr("Failed to create tag"), error.message);
}
