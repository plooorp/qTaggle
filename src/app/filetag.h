#pragma once

#include <QDateTime>
#include <QSharedPointer>
#include "app/database.h"
#include "app/tag.h"

class FileTag
{
public:
	FileTag(sqlite3_stmt* stmt);
	QSharedPointer<Tag> tag() const;
	QDateTime created() const;

private:
	QSharedPointer<Tag> m_tag;
	QDateTime m_created;
};
