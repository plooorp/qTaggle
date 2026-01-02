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
	explicit FileProperties(const File& file, QWidget* parent = nullptr);
	explicit FileProperties(QWidget* parent = nullptr);
	virtual ~FileProperties() override;
	File file() const;
	void setFile(const File& state);
	void clear();

private:
	Ui::FileProperties* m_ui;
	File m_file;
	void populate();
	void depopulate();
};
