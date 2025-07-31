#pragma once

#include <QDialog>
#include "app/tag.h"

namespace Ui
{
	class EditTagDialogMulti;
}

class EditTagDialogMulti : public QDialog
{
	Q_OBJECT

public:
	explicit EditTagDialogMulti(const QList<QSharedPointer<Tag>>& tags, QWidget* parent = nullptr);
	~EditTagDialogMulti();

private slots:
	void accept() override;

private:
	Ui::EditTagDialogMulti* m_ui;
	QList<QSharedPointer<Tag>> m_tags;
};
