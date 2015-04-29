#include "mainwindow.h"

#include <qaction.h>
#include <qboxlayout.h>
#include <qfiledialog.h>
#include <qheaderview.h>
#include <qimage.h>
#include <qimagereader.h>
#include <qinputdialog.h>
#include <qitemdelegate.h>
#include <qlabel.h>
#include <qlistwidget.h>
#include <qmenu.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qplaintextedit.h>
#include <qscrollbar.h>
#include <qsettings.h>
#include <qtablewidget.h>
#include <qtimer.h>
#include <qtreewidget.h>

#include <qapplication.h>
#include <qtranslator.h>

#include <Polygon4/Database.h>
#include <Polygon4/DatabaseSchema.h>
#include <Polygon4/Helpers.h>
#include <Polygon4/Storage.h>

#include "version.h"

#include "qlabeledlistwidget.h"
#include "qsignalleditemdelegate.h"

#define ADD_TABLE_TO_VIEW(type, vars, var) \
    { \
    std::map<polygon4::String, polygon4::Ptr<polygon4::detail::type>> __vars; \
    for (auto &v : storage->vars) \
        __vars[v.second->var] = v.second; \
    for (auto &v : __vars) \
    { \
        auto i = new QTreeWidgetItem(item, QStringList(QString::fromStdWString(v.first))); \
        i->setData(0, Qt::UserRole, (uint64_t)v.second.get())

QTranslator appTranslator;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    
    setMinimumSize(600, 400);
    resize(minimumSizeHint());
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    createWidgets();
    createActions();
    createLanguageMenu();
    createMenus();
    createButtons();
    createLayouts();

    retranslateUi();
}

void MainWindow::createActions()
{
    openDbAction = new QAction(this);
    connect(openDbAction, SIGNAL(triggered()), SLOT(openDb()));

    saveDbAction = new QAction(this);
    connect(saveDbAction, SIGNAL(triggered()), SLOT(saveDb()));

    exitAction = new QAction(this);
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    aboutAction = new QAction(this);
    connect(aboutAction, &QAction::triggered, [=]
    {
        QMessageBox::information(this, tr("Polygon-4 DB Tool"),
            tr("Author: lzwdgc") + ", 2015\n" + tr("Version") +
            QString(": %1.%2.%3.%4")
            .arg(DBTOOL_VERSION_MAJOR)
            .arg(DBTOOL_VERSION_MINOR)
            .arg(DBTOOL_VERSION_PATCH)
            .arg(DBTOOL_VERSION_BUILD));
    });
}

void MainWindow::createLanguageMenu()
{
    languageMenu = new QMenu(this);
    QAction *defaultApplicationLanguage = languageMenu->addAction("English");
    defaultApplicationLanguage->setCheckable(true);
    defaultApplicationLanguage->setData(QString());
    defaultApplicationLanguage->setChecked(true);
    languageActionGroup = new QActionGroup(this);
    languageActionGroup->addAction(defaultApplicationLanguage);
    connect(languageActionGroup, SIGNAL(triggered(QAction *)), SLOT(changeLanguage(QAction *)));

    bool defaultFound = false;
    QString tsDirName = ":/ts/";
    QDir dir(tsDirName);
    QStringList filenames = dir.entryList();
    for (int i = 0; i < filenames.size(); ++i)
    {
        QString filename = tsDirName + filenames[i];

        QTranslator translator;
        translator.load(filename);

        QString language = translator.translate("MainWindow", "English");
        if (language == "English")
            continue;
        QString defaultLanguage = translator.translate("MainWindow", "Default application language",
            "Set this variable to \"1\" to default choose current language");

        QAction *action = new QAction(language, this);
        action->setCheckable(true);
        action->setData(filename);
        languageMenu->addAction(action);
        languageActionGroup->addAction(action);

        if (!defaultFound && defaultLanguage != QString())
        {
            action->setChecked(true);
            defaultFound = true;

            QApplication::installTranslator(&appTranslator);
            appTranslator.load(action->data().toString());
        }
    }
}

void MainWindow::createMenus()
{
    fileMenu = new QMenu(this);
    fileMenu->addAction(openDbAction);
    fileMenu->addAction(saveDbAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    settingsMenu = new QMenu(this);
    settingsMenu->addMenu(languageMenu);

    helpMenu = new QMenu(this);
    helpMenu->addAction(aboutAction);

    mainMenu = new QMenuBar(this);
    mainMenu->addMenu(fileMenu);
    mainMenu->addMenu(settingsMenu);
    mainMenu->addMenu(helpMenu);

    setMenuBar(mainMenu);
}

void MainWindow::createButtons()
{
}

void MainWindow::createLayouts()
{
    leftLayout = new QHBoxLayout;
    leftLayout->addWidget(treeWidget);

    rightLayout = new QVBoxLayout;
    rightLayout->addWidget(tableWidget);

    mainLayout = new QHBoxLayout;
    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 5);

    centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
}

void MainWindow::createWidgets()
{
    signalledItemDelegate = new QSignalledItemDelegate;

    treeWidget = new QTreeWidget(this);
    treeWidget->setColumnCount(1);
    treeWidget->setHeaderHidden(true);
    treeWidget->setMaximumWidth(300);

    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(3);
    tableWidget->horizontalHeader()->setVisible(false);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
    tableWidget->verticalHeader()->setDefaultSectionSize(24);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->setItemDelegateForColumn(2, signalledItemDelegate);

    connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), SLOT(currentTreeWidgetItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
    connect(tableWidget, SIGNAL(itemClicked(QTableWidgetItem *)), SLOT(currentTableWidgetItemChanged(QTableWidgetItem *)));

    connect(signalledItemDelegate, SIGNAL(startEditing(QWidget *, const QModelIndex &)), SLOT(tableWidgetStartEdiding(QWidget *, const QModelIndex &)));
    connect(signalledItemDelegate, SIGNAL(endEditing(QWidget *, QAbstractItemModel *, const QModelIndex &)), SLOT(tableWidgetEndEdiding(QWidget *, QAbstractItemModel *, const QModelIndex &)));
}

void MainWindow::retranslateUi()
{
    fileMenu->setTitle(tr("File"));
    openDbAction->setText(tr("Open database..."));
    saveDbAction->setText(tr("Save database..."));
    exitAction->setText(tr("Exit"));
    settingsMenu->setTitle(tr("Settings"));
    languageMenu->setTitle(tr("Language"));
    helpMenu->setTitle(tr("Help"));
    aboutAction->setText(tr("About"));
    
    setTableHeaders();
    setTitle();
}

void MainWindow::setTableHeaders()
{
    QStringList headers;
    headers << tr("Field name") << tr("Type") << tr("Value");
    tableWidget->setHorizontalHeaderLabels(headers);
}

void MainWindow::setTitle()
{
    QString title;
    if (database)
    {
        title += QString(" - ") + database->getName().c_str();
        if (dataChanged)
            title += "*";
    }
    setWindowTitle(tr("Polygon-4 DB Tool") + title);
}

void MainWindow::changeLanguage(QAction *action)
{
    QApplication::removeTranslator(&appTranslator);
    if (action->data() != QString())
    {
        QApplication::installTranslator(&appTranslator);
        appTranslator.load(action->data().toString());
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //event->ignore();
}

void MainWindow::openDb()
{
    QSettings settings;
    QString dir = settings.value("openDir", ".").toString();

    QString fn = QFileDialog::getOpenFileName(this, tr("Open file"), dir, tr("sqlite3 database") + " (*.sqlite)");
    if (fn == QString())
        return;

    settings.setValue("openDir", QFileInfo(fn).absoluteDir().absolutePath());

    QByteArray ba = fn.toLatin1();
    std::string filename = ba.data();

    try
    {
        database = std::make_shared<polygon4::Database>(filename);
        storage = polygon4::initStorage(database);
        storage->load();
    }
    catch (std::exception e)
    {
        QMessageBox::critical(this, "Critical error while opening database!", e.what());
        return;
    }

    setTitle();

    dataChanged = false;
    delete schema;
    schema = new polygon4::DatabaseSchema;
    database->getSchema(schema);
        
    treeWidget->clear();
    QList<QTreeWidgetItem *> items;
    for (auto &tbl : schema->tables)
    {
        auto item = new QTreeWidgetItem((QTreeWidget*)0, QStringList(tbl.first.c_str()));
        auto type = polygon4::detail::getTableType(tbl.first);
        switch (type)
        {
            using namespace polygon4::detail;
        case EObjectType::Building:
            ADD_TABLE_TO_VIEW(Building, buildings, name.ptr->get()); } }
            break;
        case EObjectType::Clan:
            ADD_TABLE_TO_VIEW(Clan, clans, name.ptr->get()); } }
            break;
        case EObjectType::Configuration:
            ADD_TABLE_TO_VIEW(Configuration, configurations, name.ptr->get()); } }
            break;
        case EObjectType::Equipment:
            ADD_TABLE_TO_VIEW(Equipment, equipments, name.ptr->get()); } }
            break;
        case EObjectType::Glider:
            ADD_TABLE_TO_VIEW(Glider, gliders, name.ptr->get()); } }
            break;
        case EObjectType::Good:
            ADD_TABLE_TO_VIEW(Good, goods, name.ptr->get()); } }
            break;
        case EObjectType::Map:
            ADD_TABLE_TO_VIEW(Map, maps, name.ptr->get()); } }
            break;
        case EObjectType::Mechanoid:
            ADD_TABLE_TO_VIEW(Mechanoid, mechanoids, name); } }
            break;
        case EObjectType::Modification:
            ADD_TABLE_TO_VIEW(Modification, modifications, name); } }
            break;
        case EObjectType::Modificator:
            ADD_TABLE_TO_VIEW(Modificator, modificators, name); } }
            break;
        case EObjectType::Object:
            ADD_TABLE_TO_VIEW(Object, objects, name); } }
            break;
        case EObjectType::Player:
            for (auto &player : storage->players)
            {
                auto i = new QTreeWidgetItem(item, QStringList(tr("Player #") + QString::number(player.first)));
                i->setData(0, Qt::UserRole, (uint64_t)player.second.get());
            }
            break;
        case EObjectType::Projectile:
            ADD_TABLE_TO_VIEW(Projectile, projectiles, name.ptr->get()); } }
            break;
        case EObjectType::Quest:
            ADD_TABLE_TO_VIEW(Quest, quests, name.ptr->get()); } }
            break;
        case EObjectType::Save:
            ADD_TABLE_TO_VIEW(Save, saves, name); } }
            break;
        case EObjectType::String:
            for (auto &str : storage->strings)
            {
                QString s = QString("%1. %2").arg(str.first).arg(QString::fromStdWString(str.second->get()));
                auto i = new QTreeWidgetItem(item, QStringList(s));
                i->setData(0, Qt::UserRole, (uint64_t)str.second.get());
            }
            break;
        case EObjectType::Weapon:
            ADD_TABLE_TO_VIEW(Weapon, weapons, name.ptr->get()); } }
            break;
        default:
            delete item;
            continue;
        }
        items.append(item);
    }
    treeWidget->insertTopLevelItems(0, items);
}

void MainWindow::saveDb()
{
    try
    {
        if (storage)
            storage->save();
        dataChanged = false;
        setTitle();
    }
    catch (std::exception e)
    {
        QMessageBox::critical(this, "Critical error while saving database!", e.what());
        return;
    }
}

void MainWindow::currentTreeWidgetItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    using namespace polygon4::detail;

    if (!current || current->parent() == 0)
    {
        tableWidget->setRowCount(0);
        tableWidget->horizontalHeader()->setVisible(false);
        return;
    }
    setTableHeaders();
    tableWidget->horizontalHeader()->setVisible(true);

    currentTreeWidgetItem = current;
    currentTableWidgetItem = 0;

    IObject *data = (IObject *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
    auto &table = schema->tables[polygon4::detail::getTableNameByType(data->getType())];
    tableWidget->setRowCount(table.columns.size());
    for (auto &col : table.columns)
    {
        bool disabled = false;
        if (col.second.id == 0 && col.second.name == "id" && col.second.type == polygon4::ColumnType::Integer)
            disabled = true;

        auto item = new QTableWidgetItem(col.first.c_str());
        item->setFlags(Qt::ItemIsEnabled);
        tableWidget->setItem(col.second.id, 0, item);
        if (disabled)
            item->setFlags(Qt::NoItemFlags);

        item = new QTableWidgetItem(getColumnTypeString(col.second.type).c_str());
        item->setFlags(Qt::NoItemFlags);
        tableWidget->setItem(col.second.id, 1, item);
        if (disabled)
            item->setFlags(Qt::NoItemFlags);

        item = new QTableWidgetItem((const char *)data->getVariableString(col.second.id));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        tableWidget->setItem(col.second.id, 2, item);
        item->setData(Qt::UserRole, (uint64_t)&col.second);
        if (disabled)
            item->setFlags(Qt::NoItemFlags);
    }
}

void MainWindow::currentTableWidgetItemChanged(QTableWidgetItem *item)
{
    if (!item || item->row() == -1 || item->column() != 2)
        return;
    currentTableWidgetItem = item;
}

void MainWindow::tableWidgetStartEdiding(QWidget *editor, const QModelIndex &index)
{
    using namespace polygon4;

    if (!currentTableWidgetItem)
        return;

    Column *column = (Column *)currentTableWidgetItem->data(Qt::UserRole).toULongLong();
    if (!column)
        return;

    QValidator *validator = 0;
    switch (column->type)
    {
    case ColumnType::Integer:
        {
            static QIntValidator intValidator;
            validator = &intValidator;
        }
        break;
    case ColumnType::Real:
        {
            static QDoubleValidator doubleValidator;
            doubleValidator.setLocale(QLocale::c());
            validator = &doubleValidator;
        }
        break;
    default:
        break;
    }
    
    QLineEdit *edit = (QLineEdit *)editor;
    edit->setValidator(validator);
}

void MainWindow::tableWidgetEndEdiding(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index)
{
    using namespace polygon4::detail;

    IObject *data = (IObject *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
    auto &table = schema->tables[polygon4::detail::getTableNameByType(data->getType())];

    data->setVariableString(currentTableWidgetItem->row(), currentTableWidgetItem->data(Qt::DisplayRole).toString().toStdString());

    dataChanged = true;
    setTitle();
}