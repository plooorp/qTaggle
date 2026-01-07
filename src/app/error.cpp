#include "error.h"

#include "app/globals.h"

Error::Error(int code)
	: code(code)
	, baseMessage(u"No message provided"_s)
{}

Error::Error(int code, const QString& message)
	: code(code)
	, baseMessage(message)
{}

QString Error::message() const
{
	return u"[%1] %2"_s.arg(name(), baseMessage);
}

QString Error::name() const
{
	return u"BaseError"_s;
}

Error::operator bool() const
{
	return code != Ok;
}
