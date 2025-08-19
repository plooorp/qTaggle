#pragma once

#include <QObject>
#include <QDir>
#include <QHash>
#include <QDateTime>

#include "app/database.h"
#include "app/filetag.h"
#include "app/error.h"

struct CheckResult : public Error
{
	explicit CheckResult()
		: Error(u"CheckResult"_s, Ok, nullptr)
	{}
	explicit CheckResult(Code code, const Error* parent = nullptr)
		: Error(u"CheckResult"_s, code, parent)
	{}
	explicit CheckResult(Code code, const QString& message, const Error* parent = nullptr)
		: Error(u"CheckResult"_s, code, message, parent)
	{}
	// holds the file's new checksum when a mismatch is detected
	QByteArray sha1;
};

class File final : public QObject, public Record
{
	Q_OBJECT

public:
	~File();
	static DBResult create(const QString& path, const QString& alias = QString(), const QString& comment = QString()
		, const QString& source = QString(), QSharedPointer<File>* out = nullptr);
	static QSharedPointer<File> fromStmt(sqlite3_stmt* stmt);
	void fetch();
	enum State
	{
		Ok = 0,
		Error, // some other error
		FileMissing,
		ChecksumChanged
	};
	CheckResult check();
	static QString stateString(State state);
	DBResult addTag(const QSharedPointer<Tag>& tag);
	DBResult removeTag(const QSharedPointer<Tag>& tag);
	int64_t id() const;
	QString name() const;
	QString alias() const;
	DBResult setAlias(const QString& alias);
	QString path() const;
	DBResult setPath(const QString& path);
	State state() const;
	DBResult setState(File::State state);
	QString comment() const;
	DBResult setComment(const QString& comment);
	QString source() const;
	DBResult setSource(const QString& source);
	QByteArray sha1() const;
	DBResult setSHA1(const QByteArray&);
	QDateTime created() const;
	QDateTime modified() const;
	QDateTime checked() const;
	DBResult updateChecked();
	QList<FileTag> tags() const;
	DBResult remove();

signals:
	void updated();
	void deleted();

private:
	File(sqlite3_stmt* stmt);
	static const int SHA1_DIGEST_SIZE_BYTES = 20;
	static QMap<int64_t, QWeakPointer<File>> m_instances;
	static QStringList m_stateString;
	static QByteArray sha1Digest(const QString& path);
	void initFile(sqlite3_stmt* stmt);
	int64_t m_id;
	QString m_path;
	QString m_alias;
	State m_state;
	QString m_comment;
	QString m_source;
	QByteArray m_sha1;
	QDateTime m_created;
	QDateTime m_modified;
	QDateTime m_checked;
	QList<FileTag> m_tags;
};
