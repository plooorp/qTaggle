#pragma once

#include <QLabel>
#include <QPixmap>
#include <QPointer>
#include <QResizeEvent>
#include <QWidget>
#include "app/gui/filelist.h"

class FilePreview : public QWidget
{
	Q_OBJECT

public:
	explicit FilePreview(FileList* fileList, QWidget* parent = nullptr);

private slots:
	void updatePreview();

private:
	QPointer<FileList> m_fileList;
	QLabel* m_label;
	QPixmap m_pixmap;
	void resizeEvent(QResizeEvent* event) override;
	void clear();
};
