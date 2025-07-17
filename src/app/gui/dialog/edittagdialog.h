#pragma once

#include <QDialog>
#include <QList>
#include <QSharedPointer>
#include "app/tag.h"

namespace Ui
{
	class EditTagDialog;
}

class EditTagDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EditTagDialog(const QList<QSharedPointer<Tag>>& tags, QWidget* parent = nullptr);
	~EditTagDialog();

private slots:
	void accept() override;

private:
	Ui::EditTagDialog* m_ui;
	QList<QSharedPointer<Tag>> m_tags;
	int updateTag();
	int updateSingleTag();
};