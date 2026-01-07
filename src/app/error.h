#pragma once

#include <QString>

struct Error
{
public:
	enum Code { Ok = 0, Fail };
	explicit Error(int code = Ok);
	explicit Error(int code, const QString& message);
	int code;
	virtual QString message() const;
	virtual QString name() const;
	QString baseMessage;
	explicit operator bool() const;
};
