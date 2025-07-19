#pragma once

#include <QObject>
#include <QList>
#include <QDir>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QSharedPointer>

#include "app/database.h"
#include "app/filetag.h"

class File final
{
public:
	~File();
	static QSharedPointer<File> create(const QString& name, const QString& path, const QString& comment, const QString& source);
	static QSharedPointer<File> fromStmt(sqlite3_stmt* stmt);
	static QList<QSharedPointer<File>> fromQuery(const QString& query);
	void fetch();
	static int remove(QList<QSharedPointer<File>>& files);
	enum State
	{
		Ok = 0,
		FileMissing,
		ChecksumMismatch
	};
	static QString stateString(State state);
	DBResult addTag(const QSharedPointer<Tag>& tag);
	DBResult removeTag(const QSharedPointer<Tag>& tag);
	int64_t id() const;
	QString name() const;
	DBResult setName(const QString& name);
	QString path() const;
	DBResult setPath(const QString& path);
	//bool setPath(const QString& dir, const QString& name);
	QString dir() const;
	State state() const;
	QString comment() const;
	DBResult setComment(const QString& comment);
	QString source() const;
	DBResult setSource(const QString& source);
	QByteArray sha1() const;
	QDateTime created() const;
	QDateTime modified() const;
	QList<FileTag> tags() const;

private:
	File(sqlite3_stmt* stmt);
	static const int SHA1_DIGEST_SIZE = 20;
	static QMap<int64_t, QWeakPointer<File>> m_instances;
	static QStringList m_stateString;
	static QByteArray sha1Digest(const QString& path);
	void fetchTags();
	bool setState(File::State state);
	int64_t m_id;
	QString m_name;
	QString m_path;
	QString m_dir;
	State m_state;
	QString m_comment;
	QString m_source;
	QByteArray m_sha1;
	QDateTime m_created;
	QDateTime m_modified;
	QList<FileTag> m_tags;
};
