#include "filepreview.h"

#include <QVBoxLayout>
#include <QImageReader>

FilePreview::FilePreview(QWidget* parent)
	: QWidget(parent)
	, m_label(new QLabel(this))
{
	m_label->setText(tr("No preview available"));
	m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	//m_label->setScaledContents(true);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(m_label);
	setLayout(layout);
}

void FilePreview::updatePreview()
{
	if (!m_fileList)
		return m_label->setPixmap(QPixmap());
	QList<QSharedPointer<File>> files = m_fileList->selectedFiles();
	if (files.isEmpty())
		return m_label->setPixmap(QPixmap());
	QSharedPointer<File> file = files.first();
	QFileInfo path(file->path());
	if (!QImageReader::supportedImageFormats().contains(path.suffix()))
	{
		m_label->setText(tr("No preview available"));
		return m_label->setPixmap(QPixmap());
	}
	m_pixmap = QPixmap(file->path());
	if (m_pixmap.isNull())
		return m_label->setPixmap(QPixmap());
	// margin so that the widget can still be rescaled
	QPixmap pixmap = m_pixmap.scaled(QSize(m_label->width() - 9, m_label->height() - 9), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	if (pixmap.isNull())
		return m_label->setPixmap(QPixmap());
	m_label->setPixmap(pixmap);
}

bool FilePreview::setFileList(FileList* fileList)
{
	if (!fileList)
		return false;

	if (m_fileList)
		disconnect(m_fileList, &FileList::selectionChanged, this, &FilePreview::updatePreview);
	m_fileList = fileList;
	connect(m_fileList, &FileList::selectionChanged, this, &FilePreview::updatePreview);
	return true;
}

void FilePreview::resizeEvent(QResizeEvent* event)
{
	if (m_pixmap.isNull())
		return;
	m_label->setPixmap(m_pixmap.scaled(QSize(m_label->width() - 9, m_label->height() - 9), Qt::KeepAspectRatio));
	event->accept();
}
