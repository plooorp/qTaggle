#include "properties.h"

#include <QVBoxLayout>
#include <QtWidgets/QScrollArea>

Properties::Properties(const MainWindow* mainWindow, FileList* fileList, TagList* tagList, QWidget* parent)
	: QStackedWidget(parent)
	, m_fileList(fileList)
	, m_tagList(tagList)
{
	setContentsMargins(0, 0, 0, 0);

	QScrollArea* fileScrollArea = new QScrollArea(this);
	//fileScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_fileProperties = new FileProperties(fileScrollArea);
	fileScrollArea->setWidget(m_fileProperties);
	addWidget(fileScrollArea);

	QScrollArea* tagScrollArea = new QScrollArea(this);
	//tagScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_tagProperties = new TagProperties(tagScrollArea);
	tagScrollArea->setWidget(m_tagProperties);
	addWidget(tagScrollArea);

	connect(m_fileList, &FileList::selectionChanged, this, &Properties::populateFile);
	connect(m_tagList, &TagList::selectionChanged, this, &Properties::populateTag);
	connect(mainWindow, &MainWindow::currentTabChanged, this, &Properties::setCurrentIndex);

	connect(db, &Database::closed, this, &Properties::clear);
}

void Properties::populateFile(const QList<File>& files)
{
	if (files.isEmpty())
		m_fileProperties->clear();
	else
		m_fileProperties->setFile(files.first());
}

void Properties::populateTag(const QList<Tag>& tags)
{
	if (tags.isEmpty())
		m_tagProperties->clear();
	else
		m_tagProperties->setTag(tags.first());
}

void Properties::clear()
{
	m_tagProperties->clear();
	m_fileProperties->clear();
}
