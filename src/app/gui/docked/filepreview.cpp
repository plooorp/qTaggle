#include "filepreview.h"

#include <QVBoxLayout>
#include <QImageReader>

FilePreview::FilePreview(FileList* fileList, QWidget* parent)
	: QWidget(parent)
{
	connect(fileList, &FileList::selectionChanged, this, &FilePreview::updatePreview);

	m_label = new QLabel(this);
	m_label->setText(tr("No preview available"));
	m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	//m_label->setScaledContents(true);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(m_label);
	setLayout(layout);
}

void FilePreview::updatePreview(const QList<File>& selected)
{
	if (selected.isEmpty())
		return clear();
	File file = selected.first();
	QString path = file.path();
	QFileInfo qpath(file.path());
	if (!QImageReader::supportedImageFormats().contains(qpath.suffix()))
		return clear();
	m_pixmap = QPixmap(path);
	if (m_pixmap.isNull())
		return clear();
	// margin so that the widget can still be rescaled
	QPixmap pixmap = m_pixmap.scaled(QSize(m_label->width() - 9, m_label->height() - 9), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	if (pixmap.isNull())
		return clear();
	m_label->setPixmap(pixmap);
}

void FilePreview::clear()
{
	m_label->setPixmap(QPixmap());
	m_label->setText(tr("No preview available"));
}

void FilePreview::resizeEvent(QResizeEvent* event)
{
	if (m_pixmap.isNull())
		return;
	m_label->setPixmap(m_pixmap.scaled(QSize(m_label->width() - 9, m_label->height() - 9), Qt::KeepAspectRatio));
	event->accept();
}
