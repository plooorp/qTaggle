#include "tag.h"

#include <QUrl>
#include "app/globals.h"

Tag::Tag()
	: m_id(-1)
{}

Tag::Tag(int64_t id)
	: m_id(id)
{}

Tag Tag::fromName(const QString& name)
{
	if (db->isClosed())
		return Tag();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT id FROM tag WHERE name = ?;", -1, &stmt, nullptr);
	QByteArray name_utf8 = name.toUtf8();
	sqlite3_bind_text(stmt, 1, name_utf8, -1, SQLITE_STATIC);
	Tag tag;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		tag = Tag(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return tag;
}

bool Tag::exists() const
{
	if (m_id < 0)
		return false;
	if (db->isClosed())
		return false;
	sqlite3_stmt* stmt;
	const char* sql = "SELECT EXISTS(SELECT 1 FROM tag WHERE id = ?);";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	bool exists = false;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		exists = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	return exists;
}

DBError Tag::create(const QString& name, const QString& description, const QList<QString>& urls, Tag* out)
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);

	QString name_norm = normalizeName(name);
	if (name_norm.isEmpty())
		return DBError(DBError::ValueError, "Name cannot be empty");

	sqlite3_stmt* stmt;
	const char* sql = "INSERT INTO tag(name, description) VALUES(?, ?);";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	QByteArray name_bytes = name_norm.toUtf8();
	QByteArray description_bytes = description.trimmed().toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, description_bytes.constData(), -1, SQLITE_STATIC);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (!urls.isEmpty() || out)
	{
		sqlite3_int64 rowid = sqlite3_last_insert_rowid(db->con());
		sqlite3_prepare_v2(db->con(), "SELECT id FROM tag WHERE ROWID = ?", -1, &stmt, nullptr);
		sqlite3_bind_int64(stmt, 1, rowid);
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			Tag tag(sqlite3_column_int64(stmt, 0));
			if (!urls.isEmpty())
				tag.setURLs(urls);
			if (out)
				*out = tag;
		}
		sqlite3_finalize(stmt);
	}
	return DBError();
}

DBError Tag::remove() const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "DELETE FROM tag WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	return DBError();
}

DBError Tag::setName(const QString& name) const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	QString name_norm = normalizeName(name);
	if (name.isEmpty())
		return DBError(DBError::ValueError, "Name cannot be empty");
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET name = ? WHERE id = ?;", -1, &stmt, nullptr);
	QByteArray name_bytes = name_norm.toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (DBError error = updateModified())
		return error;
	return DBError();
}

DBError Tag::setDescription(const QString& description) const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET description = ? WHERE id = ?;", -1, &stmt, nullptr);
	QByteArray description_bytes = description.trimmed().toUtf8();
	sqlite3_bind_text(stmt, 1, description_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (DBError error = updateModified())
		return error;
	return DBError();
}

DBError Tag::setURLs(const QStringList& urls) const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	QList<QString> oldURLsList = this->urls();
	QSet<QString> oldURLs(oldURLsList.begin(), oldURLsList.end());
	QSet<QString> newURLs(urls.begin(), urls.end());

	QSet<QString> toDelete = oldURLs - newURLs;
	QSet<QString> toInsert = newURLs - oldURLs;

	for (const QString& url : toDelete.values())
		if (DBError error = removeURL(url))
			return error;
	for (const QString& url : toInsert.values())
		if (DBError error = addURL(url))
			return error;
	return DBError();
}

DBError Tag::addURL(const QString& url) const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	QUrl qurl(url, QUrl::TolerantMode);
	if (!qurl.isValid())
		return DBError(DBError::ValueError, u"Invalid url: %1"_s.arg(url));
	sqlite3_stmt* stmt;
	QByteArray url_utf8 = qurl.toString().toUtf8();
	sqlite3_prepare_v2(db->con(), "INSERT INTO tag_url(tag_id, url) VALUES(?, ?);", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_text(stmt, 2, url_utf8, -1, SQLITE_STATIC);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_DONE)
	{
	if (DBError error = updateModified())
		return error;
	return DBError();
}
	if (rc == SQLITE_CONSTRAINT)
		return DBError();
	return DBError(rc);
}

DBError Tag::removeURL(const QString& url) const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	QByteArray url_utf8 = url.toUtf8();
	sqlite3_prepare_v2(db->con(), "DELETE FROM tag_url WHERE tag_id = ? AND url = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_text(stmt, 2, url_utf8, -1, SQLITE_STATIC);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (DBError error = updateModified())
		return error;
	return DBError();
}

int64_t Tag::id() const
{
	return m_id;
}

QString Tag::name() const
{
	if (db->isClosed())
		return QString();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT name FROM tag WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QString name;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		name = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0));
	sqlite3_finalize(stmt);
	return name;
}

QString Tag::description() const
{
	if (db->isClosed())
		return QString();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT description FROM tag WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QString description;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		description = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0));
	sqlite3_finalize(stmt);
	return description;
}

QStringList Tag::urls() const
{
	if (db->isClosed())
		return QStringList();
	sqlite3_stmt* stmt;
	const char* sql = "SELECT url FROM tag_url WHERE tag_id = ? ORDER BY url ASC;";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QStringList urls;
	while (sqlite3_step(stmt) == SQLITE_ROW)
		urls.append(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0)));
	sqlite3_finalize(stmt);
	return urls;
}

int64_t Tag::degree() const
{
	if (db->isClosed())
		return 0;
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT degree FROM tag WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	int64_t degree = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		degree = sqlite3_column_int64(stmt, 0);
	sqlite3_finalize(stmt);
	return degree;
}

//int64_t Tag::degree() const
//{
//	if (db->isClosed())
//		return 0;
//	sqlite3_stmt* stmt;
//	const char* sql = R"(
//		SELECT COUNT(*) AS degree FROM file_tag
//		WHERE tag_id = ?;
//	)";
//	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
//	sqlite3_bind_int64(stmt, 1, m_id);
//	int64_t degree = 0;
//	if (sqlite3_step(stmt) == SQLITE_ROW)
//		degree = sqlite3_column_int64(stmt, 0);
//	sqlite3_finalize(stmt);
//	return degree;
//}

QDateTime Tag::created() const
{
	if (db->isClosed())
		return QDateTime();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT created FROM tag WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QDateTime created;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		created = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return created;
}

QDateTime Tag::modified() const
{
	if (db->isClosed())
		return QDateTime();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT modified FROM tag WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QDateTime modified;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		modified = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return modified;
}

DBError Tag::updateModified() const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET modified = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, QDateTime::currentSecsSinceEpoch());
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	return DBError();
}

QString Tag::normalizeName(const QString& name)
{
	return name
		.trimmed()
		.toLower()
		.replace(' ', '_');
}
