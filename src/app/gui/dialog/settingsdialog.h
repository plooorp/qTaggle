#pragma once

#include <QDialog>

namespace Ui
{
	class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget* parent = nullptr, Qt::WindowFlags = { 0 });
	virtual ~SettingsDialog() override;

private slots:
	void accept() override;
	void reject() override;

private:
	Ui::SettingsDialog* m_ui;
};
