#pragma once

#include <QWidget>
#include "app/tag.h"
#include "app/file.h"
#include "app/gui/model/tagtablemodel.h"

namespace Ui
{
    class TagSelect;
}

class TagSelect : public QWidget
{
    Q_OBJECT

public:
    explicit TagSelect(QWidget* parent = nullptr);
    explicit TagSelect(QList<QSharedPointer<Tag>> tags, QWidget* parent = nullptr);
    ~TagSelect();
    QList<QSharedPointer<Tag>> tags();
    void setTags(const QList<QSharedPointer<Tag>> tags);

private slots:
    void add();
    void remove();
    void populate();
    void importTags();
    void exportTags();
    void showContextMenu(const QPoint& pos);
    
private:
    Ui::TagSelect *m_ui;
    TagTableModel* m_model;
    //QCompleter* m_completer;
    //QStringListModel* m_completerModel;
};
