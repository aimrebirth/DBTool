#include "mainwindow.h"

#include <algorithm>
#include <assert.h>

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
#include <qprogressbar.h>
#include <qprogressdialog.h>
#include <qscrollbar.h>
#include <qsettings.h>
#include <qsizegrip.h>
#include <qstatusbar.h>
#include <qtablewidget.h>
#include <qtimer.h>
#include <qtoolbar.h>
#include <qtreewidget.h>

#include <qapplication.h>
#include <qtranslator.h>

#include <Polygon4/DataManager/Common.h>
#include <Polygon4/DataManager/Database.h>
#include <Polygon4/DataManager/DatabaseSchema.h>
#include <Polygon4/DataManager/Helpers.h>
#include <Polygon4/DataManager/Storage.h>
#include <Polygon4/DataManager/StorageImpl.h>
#include <Polygon4/DataManager/Types.h>

#include "version.h"

#include "qlabeledlistwidget.h"
#include "qsignalleditemdelegate.h"

#define ALLOC(x) x = new std::remove_reference<decltype(*x)>::type

QTranslator appTranslator;

const int tableWidgetColumnCount = 3;

void updateText(QTreeWidgetItem *item)
{
    using namespace polygon4::detail;

    if (!item)
        return;
    auto data = (TreeItem *)item->data(0, Qt::UserRole).toULongLong();
    item->setText(0, QCoreApplication::translate("DB", to_string(data->name).c_str()));
    for (int i = 0; i < item->childCount(); i++)
        updateText(item->child(i));
}

QString getColumnTypeString(polygon4::ColumnType type)
{
    using polygon4::ColumnType;
    switch (type)
    {
    case ColumnType::Integer:
        return QCoreApplication::translate("DB Types", "INTEGER");
    case ColumnType::Real:
        return QCoreApplication::translate("DB Types", "REAL");
    case ColumnType::Text:
        return QCoreApplication::translate("DB Types", "TEXT");
    case ColumnType::Bool:
        return QCoreApplication::translate("DB Types", "BOOL");
    case ColumnType::Blob:
        return QCoreApplication::translate("DB Types", "BLOB");
    default:
        assert(false);
        break;
    }
    return "";
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
    //connect(reloadDbAction, SIGNAL(triggered()), SLOT(saveDb()));
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

    ALLOC(dumpDbAction)(QIcon(":/icons/dump.png"), 0, this);
    connect(dumpDbAction, &QAction::triggered, [=]
    {
        auto fn = QFileDialog::getSaveFileName(this, tr("Dump database"), QString(), tr("Json files") + " (*.json)");
        if (fn.isEmpty() || !database || database->getFullName().empty())
            return;
        QString tables;
        do tables = QInputDialog::getText(this, windowTitle(), tr("Enter tables to dump. * is for all tables."));
        while (tables.isEmpty());
        if (tables == "*")
            tables.clear();
        else
            tables = "--tables " + tables;
        std::string cmd = "python dbmgr.py --dump --json \"" + fn.toStdString() + "\" --db \"" + database->getFullName() + "\" " + tables.toStdString();
        system(cmd.c_str());
    });

    ALLOC(loadDbAction)(QIcon(":/icons/load.png"), 0, this);
    connect(loadDbAction, &QAction::triggered, [=]
    {
        auto fn = QFileDialog::getOpenFileName(this, tr("Load database"), QString(), tr("Json files") + " (*.json)");
        if (fn.isEmpty() || !database || database->getFullName().empty())
            return;
        std::string cmd = "python dbmgr.py --load --clear --json \"" + fn.toStdString() + "\" --db \"" + database->getFullName() + "\"";
        system(cmd.c_str());
        openDb();
    });

    ALLOC(newDbAction)(QIcon(":/icons/new.png"), 0, this);
    connect(newDbAction, &QAction::triggered, [=] { openDb(true); });

    ALLOC(saveDbAsAction)(QIcon(":/icons/save.png"), 0, this);
    connect(saveDbAsAction, &QAction::triggered, [=]
    {
        if (!storage)
            return;
        QSettings settings;
        QString dir = settings.value("openDir", ".").toString();
        QString fn = QFileDialog::getSaveFileName(this, tr("Save file"), dir, tr("sqlite3 database") + " (*.sqlite)");
        if (fn.isEmpty())
            return;
        QFile f(fn);
        if (f.exists() && !f.remove())
        {
            QMessageBox::critical(this, tr("Cannot remove old database file!"), f.errorString());
            return;
        }
        database = std::make_shared<polygon4::Database>(fn.toStdString());
        ((polygon4::detail::StorageImpl *)(storage.get()))->setDb(database);
        storage->create();
        saveDb();
    });
}

void MainWindow::createLanguageMenu()
{
    languageMenu = new QMenu(this);
    QAction *defaultApplicationLanguage = languageMenu->addAction("English");
    defaultApplicationLanguage->setCheckable(true);
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
        if (!translator.load(filename))
            continue;

        QString language = translator.translate("MainWindow", "English");
        if (language == "English")
        {
            defaultApplicationLanguage->setData(filename);
            continue;
        }
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
    fileMenu->addAction(newDbAction);
    fileMenu->addSeparator();
    fileMenu->addAction(openDbAction);
    fileMenu->addAction(saveDbAction);
    fileMenu->addAction(saveDbAsAction);
    fileMenu->addAction(reloadDbAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    ALLOC(editMenu)(this);
    editMenu->addAction(addRecordAction);
    editMenu->addAction(deleteRecordAction);
    editMenu->addSeparator();
    editMenu->addAction(dumpDbAction);
    editMenu->addAction(loadDbAction);

    settingsMenu = new QMenu(this);
    settingsMenu->addMenu(languageMenu);

    helpMenu = new QMenu(this);
    helpMenu->addAction(aboutAction);

    mainMenu = new QMenuBar(this);
    mainMenu->addMenu(fileMenu);
    mainMenu->addMenu(editMenu);
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
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addLayout(rightLayout, 5);

    centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = new QToolBar(this);
    toolBar->addAction(newDbAction);
    toolBar->addSeparator();
    toolBar->addAction(openDbAction);
    toolBar->addAction(saveDbAction);
    toolBar->addAction(reloadDbAction);
    toolBar->addSeparator();
    toolBar->addAction(addRecordAction);
    toolBar->addAction(deleteRecordAction);
    toolBar->addSeparator();
    toolBar->addAction(dumpDbAction);
    toolBar->addAction(loadDbAction);
    toolBar->addSeparator();
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
    tableWidget->setColumnCount(tableWidgetColumnCount);
    tableWidget->horizontalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setStretchLastSection(true);
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
    tableWidget->verticalHeader()->setDefaultSectionSize(24);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->setItemDelegateForColumn(tableWidgetColumnCount - 1, signalledItemDelegate);

    connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), SLOT(currentTreeWidgetItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
    connect(tableWidget, SIGNAL(itemClicked(QTableWidgetItem *)), SLOT(currentTableWidgetItemChanged(QTableWidgetItem *)));

    connect(signalledItemDelegate, SIGNAL(startEditing(QWidget *, const QModelIndex &)), SLOT(tableWidgetStartEdiding(QWidget *, const QModelIndex &)));
    connect(signalledItemDelegate, SIGNAL(endEditing(QWidget *, QAbstractItemModel *, const QModelIndex &)), SLOT(tableWidgetEndEdiding(QWidget *, QAbstractItemModel *, const QModelIndex &)));
    
    dbLabel = new QLabel(this);

    statusBar = new QStatusBar(this);
    statusBar->addWidget(dbLabel);
    statusBar->setSizeGripEnabled(false);
    setStatusBar(statusBar);
}

void MainWindow::retranslateUi()
{
    fileMenu->setTitle(tr("File"));
    newDbAction->setText(tr("New database..."));
    openDbAction->setText(tr("Open database..."));
    saveDbAction->setText(tr("Save database..."));
    saveDbAsAction->setText(tr("Save database as..."));
    reloadDbAction->setText(tr("Reload database..."));
    exitAction->setText(tr("Exit"));
    addRecordAction->setText(tr("Add record"));
    deleteRecordAction->setText(tr("Delete record"));
    settingsMenu->setTitle(tr("Settings"));
    languageMenu->setTitle(tr("Language"));
    helpMenu->setTitle(tr("Help"));
    aboutAction->setText(tr("About"));

    editMenu->setTitle(tr("Edit"));
    loadDbAction->setText(tr("Load json"));
    dumpDbAction->setText(tr("Dump json"));
    dumpDbAction->setIconText(dumpDbAction->text());
    
    setTableHeaders();
    setTitle();

    polygon4::detail::retranslateFieldNames();
    polygon4::detail::retranslateTableNames();
}

void MainWindow::setTableHeaders()
{
    QStringList headers;
    headers << tr("Name") << tr("Type") << tr("Value");
    tableWidget->setHorizontalHeaderLabels(headers);
}

void MainWindow::buildTree(QTreeWidgetItem *qitem, polygon4::TreeItem *item)
{
    if (item->children.empty())
        return;
    for (auto &c : item->children)
    {
        auto item = new QTreeWidgetItem(qitem, QStringList(QCoreApplication::translate("DB", to_string(c->name).c_str())));
        item->setData(0, Qt::UserRole, (uint64_t)c.get());
        buildTree(item, c.get());
    }
    qitem->sortChildren(0, Qt::AscendingOrder);
}

QTreeWidgetItem *MainWindow::addItem(QTreeWidgetItem *qitem, polygon4::detail::TreeItem *item)
{
    auto root = new QTreeWidgetItem(qitem, QStringList(item->name.toQString()));
    root->setData(0, Qt::UserRole, (uint64_t)item);
    buildTree(root, item);
    return root;
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
        else if (language.indexOf("en") != -1)
            polygon4::gCurrentLocalizationId = static_cast<int>(polygon4::detail::LocalizationType::en);
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

void MainWindow::openDb(bool create)
{
    std::string filename
#ifndef NDEBUG
        = R"(k:\AIM\Polygon4\Mods\db.sqlite)"
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
        if (create)
        {
            QFile f(filename.c_str());
            if (f.exists())
            {
                if (QMessageBox::warning(this, tr("Confirm file overwrite"), tr("Do you want to overwrite selected file?"), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Cancel)
                    return;
            }
            if (database && filename == database->getFullName())
            {
                storage.reset();
                database.reset();
            }
            if (!f.remove())
            {
                QMessageBox::critical(this, tr("Cannot remove old database file!"), f.errorString());
                return;
            }
        }
        database = std::make_shared<polygon4::Database>(filename);
    }
    catch (std::exception &e)
    {
        QMessageBox::critical(this, tr("Critical error while opening database!"), e.what());
        return;
    }

    loadStorage(create);
}

void MainWindow::loadStorage(bool create)
{
    if (!database)
        return;

    try
    {
        storage = polygon4::initStorage(database);
        if (create)
            storage->create();

        QProgressDialog progress(tr("Opening database..."), "Abort", 0, 100, this, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
        progress.setFixedWidth(400);
        progress.setFixedHeight(75);
        progress.setCancelButton(0);
        progress.setValue(0);
        progress.setWindowModality(Qt::WindowModal);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        auto f = [&](double p)
        {
            progress.setValue((int)p);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        };

        storage->load(f);
        dbLabel->setText(database->getFullName().c_str());
    }
    catch (std::exception &e)
    {
        QMessageBox::critical(this, tr("Critical error while loading the storage!"), e.what());
        return;
    }

    setTitle();

    dataChanged = false;
    schema = database->getSchema();

    reloadTreeView();
}

void MainWindow::reloadTreeView()
{
    tableWidget->setCurrentCell(-1, -1);
    tableWidget->setRowCount(0);
    tableWidget->horizontalHeader()->setVisible(false);
    treeWidget->setCurrentItem(treeWidget->invisibleRootItem());
    currentTableWidgetItem = 0;
    currentTreeWidgetItem = 0;
    treeWidget->clear();
    if (storage)
    {
        tree = storage->printTree();
        buildTree(treeWidget->invisibleRootItem(), tree.get());
    }
}

void MainWindow::saveDb()
{
    try
    {
        if (!storage)
            return;

        QProgressDialog progress(tr("Saving database..."), "Abort", 0, 100, this, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
        progress.setFixedWidth(400);
        progress.setFixedHeight(75);
        progress.setCancelButton(0);
        progress.setValue(0);
        progress.setWindowModality(Qt::WindowModal);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        auto f = [&](double p)
        {
            progress.setValue((int)p);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        };

        storage->save(f);
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
    if (!current || !current->parent() || !((TreeItem *)current->data(0, Qt::UserRole).toULongLong())->object)
    {
        tableWidget->setRowCount(0);
        tableWidget->horizontalHeader()->setVisible(false);
        return;
    }
    setTableHeaders();
    tableWidget->horizontalHeader()->setVisible(true);

    currentTreeWidgetItem = current;
    currentTableWidgetItem = 0;

    updateText(currentTreeWidgetItem);

    auto data = (TreeItem *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
    auto table_name = to_string(polygon4::detail::getTableNameByType(data->type));
    auto table_iter = schema->tables.find(table_name);
    if (table_iter == schema->tables.end())
        return;
    auto &table = schema->tables.find(table_name)->second;
    tableWidget->setRowCount(table.columns.size());
    for (size_t i = 0; i < table.columns.size(); i++)
        tableWidget->removeCellWidget(i, tableWidgetColumnCount - 1);
    for (auto &col : table.columns)
    {
        int col_id = 0;
        QTableWidgetItem *item;

        bool disabled = false;
        if (col.second.id == 0 && col.second.name == "id" && col.second.type == polygon4::ColumnType::Integer)
            disabled = true;

        // name
        item = new QTableWidgetItem(getFieldName(col.second.name));
        item->setFlags(Qt::ItemIsEnabled);
        tableWidget->setItem(col.second.id, col_id++, item);
        if (disabled)
            item->setFlags(Qt::NoItemFlags);

        // type
        item = new QTableWidgetItem(col.second.fk ? "" : getColumnTypeString(col.second.type));
        item->setFlags(Qt::NoItemFlags);
        tableWidget->setItem(col.second.id, col_id++, item);
        if (disabled)
            item->setFlags(Qt::NoItemFlags);

        // value
        auto value = data->object->getVariableString(col.second.id);
        if (col.second.fk)
        {
            std::string s1 = removeId(col.first);
            std::string s2 = table.name;
            std::transform(s1.begin(), s1.end(), s1.begin(), tolower);
            std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
            if (s2.find(s1) == 0)
            {
                item = new QTableWidgetItem(value.toQString());
                item->setFlags(Qt::NoItemFlags);
                tableWidget->setItem(col.second.id, col_id++, item);
            }
            else
            {
                QComboBox *cb = new QComboBox;

                auto table_type = getTableType(table.name);
                auto type = getTableType(col.second.fk->table_name);

                OrderedObjectMap m;

                // prevent printing objects from other maps
                auto f = [](auto m, auto mo) { return m->map == mo->map; };
                auto f2 = [](auto m, auto mo) { return m->modification == mo->modification && m->map == mo->map; };
                if (type == EObjectType::MapBuilding)
                    m = getOrderedMapForObject<MapBuilding>(storage, (Mechanoid *)data->object, f);
                else if (type == EObjectType::MapGood)
                    m = getOrderedMapForObject<MapGood>(storage, (Mechanoid *)data->object, f);
                else if (type == EObjectType::MapObject)
                    m = getOrderedMapForObject<MapObject>(storage, (Mechanoid *)data->object, f);
                else if (type == EObjectType::ModificationMap)
                    m = getOrderedMapForObject<ModificationMap>(storage, (Mechanoid *)data->object, f2);
                else
                    m = storage->getOrderedMap(type);

                if (type == EObjectType::String)
                {
                    for (auto it = m.cbegin(); it != m.cend(); )
                    {
                        polygon4::detail::String *s = (polygon4::detail::String *)it->second;
                        if (s->table &&
                            (s->table->getId() != static_cast<int>(table_type) || s->table->getId() == static_cast<int>(EObjectType::Any)))
                            m.erase(it++);
                        else
                            ++it;
                    }
                }
                if (type == EObjectType::Table)
                {
                    decltype(m) m2;
                    for (auto it = m.cbegin(); it != m.cend(); it++)
                        m2.insert(std::make_pair(getTableName(to_string(it->first)), it->second));
                    m = m2;
                }

                bool found = false;
                for (auto &v : m)
                {
                    cb->addItem(v.first.toQString(), (uint64_t)v.second);
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
                    auto data = (TreeItem *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
                    auto cb_data = (IObjectBase *)cb->currentData().toULongLong();
                    if (cb_data)
                    {
                        data->object->setVariableString(col.second.id, cb_data);
                        data->name = data->object->getName();
                        currentTreeWidgetItemChanged(currentTreeWidgetItem, 0);
                    }
                });
                tableWidget->setCellWidget(col.second.id, col_id, cb);
            }
        }
        else
        {
            item = new QTableWidgetItem(value.toQString());
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            tableWidget->setItem(col.second.id, col_id++, item);
            item->setData(Qt::UserRole, (uint64_t)&col.second);
            if (disabled)
                item->setFlags(Qt::NoItemFlags);
        }
    }
}

void MainWindow::currentTableWidgetItemChanged(QTableWidgetItem *item)
{
    if (!item || item->row() == -1 || item->column() != tableWidgetColumnCount - 1)
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

    if (!currentTreeWidgetItem)
        return;

    auto data = (TreeItem *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
    auto &table = schema->tables.find(to_string(polygon4::detail::getTableNameByType(data->type)))->second;

    data->object->setVariableString(currentTableWidgetItem->row(), currentTableWidgetItem->data(Qt::DisplayRole).toString().toStdString());

    dataChanged = true;
    setTitle();

    currentTreeWidgetItemChanged(currentTreeWidgetItem, 0);
}

void MainWindow::addRecord()
{
    auto item = treeWidget->currentItem();
    if (!item)
        return;
    while (1)
    {
        auto data = (polygon4::TreeItem *)item->data(0, Qt::UserRole).toULongLong();
        if (data && data->object)
            item = item->parent();
        else
            break;
    }
    auto data = (polygon4::TreeItem *)item->data(0, Qt::UserRole).toULongLong();
    auto r = storage->addRecord(data);
    auto new_item = addItem(item, r.get());
    treeWidget->setCurrentItem(new_item);
    new_item->setExpanded(true);

    dataChanged = true;
    setTitle();
}

void MainWindow::deleteRecord()
{
    auto item = treeWidget->currentItem();
    if (!item)
        return;
    auto data = (polygon4::TreeItem *)item->data(0, Qt::UserRole).toULongLong();
    if (!data || !data->object)
        return;
    storage->deleteRecord(data);
    data->remove();
    auto p = item->parent();
    if (p)
        p->removeChild(item);

    dataChanged = true;
    setTitle();
}

