#include "properties.h"

#include <QVBoxLayout>
#include <QtWidgets/QScrollArea>

Properties::Properties(MainWindow* mainWindow, FileList* fileList, TagList* tagList, QWidget* parent)
	: QStackedWidget(parent)
	, m_mainWindow(mainWindow)
	, m_fileList(fileList)
	, m_tagList(tagList)
{
	QScrollArea* fileScrollArea = new QScrollArea(this);
	m_fileProperties = new FileProperties(fileScrollArea);
	fileScrollArea->setWidget(m_fileProperties);
	addWidget(fileScrollArea);

	QScrollArea* tagScrollArea = new QScrollArea(this);
	m_tagProperties = new TagProperties(tagScrollArea);
	tagScrollArea->setWidget(m_tagProperties);
	addWidget(tagScrollArea);

	setContentsMargins(0, 0, 0, 0);

	connect(m_fileList, &FileList::selectionChanged, this, &Properties::populateFile);
	connect(m_tagList, &TagList::selectionChanged, this, &Properties::populateTag);
	connect(m_mainWindow, &MainWindow::currentTabChanged, this, &Properties::setCurrentIndex);

	connect(db, &Database::closed, this, &Properties::clear);
}

void Properties::populateFile()
{
	QList<QSharedPointer<File>> files = m_fileList->selectedFiles();
	if (files.isEmpty())
		m_fileProperties->clear();
	else
		m_fileProperties->setFile(files.first());
}

void Properties::populateTag()
{
	QList<QSharedPointer<Tag>> tags = m_tagList->selectedTags();
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
