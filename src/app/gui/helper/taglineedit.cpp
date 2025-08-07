#include "taglineedit.h"

#include <QCompleter>
#include "app/globals.h"
#include "app/tag.h"

TagLineEdit::TagLineEdit(QWidget* parent)
	: QLineEdit(parent)
{
	TagCompleterModel* suggestions = new TagCompleterModel();
	QCompleter* completer = new QCompleter(suggestions, this);
	completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	setCompleter(completer);
	connect(this, &QLineEdit::textEdited, this, [this, suggestions]() -> void
		{
			suggestions->updateSuggestions(text());
		});
}

TagCompleterModel::TagCompleterModel(QObject* parent)
	: QAbstractListModel(parent)
{}

QVariant TagCompleterModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (index.row() < 0 || index.row() >= m_suggestions.size())
		return QVariant();

	if (role == Qt::DisplayRole || role == Qt::EditRole)
		return m_suggestions.at(index.row());

	return QVariant();
}

int TagCompleterModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return m_suggestions.size();
}

void TagCompleterModel::updateSuggestions(const QString& text)
{
	if (text.isEmpty())
		return clear();
	QString base, prefix;
	qsizetype i = std::max(text.lastIndexOf(' '), text.lastIndexOf('!'));
	if (i == -1)
		prefix = text + u"*"_s;
	else
	{
		base = text.sliced(0, i + 1);
		prefix = text.sliced(i + 1) + u"*"_s;
	}	
	if (prefix.isEmpty())
		return clear();

	QStringList tags;
	sqlite3_stmt* stmt;
	const char* sql = R"(
		SELECT name FROM tag_search
		WHERE tag_search MATCH ?
		ORDER BY bm25(tag_search, 10.0, 5.0)
		LIMIT 8;
	)";
	sqlite3_prepare_v2(db->con(), sql, -1, &stmt, nullptr);
	QByteArray completion_bytes = prefix.toUtf8();
	sqlite3_bind_text(stmt, 1, completion_bytes.constData(), -1, SQLITE_STATIC);
	beginResetModel();
	m_suggestions.clear();
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		QString suggestion = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
		m_suggestions.append(base + suggestion);
	}
	endResetModel();
	sqlite3_finalize(stmt);
}

void TagCompleterModel::clear()
{
	beginResetModel();
	m_suggestions.clear();
	endResetModel();
}
