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
	explicit EditFileDialog(File file, QWidget* parent = nullptr);
	virtual ~EditFileDialog() override;

private slots:
	void accept() override;
	void openFileDialog_file();
	void openFileDialog_dir();

private:
	Ui::EditFileDialog* m_ui;
	File m_file;
	void keyPressEvent(QKeyEvent* event) override;
};
