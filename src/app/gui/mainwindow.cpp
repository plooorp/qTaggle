#include "mainwindow.h"

#include <QAction>
#include <QFileDialog>
#include <QSettings>
#include <QApplication>
#include <QMessageBox>

#include "app/database.h"
//#include "app/database.test.h"
#include "app/gui/dialog/edittagdialog.h"
#include "app/gui/dialog/newtagdialog.h"
#include "app/gui/dialog/newfiledialog.h"
#include "app/gui/docked/properties.h"
#include "app/gui/docked/filepreview.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	m_ui.setupUi(this);

	m_ui.filePreviewDock->setWidget(new FilePreview(m_ui.fileList, this));
	m_ui.propertiesDock->setWidget(new Properties(this, m_ui.fileList, m_ui.tagList, this));

	// file
	connect(m_ui.actionAddFile, &QAction::triggered, this, &MainWindow::actionAddFile_triggered);
	connect(m_ui.actionCreateTag, &QAction::triggered, this, &MainWindow::actionCreateTag_triggered);
	connect(m_ui.actionNewDatabase, &QAction::triggered, this, &MainWindow::actionNewDatabase_triggered);
	connect(m_ui.actionOpenDatabase, &QAction::triggered, this, &MainWindow::actionOpenDatabase_triggered);
	connect(m_ui.actionCloseDatabase, &QAction::triggered, this, &MainWindow::actionCloseDatabase_triggered);
	connect(m_ui.actionExit, &QAction::triggered, this, &QMainWindow::close);
	// edit
	connect(m_ui.actionEditSelected, &QAction::triggered, this, &MainWindow::actionEditSelected_triggered);
	connect(m_ui.actionCheckSelected, &QAction::triggered, this, &MainWindow::actionCheckSelected_triggered);
	connect(m_ui.actionDeleteSelected, &QAction::triggered, this, &MainWindow::actionDeleteSelected_triggered);
	// tools
	connect(m_ui.actionOptions, &QAction::triggered, this, &MainWindow::actionOptions_triggered);
	// help
	connect(m_ui.actionAboutQt, &QAction::triggered, this, &QApplication::aboutQt);

	connect(db, &Database::opened, this, &MainWindow::unlockUi);
	connect(db, &Database::closed, this, &MainWindow::lockUi);

	connect(m_ui.tabWidget, &QTabWidget::currentChanged, this, [this]() -> void {emit currentTabChanged((Tab)m_ui.tabWidget->currentIndex()); });

	readSettings();
	updateRecentlyOpened();
}

MainWindow::~MainWindow()
{}

void MainWindow::actionAddFile_triggered()
{
	NewFileDialog* dialog = new NewFileDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
	//if (m_newFileDialog)
	//    m_newFileDialog->activateWindow();
	//else
	//{
	//    m_newFileDialog = new NewFileDialog(this);
	//    m_newFileDialog->setAttribute(Qt::WA_DeleteOnClose);
	//    m_newFileDialog->show();
	//}
}

void MainWindow::actionCreateTag_triggered()
{
	NewTagDialog* dialog = new NewTagDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();
	//if (m_newTagDialog)
	//    m_newTagDialog->activateWindow();
	//else
	//{
	//    m_newTagDialog = new NewTagDialog(this);
	//    m_newTagDialog->setAttribute(Qt::WA_DeleteOnClose);
	//    m_newTagDialog->show();
	//}
}

void MainWindow::actionEditSelected_triggered()
{
	switch (m_ui.tabWidget->currentIndex())
	{
	case Tab::File:
		m_ui.fileList->editSelected();
		break;
	case Tab::Tag:
		m_ui.tagList->editSelected();
		break;
	}
}

void MainWindow::actionCheckSelected_triggered()
{
	if (m_ui.tabWidget->currentIndex() == Tab::File)
		m_ui.fileList->checkSelected();
}

void MainWindow::actionDeleteSelected_triggered()
{
	switch (m_ui.tabWidget->currentIndex())
	{
	case Tab::File:
		m_ui.fileList->deleteSelected();
		break;
	case Tab::Tag:
		m_ui.tagList->deleteSelected();
		break;
	}
}

void MainWindow::actionNewDatabase_triggered()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("SQLite database (*.sqlite *.sqlite3 *.db *.db3 *.s3db *.sl3)"));
	dialog.setDefaultSuffix("sqlite3");
	if (dialog.exec())
	{
		const QString path = dialog.selectedFiles().first();
		openDatabase(path);
	}
}

void MainWindow::actionOpenDatabase_triggered()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("SQLite database (*.sqlite *.sqlite3 *.db *.db3 *.s3db *.sl3)"));
	if (dialog.exec())
	{
		const QString path = dialog.selectedFiles().first();
		openDatabase(path);
	}
}

void MainWindow::actionCloseDatabase_triggered()
{
	db->close();
	setWindowTitle(qApp->applicationName());
}

void MainWindow::lockUi()
{
	m_ui.actionAddFile->setEnabled(false);
	m_ui.actionCreateTag->setEnabled(false);
	m_ui.actionEditSelected->setEnabled(false);
	m_ui.actionCheckSelected->setEnabled(false);
	m_ui.actionDeleteSelected->setEnabled(false);
	m_ui.actionCloseDatabase->setEnabled(false);
}

void MainWindow::unlockUi()
{
	updateRecentlyOpened();
	m_ui.actionAddFile->setEnabled(true);
	m_ui.actionCreateTag->setEnabled(true);
	m_ui.actionEditSelected->setEnabled(true);
	m_ui.actionCheckSelected->setEnabled(true);
	m_ui.actionDeleteSelected->setEnabled(true);
	m_ui.actionCloseDatabase->setEnabled(true);
}

void MainWindow::actionOptions_triggered()
{
	if (!m_settingsDialog)
	{
		m_settingsDialog = new SettingsDialog();
		m_settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
		m_settingsDialog->setModal(true);
		m_settingsDialog->show();
	}
}

void MainWindow::updateRecentlyOpened()
{
	QSettings settings;
	QStringList recentlyOpened = settings.value("GUI/MainWindow/recentlyOpened", QStringList()).toStringList();
	if (recentlyOpened.isEmpty())
	{
		m_ui.menuOpenRecent->setDisabled(true);
		return;
	}
	m_ui.menuOpenRecent->setDisabled(false);
	for (const QAction* action : m_ui.menuOpenRecent->actions())
		action->disconnect();
	m_ui.menuOpenRecent->clear();
	for (const QString& path : recentlyOpened)
	{
		QAction* action = new QAction(path, this);
		connect(action, &QAction::triggered, this, [this, path]() -> void
			{
				// ensure we only ever open existing databases
				if (QFileInfo::exists(path))
					openDatabase(path);
			});
		m_ui.menuOpenRecent->addAction(action);
	}
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	writeSettings();
	delete db;
	event->accept();
}

void MainWindow::readSettings()
{
	QSettings settings;

	const QByteArray geometry = settings.value("GUI/MainWindow/geometry", QByteArray()).toByteArray();
	if (!geometry.isEmpty())
		restoreGeometry(geometry);
	const QByteArray state = settings.value("GUI/MainWindow/state", QByteArray()).toByteArray();
	if (!state.isEmpty())
		restoreState(state);
	int tabIndex = settings.value("GUI/MainWindow/currentIndex", 0).toInt();
	m_ui.tabWidget->setCurrentIndex(tabIndex);

	const QString lastOpened = settings.value("lastOpened", QString()).toString();
	if (QFileInfo::exists(lastOpened) && !lastOpened.isEmpty())
		openDatabase(lastOpened);
}

void MainWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("GUI/MainWindow/geometry", saveGeometry());
	settings.setValue("GUI/MainWindow/state", saveState());
	settings.setValue("GUI/MainWindow/currentIndex", m_ui.tabWidget->currentIndex());
}

void MainWindow::openDatabase(const QString& path)
{
	if (DBResult error = db->open(path))
		QMessageBox::warning(this, qApp->applicationName()
			, tr("Failed to open database: ") + error.msg);
	else
	{
		setWindowTitle(qApp->applicationName() + " - " + path);
		m_ui.statusbar->showMessage(tr("Database opened"), 2000);
	}	
}
