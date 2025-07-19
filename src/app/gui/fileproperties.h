#pragma once

#include <QWidget>
#include "app/file.h"

namespace Ui
{
	class FileProperties;
}

class FileProperties : public QWidget
{
	Q_OBJECT

public:
	explicit FileProperties(const QSharedPointer<File>& file, QWidget* parent = nullptr);
	explicit FileProperties(QWidget* parent = nullptr);
	~FileProperties();
	QSharedPointer<File> file() const;
	void setFile(const QSharedPointer<File>& state);
	void clear();

private:
	Ui::FileProperties* m_ui;
	QSharedPointer<File> m_file;
	void populate();
	void depopulate();
};
