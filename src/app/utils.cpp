#include "utils.h"

#include <QLocale>

QString friendlyNumber(const int64_t num)
{
	int64_t abs = std::abs(num);
	if (abs < 1000)
		return QLocale().toString(num);

	else if (abs < 10000)
		return QLocale().toString((double)num / 1000, 'f', 2) + "k";
	else if (abs < 100000)
		return QLocale().toString((double)num / 1000, 'f', 1) + "k";
	else if (abs < 1000000)
		return QLocale().toString((double)num / 1000, 'f', 0) + "k";

	else if (abs < 10000000)
		return QLocale().toString((double)num / 1000000, 'f', 2) + "M";
	else if (abs < 100000000)
		return QLocale().toString((double)num / 1000000, 'f', 1) + "M";
	else
		return QLocale().toString((double)num / 1000000, 'f', 0) + "M";
}