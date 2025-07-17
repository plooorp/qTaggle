#pragma once

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QPointer>
#include <QResizeEvent>
#include "app/gui/filelist.h"

class FilePreview : public QWidget
{
	Q_OBJECT

public:
	explicit FilePreview(QWidget* parent = nullptr);
	bool setFileList(FileList* fileList);

private:
	QPointer<FileList> m_fileList;
	QLabel* m_label;
	QPixmap m_pixmap;
	void updatePreview();
	void resizeEvent(QResizeEvent* event) override;
};
