#pragma once

#include <QWidget>
#include <QtWidgets/QDialog>
#include <QPointer>
#include <QtWidgets/QFileDialog>

namespace Ui
{
	class NewFileDialog;
}

class NewFileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewFileDialog(QWidget* parent = nullptr, Qt::WindowFlags = { 0 });
	~NewFileDialog();

private slots:
	void accept() override;
	void reject() override;
	void openFileDialog_file();
	void openFileDialog_dir();

private:
	Ui::NewFileDialog* m_ui;
	void create(const QDir& directory, bool recursive, bool ignoreHidden);
};
