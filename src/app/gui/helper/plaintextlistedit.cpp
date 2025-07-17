#include "plaintextlistedit.h"

PlainTextListEdit::PlainTextListEdit(QWidget* parent)
	: QPlainTextEdit(parent)
{}

QStringList PlainTextListEdit::values() const
{
	QStringList values;
	for (QString line : this->toPlainText().split('\n'))
	{
		QString trimmed = line.trimmed();
		if (!trimmed.isEmpty())
			values.append(trimmed);
	}
	return values;
}

void PlainTextListEdit::setValues(const QStringList& values)
{
	this->setPlainText(values.join('\n'));
}
