#pragma once

#include <qwidget.h>

class QLabel;
class QListWidget;

class QLabeledListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QLabeledListWidget(QString label, QWidget *parent = 0);
    ~QLabeledListWidget();

    QListWidget *widget();

private:
    QLabel *label;
    QListWidget *list;
};