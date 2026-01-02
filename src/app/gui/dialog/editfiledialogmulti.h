#pragma once

#include <QDialog>

#include "app/file.h"

namespace Ui
{
	class EditFileDialogMulti;
}

class EditFileDialogMulti : public QDialog
{
	Q_OBJECT

public:
	explicit EditFileDialogMulti(const QList<File>& files, QWidget* parent);
	virtual ~EditFileDialogMulti() override;

private slots:
	void accept() override;
	void openFileDialog();

private:
	Ui::EditFileDialogMulti* m_ui;
	QList<File> m_files;
};
