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
	explicit Properties(const MainWindow* mainWindow, FileList* fileList, TagList* tagList, QWidget* parent);

private:
	FileList* m_fileList;
	TagList* m_tagList;
	FileProperties* m_fileProperties;
	TagProperties* m_tagProperties;
	void populateFile(const QList<File>& files);
	void populateTag(const QList<Tag>& tags);
	void clear();
};
