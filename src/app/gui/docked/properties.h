#pragma once

#include <QWidget>
#include <QtWidgets/QStackedWidget>

#include "app/gui/mainwindow.h"
#include "app/gui/filelist.h"
#include "app/gui/taglist.h"
#include "app/gui/tagproperties.h"
#include "app/gui/fileproperties.h"

class Properties : public QStackedWidget
{
	Q_OBJECT

public:
	explicit Properties(MainWindow* mainWindow, FileList* fileList, TagList* tagList, QWidget* parent);

private:
	QPointer<MainWindow> m_mainWindow;
	QPointer<FileList> m_fileList;
	QPointer<TagList> m_tagList;
	FileProperties* m_fileProperties;
	TagProperties* m_tagProperties;
	void populateFile();
	void populateTag();
	void clear();
};
