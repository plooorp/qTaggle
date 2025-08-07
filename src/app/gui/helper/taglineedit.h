#pragma once

#include <QLineEdit>
#include <QAbstractListModel>

class TagLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	explicit TagLineEdit(QWidget* parent = nullptr);
};

class TagCompleterModel : public QAbstractListModel
{
	Q_OBJECT

public:
	explicit TagCompleterModel(QObject* parent = nullptr);
	void updateSuggestions(const QString& text);
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex& parent) const override;
	void clear();

private:
	QStringList m_suggestions;
};
