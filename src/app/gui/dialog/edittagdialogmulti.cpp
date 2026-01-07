#include "edittagdialogmulti.h"
#include "ui_edittagdialogmulti.h"

#include <QMessageBox>

EditTagDialogMulti::EditTagDialogMulti(const QList<Tag>& tags, QWidget* parent)
	: QDialog(parent)
	, m_ui(new Ui::EditTagDialogMulti)
	, m_tags(tags)
{
	Q_ASSERT(tags.size() > 0);
	m_ui->setupUi(this);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &EditTagDialogMulti::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &EditTagDialogMulti::reject);

	if (m_tags.size() <= 3)
	{
		QString title = tr("Editing") + m_tags.first().name();
		for (int i = 1; i < m_tags.size(); i++)
			title += u", "_s + m_tags[i].name();
	}
	else
		setWindowTitle(tr("Editing ") + QString::number(m_tags.size()) + tr(" tags"));
}

EditTagDialogMulti::~EditTagDialogMulti()
{
	delete m_ui;
}

void EditTagDialogMulti::accept()
{
	db->begin();
	DBError error;
	if (m_ui->descriptionGroup->isChecked())
		for (const Tag& tag : m_tags)
			if (error = tag.setDescription(m_ui->description->toPlainText()))
				goto error;
	if (m_ui->urlsGroup->isChecked())
		for (const Tag& tag : m_tags)
			if (error = tag.setURLs(m_ui->urls->values()))
				goto error;
	db->commit();
	return QDialog::accept();

error:
	db->rollback();
	QMessageBox::warning(this, qApp->applicationName()
		, tr("Failed to update tags: ") + error.message());
}
