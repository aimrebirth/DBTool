#include "mainwindow.h"

#include <qaction.h>
#include <qboxlayout.h>
#include <qcombobox.h>
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
#include <qtoolbar.h>
#include <qtreewidget.h>

#include <qapplication.h>
#include <qtranslator.h>

#include <Polygon4/Database.h>
#include <Polygon4/DatabaseSchema.h>
#include <Polygon4/Helpers.h>
#include <Polygon4/Storage.h>
#include <Polygon4/Types.h>

#include "version.h"

#include "qlabeledlistwidget.h"
#include "qsignalleditemdelegate.h"

QTranslator appTranslator;

void updateText(QTreeWidgetItem *item)
{
    using namespace polygon4::detail;

    for (int i = 0; i < item->childCount(); i++)
    {
        auto child = item->child(i);
        IObject *data = (IObject *)child->data(0, Qt::UserRole).toULongLong();
        if ((size_t)data > (1 << 20))
            child->setText(0, QString::fromStdWString(data->getName().wstring()));
        updateText(child);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    
    setMinimumSize(800, 600);
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
    createToolBar();
    createLayouts();

    retranslateUi();
}

void MainWindow::createActions()
{
    openDbAction = new QAction(QIcon(":/icons/open.png"), 0, this);
    connect(openDbAction, SIGNAL(triggered()), SLOT(openDb()));

    saveDbAction = new QAction(QIcon(":/icons/save.png"), 0, this);
    connect(saveDbAction, SIGNAL(triggered()), SLOT(saveDb()));

    reloadDbAction = new QAction(QIcon(":/icons/reload.png"), 0, this);
    connect(reloadDbAction, SIGNAL(triggered()), SLOT(saveDb()));
    connect(reloadDbAction, SIGNAL(triggered()), SLOT(loadStorage()));

    addRecordAction = new QAction(QIcon(":/icons/plus.png"), 0, this);
    connect(addRecordAction, SIGNAL(triggered()), SLOT(addRecord()));

    deleteRecordAction = new QAction(QIcon(":/icons/minus.png"), 0, this);
    connect(deleteRecordAction, SIGNAL(triggered()), SLOT(deleteRecord()));

    exitAction = new QAction(this);
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    aboutAction = new QAction(this);
    connect(aboutAction, &QAction::triggered, [=]
    {
        QMessageBox::information(this, tr("Polygon-4 DB Tool"),
            tr("Author") + ": lzwdgc" + ", 2015\n" + tr("Version") +
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
    fileMenu->addAction(reloadDbAction);
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

void MainWindow::createToolBar()
{
    QToolBar *toolBar = new QToolBar(this);
    toolBar->addAction(openDbAction);
    toolBar->addAction(saveDbAction);
    toolBar->addAction(reloadDbAction);
    toolBar->addSeparator();
    toolBar->addAction(addRecordAction);
    toolBar->addAction(deleteRecordAction);
    addToolBar(toolBar);
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
    tableWidget->horizontalHeader()->setStretchLastSection(true);
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
    reloadDbAction->setText(tr("Reload database..."));
    exitAction->setText(tr("Exit"));
    addRecordAction->setText(tr("Add record"));
    deleteRecordAction->setText(tr("Delete record"));
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
        QString language = action->data().toString();
        QApplication::installTranslator(&appTranslator);
        appTranslator.load(language);
        if (language.indexOf("ru") != -1)
            polygon4::gCurrentLocalizationId = static_cast<int>(polygon4::detail::LocalizationType::ru);
    }
    else
        polygon4::gCurrentLocalizationId = static_cast<int>(polygon4::detail::LocalizationType::en);
    reloadTreeView();
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
    std::string filename
#ifndef NDEBUG
        = "h:\\Games\\Epic Games\\Projects\\Polygon4\\Mods\\db.sqlite"
#endif
        ;

    if (filename.empty())
    {
        QSettings settings;
        QString dir = settings.value("openDir", ".").toString();
        QString fn = QFileDialog::getOpenFileName(this, tr("Open file"), dir, tr("sqlite3 database") + " (*.sqlite)");
        if (fn == QString())
            return;
        settings.setValue("openDir", QFileInfo(fn).absoluteDir().absolutePath());
        filename = fn.toStdString();
    }

    try
    {
        database = std::make_shared<polygon4::Database>(filename);
    }
    catch (std::exception e)
    {
        QMessageBox::critical(this, tr("Critical error while opening database!"), e.what());
        return;
    }

    loadStorage();
}

void MainWindow::loadStorage()
{
    if (!database)
        return;

    try
    {
        storage = polygon4::initStorage(database);
        storage->load();
    }
    catch (std::exception e)
    {
        QMessageBox::critical(this, tr("Critical error while loading the storage!"), e.what());
        return;
    }

    setTitle();

    dataChanged = false;
    delete schema;
    schema = new polygon4::DatabaseSchema;
    database->getSchema(schema);

    reloadTreeView();
}

void MainWindow::reloadTreeView()
{
    treeWidget->clear();
    storage->printQtTreeView(treeWidget->invisibleRootItem());
    treeWidget->invisibleRootItem()->sortChildren(0, Qt::AscendingOrder);
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

    tableWidget->setCurrentCell(-1, -1);
    if (!current || !current->parent() || !current->data(0, Qt::UserRole).toULongLong())
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
    for (size_t i = 0; i < table.columns.size(); i++)
        tableWidget->removeCellWidget(i, 2);
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

        auto value = data->getVariableString(col.second.id);
        if (col.second.fk)
        {
            static std::map<void *, Ptr<IObject>> ptr_storage;
            ptr_storage.clear();

            auto m = storage->getOrderedMap(polygon4::detail::getTableType(col.second.fk->table_name));
            QComboBox *cb = new QComboBox;
            bool found = false;
            for (auto &v : m)
            {
                ptr_storage[v.second.get()] = v.second;
                cb->addItem(QString::fromStdWString(v.first), (uint64_t)v.second.get());
                if (value == v.second->getName())
                {
                    cb->setCurrentIndex(cb->count() - 1);
                    found = true;
                }
            }
            if (!found)
            {
                cb->addItem("");
                cb->setCurrentIndex(cb->count() - 1);
            }
            connect(cb, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, [cb, col, this](int index)
            {
                IObject *data = (IObject *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
                IObject *cb_data = (IObject *)cb->currentData().toULongLong();
                if (cb_data)
                {
                    data->setVariableString(col.second.id, "", ptr_storage[cb_data]);
                    updateText(treeWidget->invisibleRootItem());
                }
            });
            tableWidget->setCellWidget(col.second.id, 2, cb);
            continue;
        }
        item = new QTableWidgetItem(QString::fromStdWString(value));
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

    updateText(treeWidget->invisibleRootItem());
}

void MainWindow::addRecord()
{
    auto item = treeWidget->currentItem();
    if (!item)
        return;
    while (item->parent())
        item = item->parent();
    auto new_item = storage->addRecord(item);
    treeWidget->setCurrentItem(new_item);

    dataChanged = true;
    setTitle();
}

void MainWindow::deleteRecord()
{
    auto item = treeWidget->currentItem();
    if (!item || !item->parent())
        return;
    while (item->parent()->parent())
        item = item->parent();
    storage->deleteRecord(item);

    dataChanged = true;
    setTitle();
}

