#pragma once

#include <QDateTime>
#include <QSharedPointer>
#include "app/database.h"
#include "app/tag.h"

struct FileTag
{
public:
	FileTag();
	FileTag(int64_t fileId, int64_t tagId);
	int64_t fileId() const;
	int64_t tagId() const;
	QDateTime created() const;
private:
	int64_t m_fileId;
	int64_t m_tagId;
};
