#include "qlabeledlistwidget.h"

#include <qlabel.h>
#include <qlistwidget.h>
#include <qboxlayout.h>

QLabeledListWidget::QLabeledListWidget(QString label, QWidget *parent)
    : QWidget(parent)
{
    this->label = new QLabel(label, this);
    list = new QListWidget(this);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(this->label);
    l->addWidget(this->list);
    l->setMargin(0);
    setLayout(l);
}

QLabeledListWidget::~QLabeledListWidget()
{

}

QListWidget *QLabeledListWidget::widget()
{
    return list;
}