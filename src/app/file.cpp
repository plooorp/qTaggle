#include "file.h"

#include <QCryptographicHash>
#include <QFile>
#include "app/globals.h"

File::File()
	: m_id(-1)
{}

File::File(int64_t id)
	: m_id(id)
{}

bool File::exists() const
{
	if (m_id < 0)
		return false;
	if (db->isClosed())
		return false;
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT EXISTS(SELECT 1 FROM file WHERE id = ?);", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	bool exists = false;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		exists = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	return exists;
}

DBError File::create(const QString& path, const QString& alias, const QString& comment, const QString& source, File* out)
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	QFileInfo fileInfo(path);
	QByteArray sha1 = sha1Digest(path);
	if (sha1.isNull())
		return DBError(DBError::FileIOError, "Failed to calculate SHA1 digest");
	const char* sql = R"(
		INSERT INTO file(name, dir, alias, state, comment, source, sha1)
		VALUES(?, ?, ?, ?, ?, ?, ?);
	)";
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	QByteArray name_bytes = fileInfo.fileName().toUtf8();
	QByteArray dir_bytes = fileInfo.dir().absolutePath().toUtf8();
	QByteArray alias_bytes = alias.trimmed().toUtf8();
	QByteArray comment_bytes = comment.trimmed().toUtf8();
	QByteArray source_bytes = source.trimmed().toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, dir_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, alias_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 4, Ok);
	sqlite3_bind_text(stmt, 5, comment_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 6, source_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 7, sha1.constData(), SHA1_DIGEST_SIZE_BYTES, SQLITE_STATIC);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (out)
	{
		sqlite3_int64 rowid = sqlite3_last_insert_rowid(db->con());
		sqlite3_prepare_v2(db->con(), "SELECT id FROM file WHERE ROWID = ?;", -1, &stmt, nullptr);
		sqlite3_bind_int64(stmt, 1, rowid);
		if (sqlite3_step(stmt) == SQLITE_ROW)
			*out = File(sqlite3_column_int64(stmt, 0));
		sqlite3_finalize(stmt);
	}
	return DBError();
}

DBError File::remove() const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "DELETE FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	return DBError();
}

CheckError File::check() const
{
	CheckError ok = CheckError();
	QByteArray newChecksum = sha1Digest(path());
	QByteArray oldChecksum = sha1();
	if (!QFileInfo::exists(path()))
	{
		if (DBError error = setState(FileMissing))
			return CheckError(CheckError::Fail, u"Failed to update file state to FileMissing"_s, error);
	}
	else if (newChecksum.isEmpty())
	{
		if (DBError error = setState(Error))
			return CheckError(CheckError::Fail, u"Failed to update state to Error"_s, error);
	}
	else if (newChecksum != oldChecksum)
	{
		if (DBError error = setState(ChecksumChanged))
			return CheckError(CheckError::Fail, u"Failed to set state to ChecksumChanged"_s, error);
		ok.sha1 = oldChecksum;
	}
	else
		if (DBError error = setState(Ok))
			return CheckError(CheckError::Fail, u"Failed to set state back to Ok"_s, error);
	updateChecked();
	return ok;

}

DBError File::setTags(const QList<Tag>& tags) const
{
	QSet<Tag> oldTags;
	for (const FileTag& tag : this->tags())
		oldTags.insert(Tag(tag.tagId()));
	QSet<Tag> newTags(tags.begin(), tags.end());

	QSet<Tag> toDelete = oldTags - newTags;
	QSet<Tag> toInsert = newTags - oldTags;

	for (const Tag& tag : toDelete.values())
		if (DBError error = removeTag(tag))
			return error;
	for (const Tag& tag : toInsert)
		if (DBError error = addTag(tag))
			return error;
	return DBError();
}

DBError File::addTag(const Tag& tag) const
{
	sqlite3_stmt* stmt;
	const char* sql = "INSERT INTO file_tag(file_id, tag_id) VALUES(?, ?);";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_int64(stmt, 2, tag.id());
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

DBError File::removeTag(const Tag& tag) const
{
	sqlite3_stmt* stmt;
	const char* sql = "DELETE FROM file_tag WHERE file_id = ? AND tag_id = ?;";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	sqlite3_bind_int64(stmt, 2, tag.id());
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (DBError error = updateModified())
		return error;
	return DBError();
}

DBError File::setAlias(const QString& alias) const
{
	QByteArray alias_bytes = alias.trimmed().toUtf8();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET alias = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, alias_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (DBError error = updateModified())
		return error;
	return DBError();
}

QString File::displayName() const
{
	QString name = this->name();
	QString alias = this->alias();
	if (alias.isEmpty())
		return name;
	return alias;
}

DBError File::setPath(const QString& path) const
{
	QFileInfo file(path);
	QByteArray name_utf8 = file.fileName().toUtf8();
	QByteArray dir_utf8 = file.dir().absolutePath().toUtf8();
	sqlite3_stmt* stmt;
	const char* sql = "UPDATE file SET name = ?, dir = ? WHERE id = ?;";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, name_utf8.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, dir_utf8.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 3, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	return DBError();
}

DBError File::setState(File::State state) const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET state = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, state);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	return DBError();
}

DBError File::setComment(const QString& comment) const
{
	QByteArray comment_bytes = comment.trimmed().toUtf8();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET comment = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, comment_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (DBError error = updateModified())
		return error;
	return DBError();
}

DBError File::setSource(const QString& source) const
{
	QByteArray source_bytes = source.trimmed().toUtf8(); // validation
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET source = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, source_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (DBError error = updateModified())
		return error;
	return DBError();
}

DBError File::setSHA1(const QByteArray& sha1) const
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET sha1 = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_blob(stmt, 1, sha1, SHA1_DIGEST_SIZE_BYTES, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	if (DBError error = updateModified())
		return error;
	return DBError();
}

DBError File::updateChecked() const
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET checked = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, QDateTime::currentSecsSinceEpoch());
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	return DBError();
}

int64_t File::id() const
{
	return m_id;
}

QString File::path() const
{
	return QFileInfo(dir(), name()).filePath();
}

QString File::name() const
{
	if (db->isClosed())
		return QString();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT name FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QString name;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		name = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0));
	sqlite3_finalize(stmt);
	return name;
}

QString File::dir() const
{
	if (db->isClosed())
		return QString();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT dir FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QString dir;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		dir = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0));
	sqlite3_finalize(stmt);
	return dir;
}

QString File::alias() const
{
	if (db->isClosed())
		return QString();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT alias FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QString alias;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		alias = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0));
	sqlite3_finalize(stmt);
	return alias;
}

File::State File::state() const
{
	if (db->isClosed())
		return Ok;
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT state FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	State state = Ok;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		state = static_cast<State>(sqlite3_column_int(stmt, 0));
	sqlite3_finalize(stmt);
	return state;
}

QString File::comment() const
{
	if (db->isClosed())
		return QString();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT comment FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QString comment;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		comment = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0));
	sqlite3_finalize(stmt);
	return comment;
}

QString File::source() const
{
	if (db->isClosed())
		return QString();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT source FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QString source;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		source = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), sqlite3_column_bytes(stmt, 0));
	sqlite3_finalize(stmt);
	return source;
}

QByteArray File::sha1() const
{
	if (db->isClosed())
		return QByteArray();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT sha1 FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QByteArray sha1;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		sha1 = QByteArray(static_cast<const char*>(sqlite3_column_blob(stmt, 0)), SHA1_DIGEST_SIZE_BYTES);
	sqlite3_finalize(stmt);
	return sha1;
}

QDateTime File::created() const
{
	if (db->isClosed())
		return QDateTime();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT created FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QDateTime created;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		created = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return created;
}

QDateTime File::modified() const
{
	if (db->isClosed())
		return QDateTime();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT modified FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QDateTime modified;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		modified = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return modified;
}

DBError File::updateModified() const
{
	if (db->isClosed())
		return DBError(DBError::DatabaseClosed);
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE file SET modified = ? WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, QDateTime::currentSecsSinceEpoch());
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBError(rc);
	return DBError();
}

QDateTime File::checked() const
{
	if (db->isClosed())
		return QDateTime();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT checked FROM file WHERE id = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QDateTime checked;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		checked = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return checked;
}

QList<FileTag> File::tags() const
{
	if (db->isClosed())
		return QList<FileTag>();
	sqlite3_stmt* stmt;
	const char* sql = R"(
		SELECT tag_id FROM file_tag AS ft
		INNER JOIN tag ON tag.id = ft.tag_id
		WHERE ft.file_id = ?
		ORDER BY tag.name ASC;
	)";
	// const char* sql = "SELECT tag_id FROM file_tag WHERE file_id = ?;";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, m_id);
	QList<FileTag> tags;
	while (sqlite3_step(stmt) == SQLITE_ROW)
		tags.append(FileTag(m_id, sqlite3_column_int64(stmt, 0)));
	sqlite3_finalize(stmt);
	return tags;
}

QByteArray File::sha1Digest(const QString& path)
{
	QCryptographicHash hash(QCryptographicHash::Sha1);
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return QByteArray();
	char buff[4096];
	while (!file.atEnd())
	{
		qint64 bytes = file.read(buff, 4096);
		if (bytes < 0)
			return QByteArray();
		hash.addData(buff, bytes);
	}
	return hash.result();
}

int64_t File::countByState(File::State state)
{
	if (db->isClosed())
		return 0;
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT COUNT(*) FROM file WHERE state = ?;", -1, &stmt, nullptr);
	sqlite3_bind_int(stmt, 1, state);
	int64_t count = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW)
		count = sqlite3_column_int64(stmt, 0);
	sqlite3_finalize(stmt);
	return count;
}

const QStringList File::stateString
{
	"Ok",
	"Error",
	"File missing",
	"Checksum changed"
};
