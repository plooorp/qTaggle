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
    explicit TagSelect(const QList<Tag>& tags, QWidget* parent = nullptr);
    explicit TagSelect(const QList<File>& files, QWidget* parent = nullptr);
    virtual ~TagSelect() override;
    QList<Tag> tags();
    void setTags(const QList<Tag>& tags);

private slots:
    void add();
    void remove();
    void importTags();
    void exportTags();
    void showContextMenu(const QPoint& pos);
    
private:
    Ui::TagSelect *m_ui;
    TagTableModel* m_model;
    void readSettings();
    void writeSettings();
};
