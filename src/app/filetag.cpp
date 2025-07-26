#include "filetag.h"

FileTag::FileTag(sqlite3_stmt* stmt)
	: m_tag(Tag::fromID(sqlite3_column_int64(stmt, 1)))
	, m_created(QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 2)))
{}

QSharedPointer<Tag> FileTag::tag() const
{
	return m_tag;
}

QDateTime FileTag::created() const
{
	return m_created;
}
