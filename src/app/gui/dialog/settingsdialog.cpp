#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSettings>

#include "app/database.h"

SettingsDialog::SettingsDialog(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
	, m_ui(new Ui::SettingsDialog)
{
	m_ui->setupUi(this);

	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

	if (!db->isOpen())
		m_ui->tabWidget->setTabEnabled(0, false);
}

SettingsDialog::~SettingsDialog()
{
	delete m_ui;
}

void SettingsDialog::accept()
{
	//QSettings settings(db->path)
	//if (app->isOpen())
	//{
	//	app->watchedDirectories()
	//}
	QDialog::accept();
}

void SettingsDialog::reject()
{
	QDialog::reject();
}