#include "tag.h"

#include <QDebug>
#include <QUrl>

Tag::Tag(sqlite3_stmt* stmt)
	: QObject(nullptr)
	, m_id(sqlite3_column_int64(stmt, 0))
	, m_name(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1)))
	, m_description(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2), sqlite3_column_bytes(stmt, 2)))
	, m_urls(QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3), sqlite3_column_bytes(stmt, 3)).split(';'))
	, m_degree(sqlite3_column_int64(stmt, 4))
	, m_created(QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 5)))
	, m_modified(QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 6)))
{
	qDebug() << "Tag" << m_name << "created";
}

Tag::~Tag()
{
	m_instances.remove(m_id);
	qDebug() << "Tag" << m_name << "deleted";
}

void Tag::fetch()
{
	if (!db->isOpen())
		return;
	sqlite3_stmt* stmt;
	std::string sql = "SELECT * FROM tag WHERE id = " + std::to_string(m_id) + ";";
	sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		m_id = sqlite3_column_int64(stmt, 0);
		m_name = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
		m_description = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2), sqlite3_column_bytes(stmt, 2));
		m_urls = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3), sqlite3_column_bytes(stmt, 3)).split(';');
		m_degree = sqlite3_column_int64(stmt, 4);
		m_created = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 5));
		m_modified = QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 6));
	}
	sqlite3_finalize(stmt);
	emit updated();
}

QSharedPointer<Tag> Tag::create(const QString& name, const QString& description, const QStringList& urls)
{
	QSharedPointer<Tag> tag;
	if (!db->isOpen())
		return tag;
	QString name_norm = normalizeName(name);
	QString description_norm = description.trimmed();
	QString urls_str;
	if (urls.isEmpty())
		urls_str = "";
	else
	{
		QStringList urls_cpy;
		for (QString url : urls)
			urls_cpy.append(QUrl(url, QUrl::TolerantMode).toEncoded());
		urls_str = urls_cpy.join(';');
	}

	const std::string q = "INSERT INTO tag(name, description, urls) VALUES(?, ?, ?);";
	db->begin();
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), q.c_str(), -1, &stmt, nullptr);
	QByteArray name_ba = name_norm.toUtf8();
	QByteArray desc_ba = description_norm.toUtf8();
	QByteArray urls_ba = urls_str.toUtf8();
	sqlite3_bind_text(stmt, 1, name_ba.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, desc_ba.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, urls_ba.constData(), -1, SQLITE_STATIC);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
	{
		db->rollback();
		qCritical() << "Failed to create tag:" << sqlite3_errstr(rc);
		return tag;
	}
	db->commit();
	sqlite3_prepare_v2(db->con(), "SELECT * FROM tag WHERE name = ?", -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, name_ba.constData(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		tag = QSharedPointer<Tag>(new Tag(stmt));
		m_instances.insert(tag->id(), tag.toWeakRef());
	}
	sqlite3_finalize(stmt);
	return tag;
}

QSharedPointer<Tag> Tag::create(const QString& name, const QString& description)
{
	return Tag::create(name, description, QStringList());
}

QSharedPointer<Tag> Tag::create(const QString& name)
{
	return Tag::create(name, QString(), QStringList());
}

QSharedPointer<Tag> Tag::fromStmt(sqlite3_stmt* stmt)
{
	int64_t id = sqlite3_column_int64(stmt, 0);
	if (m_instances.contains(id))
		if (QSharedPointer<Tag> tag = m_instances.value(id).toStrongRef())
			return tag;
	QSharedPointer<Tag> tag(new Tag(stmt));
	m_instances.insert(tag->id(), tag.toWeakRef());
	return tag;
}

QSharedPointer<Tag> Tag::fromID(const int64_t id)
{
	if (m_instances.contains(id))
		if (QSharedPointer<Tag> tag = m_instances.value(id).toStrongRef())
			return tag;
	QSharedPointer<Tag> tag;
	if (!db->isOpen())
		return tag;
	std::string q = "SELECT * FROM tag WHERE id = ?";
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), q.c_str(), -1, &stmt, nullptr);
	sqlite3_bind_int64(stmt, 1, id);
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		tag = QSharedPointer<Tag>(new Tag(stmt));
		m_instances.insert(tag->id(), tag.toWeakRef());
	}
	else
		qCritical() << "No such tag exists with id" << id;
	sqlite3_finalize(stmt);
	return tag;
}

QSharedPointer<Tag> Tag::fromName(const QString& name)
{
	QSharedPointer<Tag> tag;
	if (!db->isOpen())
		return tag;
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "SELECT * FROM tag WHERE name = ?;", -1, &stmt, nullptr);
	QByteArray name_bytes = name.toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW)
		tag = fromStmt(stmt);
	sqlite3_finalize(stmt);
	return tag;
}

QList<QSharedPointer<Tag>> Tag::fromQuery(const QString& query)
{
	QList<QSharedPointer<Tag>> tags;
	if (!db->isOpen())
		return tags;
	QString query_trimmed = query.trimmed();
	QByteArray query_bytes = query_trimmed.toUtf8();
	sqlite3_stmt* stmt;
	if (query_trimmed.isEmpty())
		sqlite3_prepare_v2(db->con(), "SELECT * FROM tag;", -1, &stmt, nullptr);
	else
	{
		std::string sql = R"(
			SELECT * FROM tag
			WHERE name LIKE concat('%', ?, '%')
			ORDER BY
				CASE
					WHEN name LIKE concat(?, '%') THEN 1
					WHEN name LIKE concat('%', ?, '%') THEN 2
					ELSE 3
				END
				, degree DESC;
		)";
		sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
		sqlite3_bind_text(stmt, 1, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, query_bytes.constData(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, query_bytes.constData(), -1, SQLITE_STATIC);
	}
	while (sqlite3_step(stmt) == SQLITE_ROW)
		tags.append(fromStmt(stmt));
	sqlite3_finalize(stmt);
	return tags;
}

int Tag::remove(QList<QSharedPointer<Tag>> tags)
{
	if (!db->isOpen())
		return SQLITE_CANTOPEN;
	int rc = 0;
	db->begin();
	sqlite3_stmt* stmt;
	std::string sql = "DELETE FROM tag WHERE id = ?;";
	for (QSharedPointer<Tag> tag : tags)
	{
		sqlite3_prepare_v2(db->con(), sql.c_str(), -1, &stmt, nullptr);
		sqlite3_bind_int64(stmt, 1, tag->m_id);
		rc = sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		if (rc != SQLITE_DONE)
		{
			db->rollback();
			qCritical() << "Failed to delete tag" << tag->m_name << ":" << sqlite3_errstr(rc);
			return rc;
		}
	}
	db->commit();
	return rc;
}

DBResult Tag::setName(const QString& name)
{
	QString name_norm = normalizeName(name);
	if (name.isEmpty())
		return DBResult(DBResult::ValueError, "Name cannot be empty");

	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET name = ? WHERE id = ?;", -1, &stmt, nullptr);
	QByteArray name_bytes = name_norm.toUtf8();
	sqlite3_bind_text(stmt, 1, name_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

DBResult Tag::setDescription(const QString& description)
{
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET description = ? WHERE id = ?;", -1, &stmt, nullptr);
	QByteArray description_bytes = description.trimmed().toUtf8();
	sqlite3_bind_text(stmt, 1, description_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

DBResult Tag::setURLs(const QStringList& urls)
{
	QString urls_str;
	if (urls.isEmpty())
		urls_str = "";
	else
	{
		QStringList urls_cpy;
		for (QString url_str : urls)
		{
			QUrl url(url_str, QUrl::TolerantMode);
			if (!url.isEmpty())
				urls_cpy.append(url.toEncoded());
		}
		urls_str = urls_cpy.join(';');
	}

	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db->con(), "UPDATE tag SET urls = ? WHERE id = ?;", -1, &stmt, nullptr);
	QByteArray urls_str_bytes = urls_str.toUtf8();
	sqlite3_bind_text(stmt, 1, urls_str_bytes.constData(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 2, m_id);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE)
		return DBResult(rc);
	if (db->inTransaction())
		db->addRecordToRollbackFetch(m_instances.value(m_id));
	fetch();
	return DBResult();
}

int64_t Tag::id() const
{
	return m_id;
}

QString Tag::name() const
{
	return m_name;
}

QString Tag::description() const
{
	return m_description;
}

QStringList Tag::urls() const
{
	return m_urls;
}

int64_t Tag::degree() const
{
	return m_degree;
}

QDateTime Tag::created() const
{
	return m_created;
}

QDateTime Tag::modified() const
{
	return m_modified;
}

QString Tag::normalizeName(const QString& name)
{
	return name
		.trimmed()
		.toLower()
		.replace(' ', '_');
}

QMap<int64_t, QWeakPointer<Tag>> Tag::m_instances;
