#pragma once

#include "ui_mainwindow.h"

#include <QMainWindow>
#include <QPointer>
#include <QCloseEvent>

#include "app/gui/dialog/settingsdialog.h"

class MainWindow final : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();
	enum Tab { File, Tag };

signals:
	void currentTabChanged(Tab index);

private slots:
	void actionAddFile_triggered();
	void actionCreateTag_triggered();
	void actionEditSelected_triggered();
	void actionDeleteSelected_triggered();
	void actionNewDatabase_triggered();
	void actionOpenDatabase_triggered();
	void actionCloseDatabase_triggered();
	void actionOptions_triggered();
	void lockUi();
	void unlockUi();
	void updateRecentlyOpened();

private:
	Ui::MainWindow m_ui;
	QPointer<SettingsDialog> m_settingsDialog;
	void closeEvent(QCloseEvent* event) override;
	void readSettings();
	void writeSettings();
	void openDatabase(const QString& path);
};
