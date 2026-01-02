#pragma once

#include <QDialog>

namespace Ui
{
	class NewTagDialog;
}

class NewTagDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewTagDialog(QWidget *parent = nullptr, Qt::WindowFlags f = { 0 });
	virtual ~NewTagDialog() override;

private slots:
	void accept() override;

private:
	Ui::NewTagDialog* m_ui;
};
