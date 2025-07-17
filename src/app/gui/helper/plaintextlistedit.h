#pragma once

#include <QWidget>
#include <QStringList>
#include <QtWidgets/QPlainTextEdit>

class PlainTextListEdit : public QPlainTextEdit
{
	Q_OBJECT

public:
	explicit PlainTextListEdit(QWidget* parent = nullptr);
	QStringList values() const;
	void setValues(const QStringList& values);
};