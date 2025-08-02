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
	explicit EditFileDialogMulti(QList<QSharedPointer<File>>& files, QWidget* parent);
	~EditFileDialogMulti();

private slots:
	void accept() override;
	void openFileDialog();

private:
	Ui::EditFileDialogMulti* m_ui;
	QList<QSharedPointer<File>> m_files;
};
