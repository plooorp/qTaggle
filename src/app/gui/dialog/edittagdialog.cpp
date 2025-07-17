#include "edittagdialog.h"
#include "ui_edittagdialog.h"

EditTagDialog::EditTagDialog(const QList<QSharedPointer<Tag>>& tags, QWidget* parent)
	: QDialog(parent)
	, m_ui(new Ui::EditTagDialog)
	, m_tags(QList(tags))
{
	m_ui->setupUi(this);

	if (m_tags.size() == 1)
	{
		m_ui->description_groupBox->setCheckable(false);
		m_ui->urls_groupBox->setCheckable(false);

		m_ui->name_lineEdit->setText(m_tags.first()->name());
		m_ui->description_plainTextEdit->setPlainText(m_tags.first()->description());
		m_ui->urls_plainTextListEdit->setValues(m_tags.first()->urls());
		setWindowTitle("Edit " + m_tags.first()->name());
	}
	else
	{
		m_ui->name_lineEdit->setDisabled(true);
		QStringList names;
		for (const QSharedPointer<Tag>& tag : tags)
			names.append(tag->name());
		QString names_str = names.join(", ");
		m_ui->name_lineEdit->setText(names_str);
		m_ui->description_groupBox->setCheckable(true);
		m_ui->description_groupBox->setChecked(false);
		m_ui->urls_groupBox->setCheckable(true);
		m_ui->urls_groupBox->setChecked(false);

		if (names.size() <= 3)
			setWindowTitle(QString("Edit " + names_str));
		else
			setWindowTitle(QString("Edit " + QString::number(m_tags.size()) + " tags"));
	}
}

EditTagDialog::~EditTagDialog()
{
	delete m_ui;
}

void EditTagDialog::accept()
{
	if (m_tags.size() == 1)
		updateSingleTag();
	else
		updateTag();
	QDialog::accept();
}

int EditTagDialog::updateSingleTag()
{
	db->begin();
	sqlite3_stmt* stmt;
	std::string sql = "UPDATE tag SET name = ?, description = ?, urls = ? WHERE id = ?;";
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	QByteArray name = m_ui->name_lineEdit->text().toUtf8();
	QByteArray description = m_ui->description_plainTextEdit->toPlainText().toUtf8();
	QByteArray urls = m_ui->urls_plainTextListEdit->values().join(';').toUtf8();
	sqlite3_bind_text(stmt, 1, name.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, description.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, urls.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 4, m_tags.first()->id());
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
	{
		db->rollback();
		return rc;
	}
	db->commit();
	m_tags.first()->fetch();
	return rc;
}

int EditTagDialog::updateTag()
{
	int rc = 0;
	db->begin();
	sqlite3_stmt* stmt;
	std::string sql = "UPDATE tag SET description = ?, urls = ? WHERE id = ?;";
	for (const QSharedPointer<Tag>& tag : m_tags)
	{
		sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
		QByteArray description = m_ui->description_plainTextEdit->toPlainText().toUtf8();
		QByteArray urls = m_ui->urls_plainTextListEdit->values().join(';').toUtf8();
		sqlite3_bind_text(stmt, 1, description.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, urls.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_int64(stmt, 3, tag->id());
		rc = sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		if (rc != SQLITE_DONE)
		{
			db->rollback();
			return rc;
		}
	}
	db->commit();
	for (QSharedPointer<Tag>& tag : m_tags)
		tag->fetch();
	return rc;
}