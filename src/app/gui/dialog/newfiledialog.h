#pragma once

#include <QDialog>
#include <QDir>

namespace Ui
{
	class NewFileDialog;
}

class NewFileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewFileDialog(QWidget* parent = nullptr, Qt::WindowFlags = { 0 });
	virtual ~NewFileDialog() override;

private slots:
	void accept() override;
	void openFileDialog_file();
	void openFileDialog_dir();

private:
	Ui::NewFileDialog* m_ui;
	static void walkDirectory(const QDir& dir, QDir::Filters filters, bool recursive, QStringList& paths);
};
