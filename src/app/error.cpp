#include "error.h"

Error::Error(const QString& type, const Error* parent)
	: Error(type, Ok, parent)
{}

Error::Error(const QString& type, int code, const Error* parent)
	: Error(type, code, u"No message provided"_s, parent)
{}

Error::Error(const QString& type, int code, const QString& message, const Error* parent)
	: type(type)
	, code(code)
	, baseMessage(type + u": "_s + message)
{
	this->message = baseMessage;
	if (parent)
	{
		parents = parent->parents;
		parents.insert(0, *parent);
		for (int i = 0; i < parents.size(); ++i)
			this->message += u"\n    "_s + QString::number(i + 1) + u": "_s + parents.at(i).baseMessage;
	}
}

Error::operator bool() const
{
	return code != 0;
}
