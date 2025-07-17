#pragma once

#include <QDialog>

#include "app/file.h"

namespace Ui
{
	class EditFileDialog;
}

class EditFileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EditFileDialog(QSharedPointer<File> file, QWidget* parent = nullptr, Qt::WindowFlags f = { 0 });
	~EditFileDialog();

private slots:
	void accept() override;
	void reject() override;

private:
	Ui::EditFileDialog* m_ui;
	QSharedPointer<File> m_file;
	void keyPressEvent(QKeyEvent* event) override;
};