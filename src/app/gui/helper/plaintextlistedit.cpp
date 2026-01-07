#include "plaintextlistedit.h"

PlainTextListEdit::PlainTextListEdit(QWidget* parent)
	: QPlainTextEdit(parent)
{}

QStringList PlainTextListEdit::values(int o) const
{
	QStringList lines = o & SkipEmptyLines
		? this->toPlainText().split('\n')
		: this->toPlainText().split('\n', Qt::KeepEmptyParts);
	if (o & TrimLines)
		for (QString& line : lines)
			line = line.trimmed();
	return lines;
}

void PlainTextListEdit::setValues(const QStringList& values)
{
	this->setPlainText(values.join('\n'));
}
