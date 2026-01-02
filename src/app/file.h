#pragma once

#include <QObject>
#include <QDir>
#include <QHash>
#include <QDateTime>

#include "app/database.h"
#include "app/error.h"
#include "app/filetag.h"
#include "app/tag.h"

struct CheckError : public Error
{
	explicit CheckError(const Error* parent = nullptr)
		: Error(u"CheckError"_s, Ok, parent)
	{}
	explicit CheckError(Code code, const Error* parent = nullptr)
		: Error(u"CheckError"_s, code, parent)
	{}
	explicit CheckError(Code code, const QString& message, const Error* parent = nullptr)
		: Error(u"CheckError"_s, code, message, parent)
	{}
	// holds the file's new checksum when a mismatch is detected
	QByteArray sha1;
};

struct File
{
public:
	File();
	File(int64_t id);
	static DBError create(const QString& path, const QString& alias = QString(), const QString& comment = QString()
		, const QString& source = QString(), File* out = nullptr);
	enum State
	{
		Ok = 0,
		Error, // some other error
		FileMissing,
		ChecksumChanged
	};
	static const QStringList stateString;
	static int64_t countByState(File::State state);
	//static QString stateString(State state);
	bool exists() const;
	CheckError check() const;
	int64_t id() const;
	QString name() const;
	QString alias() const;
	QString displayName() const;
	DBError setAlias(const QString& alias) const;
	QString path() const;
	QString dir() const;
	DBError setPath(const QString& path) const;
	State state() const;
	DBError setState(File::State state) const;
	QString comment() const;
	DBError setComment(const QString& comment) const;
	QString source() const;
	DBError setSource(const QString& source) const;
	QByteArray sha1() const;
	DBError setSHA1(const QByteArray&) const;
	QDateTime created() const;
	QDateTime modified() const;
	QDateTime checked() const;
	DBError updateChecked() const;
	QList<FileTag> tags() const;
	DBError addTag(const Tag& tag) const;
	DBError removeTag(const Tag& tag) const;
	DBError setTags(const QList<Tag>& tags) const;
	DBError remove() const;
	bool operator==(const File& other) const
	{
		return this->id() == other.id();
	}
	bool operator!=(const File& other) const
	{
		return this->id() != other.id();
	}

private:
	static const int SHA1_DIGEST_SIZE_BYTES = 20;
	static QByteArray sha1Digest(const QString& path);
	int64_t m_id;
	DBError updateModified() const;
};

namespace std
{
	template <>
	struct hash<File> {
		std::size_t operator()(const File& file) const noexcept
		{
			return file.id();
		}
	};
}
