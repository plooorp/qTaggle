#pragma once

#include <QString>
#include <QList>
#include "app/globals.h"

struct Error
{
	explicit Error(const QString& type);
	explicit Error(const QString& type, int code, const Error* parent = nullptr);
	explicit Error(const QString& type, int code, const QString& message, const Error* parent = nullptr);
	enum Code { Ok = 0, Fail };
	int code;
	QString type;
	QString baseMessage;
	QString message;
	QList<Error> parents;
	explicit operator bool() const
	{
		return code != 0;
	}
};
