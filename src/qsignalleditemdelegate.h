#pragma once

#include <qitemdelegate.h>

class QSignalledItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        QItemDelegate::setEditorData(editor, index);
        emit startEditing(editor, index);
    }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        QItemDelegate::setModelData(editor, model, index);
        emit endEditing(editor, model, index);
    }

signals:
    void startEditing(QWidget *editor, const QModelIndex &index) const;
    void endEditing(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};