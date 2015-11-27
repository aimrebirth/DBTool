#include "mainwindow.h"

#include <algorithm>
#include <assert.h>

#include <QtWidgets>

#include <qapplication.h>
#include <qtranslator.h>

#include <Polygon4/DataManager/Common.h>
#include <Polygon4/DataManager/Database.h>
#include <Polygon4/DataManager/Storage.h>
#include <Polygon4/DataManager/StorageImpl.h>
#include <Polygon4/DataManager/Types.h>

#include "version.h"

#include "qlabeledlistwidget.h"
#include "qsignalleditemdelegate.h"

#define ALLOC(x) x = new std::remove_reference<decltype(*x)>::type

QTranslator appTranslator;

const int tableWidgetColumnCount = 3;

void setItemText(QTreeWidgetItem *item, const polygon4::String &text)
{
    QString t = polygon4::tr(text).toQString();
    auto ti = ((polygon4::TreeItem *)item->data(0, Qt::UserRole).toULongLong());
    int cc = ti->child_count();
    if (cc)
        t += " [" + QString::number(cc) + "]";
    item->setText(0, t);
}

void updateText(QTreeWidgetItem *item, bool no_children = false)
{
    using namespace polygon4::detail;

    if (!item)
        return;
    auto data = (TreeItem *)item->data(0, Qt::UserRole).toULongLong();
    setItemText(item, data->name);
    if (!no_children)
    {
        for (int i = 0; i < item->childCount(); i++)
            updateText(item->child(i));
    }
}

void updateTextAnParent(QTreeWidgetItem *item)
{
    // probably this won't update higher parents
    updateText(item->parent(), true);
    updateText(item);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    
    setMinimumSize(1000, 600);
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
        if (!database || database->getFullName().empty())
            return;
        QString tables;
        tables = QInputDialog::getText(this, windowTitle(), tr("Enter tables to dump. * is for all tables."));
        if (tables.isEmpty())
            return;
        if (tables == "*")
            tables.clear();
        else
            tables = "--tables " + tables;
        auto fn = QFileDialog::getSaveFileName(this, tr("Dump database"), QString(), tr("Json files") + " (*.json)");
        if (fn.isEmpty())
            return;
        std::string cmd = "python dbmgr.py --dump --json \"" + fn.toStdString() + "\" --db \"" + database->getFullName() + "\" " + tables.toStdString();
        system(cmd.c_str());
    });

    ALLOC(loadDbAction)(QIcon(":/icons/load.png"), 0, this);
    connect(loadDbAction, &QAction::triggered, [=]
    {
        if (!database || database->getFullName().empty())
            return;
        auto fn = QFileDialog::getOpenFileName(this, tr("Load database"), QString(), tr("Json files") + " (*.json)");
        if (fn.isEmpty())
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
        auto item = new QTreeWidgetItem(qitem);
        item->setData(0, Qt::UserRole, (uint64_t)c.get());
        buildTree(item, c.get());
        setItemText(item, c->name);
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
            polygon4::gCurrentLocalizationId = polygon4::LocalizationType::ru;
        else if (language.indexOf("en") != -1)
            polygon4::gCurrentLocalizationId = polygon4::LocalizationType::en;
    }
    else
        polygon4::gCurrentLocalizationId = polygon4::LocalizationType::en;
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
        //= R"(k:\AIM\Polygon4\Mods\db.sqlite)"
#endif
        ;

    if (filename.empty())
    {
        QSettings settings;
        QString dir = settings.value("openDir", ".").toString();
        QString fn;
        if (create)
            fn = QFileDialog::getSaveFileName(this, tr("Open file"), dir, tr("sqlite3 database") + " (*.sqlite)");
        else
            fn = QFileDialog::getOpenFileName(this, tr("Open file"), dir, tr("sqlite3 database") + " (*.sqlite)");
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
            if (database && filename == database->getFullName())
            {
                storage.reset();
                database.reset();
            }
            if (f.exists() && !f.remove())
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

    dataChanged = false;
    schemaMm.clear();

    setTitle();
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
    using namespace polygon4;
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

    updateTextAnParent(currentTreeWidgetItem);

    auto data = (TreeItem *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
    auto table_name = data->object->getClass().getCppName();
    auto classes = schema->getClasses();
    auto citer = classes.find(table_name);
    if (citer == classes.end())
        return;
    auto vars = citer->getVariables();
    tableWidget->setRowCount(vars.size());
    for (size_t i = 0; i < vars.size(); i++)
    {
        tableWidget->removeCellWidget(i, tableWidgetColumnCount - 1);
        tableWidget->setItem(i, 2, nullptr);
    }
    for (auto &var : vars)
    {
        int col_id = 0;
        QTableWidgetItem *item;

        bool disabled = false;
        if (var.isId())
            disabled = true;

        // name
        item = new QTableWidgetItem(polygon4::tr(var.getDisplayName()).toQString());
        item->setFlags(Qt::ItemIsEnabled);
        tableWidget->setItem(var.getId(), col_id++, item);
        if (disabled)
            item->setFlags(Qt::NoItemFlags);

        // type
        item = new QTableWidgetItem(polygon4::tr(var.getType()->getDataType()).toQString());
        item->setFlags(Qt::NoItemFlags);
        tableWidget->setItem(var.getId(), col_id++, item);
        if (disabled)
            item->setFlags(Qt::NoItemFlags);

        // value
        auto value = data->object->getVariableString(var.getId());
        if (var.isFk())
        {
            if (citer->getParent() && var.getName() == citer->getParentVariable().getName())
            {
                item = new QTableWidgetItem(value.toQString());
                item->setFlags(Qt::NoItemFlags);
                tableWidget->setItem(var.getId(), col_id++, item);
            }
            else
            {
                bool set = false;
                OrderedObjectMap m;
                std::tie(set, m) = data->object->getOrderedObjectMap(var.getId(), storage.get());
                
                bool enabled = set ? !m.empty() : true;

                QComboBox *cb = new QComboBox;
                cb->setEnabled(enabled);
                if (enabled)
                {
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
                    connect(cb, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, [cb, var, this](int index)
                    {
                        auto data = (TreeItem *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
                        auto cb_data = (IObjectBase *)cb->currentData().toULongLong();
                        if (cb_data)
                        {
                            data->object->setVariableString(var.getId(), cb_data);
                            data->name = data->object->getName();
                            currentTreeWidgetItemChanged(currentTreeWidgetItem, 0);
                        }
                    });
                }
                tableWidget->setCellWidget(var.getId(), col_id, cb);
            }
        }
        else if (var.getDataType() == DataType::Bool)
        {
            auto chkb = new QCheckBox;
            chkb->setChecked(value.toString() == "1");
            connect(chkb, &QCheckBox::stateChanged, [chkb, var, data](int state)
            {
                data->object->setVariableString(var.getId(), chkb->isChecked() ? "1" : "0");
            });

            QWidget* wdg = new QWidget;
            QHBoxLayout layout(wdg);
            layout.addSpacing(3);
            layout.addWidget(chkb);
            layout.setAlignment(Qt::AlignLeft);
            layout.setMargin(0);
            wdg->setLayout(&layout);

            tableWidget->setCellWidget(var.getId(), col_id++, wdg);
            tableWidget->repaint();
        }
        else if (var.getDataType() == DataType::Enum)
        {
            QComboBox *cb = new QComboBox;
            auto n = var.getType()->getCppName();
            int val = std::stoi(value);
            auto m = getOrderedMap(n);
            bool found = false;
            for (auto &v : m)
            {
                cb->addItem(v.first.toQString(), (uint64_t)v.second);
                if (val == v.second)
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
            connect(cb, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged, [cb, var, this](int index)
            {
                auto data = (TreeItem *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
                auto cb_data = cb->currentData().toInt();
                data->object->setVariableString(var.getId(), std::to_string(cb_data));
                data->name = data->object->getName();
                currentTreeWidgetItemChanged(currentTreeWidgetItem, 0);
            });
            tableWidget->setCellWidget(var.getId(), col_id, cb);
        }
        else if (var.hasFlags({ fBigEdit }))
        {
            auto te = new QTextEdit;
            te->setAcceptRichText(false);
            te->setFixedHeight(100);
            te->setPlainText(value.toQString());
            connect(te, &QTextEdit::textChanged, [te, var, data, this]()
            {
                data->object->setVariableString(var.getId(), te->toPlainText());
                data->name = data->object->getName();
                updateTextAnParent(currentTreeWidgetItem);
            });
            tableWidget->setCellWidget(var.getId(), col_id++, te);
            tableWidget->setRowHeight(var.getId(), te->height() + 2);
        }
        else
        {
            item = new QTableWidgetItem(value.toQString());
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            tableWidget->setItem(var.getId(), col_id++, item);
            item->setData(Qt::UserRole, (uint64_t)schemaMm.create<Variable>(var));
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

    Variable *var = (Variable *)currentTableWidgetItem->data(Qt::UserRole).toULongLong();
    if (!var)
        return;

    QValidator *validator = 0;
    switch (var->getDataType())
    {
    case DataType::Integer:
        {
            static QIntValidator intValidator;
            validator = &intValidator;
        }
        break;
    case DataType::Real:
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
    data->object->setVariableString(currentTableWidgetItem->row(), currentTableWidgetItem->data(Qt::DisplayRole).toString().toStdString());
    data->name = data->object->getName();

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
        if ((data && data->object) || data->type == polygon4::detail::EObjectType::None)
            item = item->parent();
        else
            break;
    }
    auto data = (polygon4::TreeItem *)item->data(0, Qt::UserRole).toULongLong();
    auto r = storage->addRecord(data);
    if (!r)
        return;
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
    {
        p->removeChild(item);
        updateText(p, true);
    }

    dataChanged = true;
    setTitle();
}
