#pragma once

#include <QPlainTextEdit>

class PlainTextListEdit : public QPlainTextEdit
{
	Q_OBJECT

public:
	explicit PlainTextListEdit(QWidget* parent = nullptr);
	enum Options
	{
		SkipEmptyLines = 0x1,
		TrimLines = 0x2
	};
	QStringList values(int o = TrimLines | SkipEmptyLines) const;
	void setValues(const QStringList& values);
};