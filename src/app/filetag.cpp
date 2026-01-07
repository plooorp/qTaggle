#include "filetag.h"

FileTag::FileTag()
	: m_fileId(-1)
	, m_tagId(-1)
{}

FileTag::FileTag(int64_t fileId, int64_t tagId)
	: m_fileId(fileId)
	, m_tagId(tagId)
{}

int64_t FileTag::fileId() const
{
	return m_fileId;
}

int64_t FileTag::tagId() const
{
	return m_tagId;
}

QDateTime FileTag::created() const
{
	if (db->isClosed())
		return QDateTime();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT created FROM file_tag WHERE file_id = ? AND tag_id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_fileId);
	sqlite3_bind_int64(stmt, 2, m_tagId);
	QDateTime created;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		created = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return created;
}