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

void updateText(QTreeWidgetItem *item, bool no_children = false)
{
    using namespace polygon4::detail;

    if (!item)
        return;
    auto data = (TreeItem *)item->data(0, Qt::UserRole).toULongLong();
    item->setText(0, data->get_name());
    if (!no_children)
    {
        for (int i = 0; i < item->childCount(); i++)
            updateText(item->child(i));
    }
}

void updateTextAndParent(QTreeWidgetItem *item)
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

    openDbNoLoadAction = new QAction(QIcon(":/icons/open.png"), 0, this);
    connect(openDbNoLoadAction, &QAction::triggered, [=]
    {
        openDb(false, false);
    });

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
    fileMenu->addAction(openDbNoLoadAction);
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
    connect(treeWidget, &QTreeWidget::expanded,  [this](const QModelIndex &index)
    {
        if (!index.isValid())
            return;
        auto c = (QTreeWidgetItem *)index.internalPointer();
        if (!c)
            return;
        auto data = (polygon4::TreeItem *)c->data(0, Qt::UserRole).toULongLong();
        buildTree(data, false, 0);
    });
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
    openDbNoLoadAction->setText(tr("Open database without loading..."));
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

void MainWindow::buildTree(polygon4::TreeItem *item, bool recurse, int depth)
{
    if (item->children.empty()) // nothing to add
        return;
    if (item->flags[fNoChildren]) // denied to print children
        return;
    auto qitem = (QTreeWidgetItem*)item->guiItem;
    if (qitem->childCount())
    {
        // already populated, just build children
        for (int i = 0; i < qitem->childCount(); i++)
        {
            auto qchild = qitem->child(i);
            auto data = (polygon4::TreeItem *)qchild->data(0, Qt::UserRole).toULongLong();
            if (recurse || depth <= 0)
                buildTree(data, recurse, depth + 1);
        }
    }
    else
    {
        for (auto &c : item->children)
        {
            auto item = new QTreeWidgetItem(qitem);
            c->guiItem = item;
            item->setData(0, Qt::UserRole, (uint64_t)c.get());
            if (recurse || depth <= 0)
                buildTree(c.get(), recurse, depth + 1);
            item->setText(0, c->get_name()); // after children init
        }
        qitem->sortChildren(0, Qt::AscendingOrder);
    }
}

QTreeWidgetItem *MainWindow::addItem(polygon4::detail::TreeItem *item)
{
    auto qitem = (QTreeWidgetItem*)item->parent->guiItem;
    auto root = new QTreeWidgetItem(qitem, QStringList(item->name.toQString()));
    item->guiItem = root;
    root->setData(0, Qt::UserRole, (uint64_t)item);
    buildTree(item);
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
            polygon4::getCurrentLocalizationId(polygon4::LocalizationType::ru);
        else if (language.indexOf("en") != -1)
            polygon4::getCurrentLocalizationId(polygon4::LocalizationType::en);
    }
    else
        polygon4::getCurrentLocalizationId(polygon4::LocalizationType::en);
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

void MainWindow::openDb(bool create, bool load)
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
        auto db = std::make_shared<polygon4::Database>(filename);
        database = db;
    }
    catch (std::exception &e)
    {
        QMessageBox::critical(this, tr("Critical error while opening database!"), e.what());
        return;
    }

    if (load)
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
        storage.reset();
        database.reset();
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
        tree->guiItem = treeWidget->invisibleRootItem();
        buildTree(tree.get(), false);
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

void MainWindow::printVariable(
    Variable &var,
    int &row_id,
    polygon4::detail::TreeItem *data,
    const Class &cls)
{
    using namespace polygon4;
    using namespace polygon4::detail;

    row_id++;
    int col_id = 0;
    QTableWidgetItem *item;

    bool disabled = data->flags[fReadOnly] ? true : false;
    if (var.isId())
        disabled = true;

    // name
    item = new QTableWidgetItem(polygon4::tr(var.getDisplayName()).toQString());
    item->setFlags(Qt::ItemIsEnabled);
    tableWidget->setItem(row_id, col_id++, item);
    if (disabled)
        item->setFlags(Qt::NoItemFlags);

    // type
    item = new QTableWidgetItem(polygon4::tr(var.getDataType()).toQString());
    item->setFlags(Qt::NoItemFlags);
    tableWidget->setItem(row_id, col_id++, item);
    if (disabled)
        item->setFlags(Qt::NoItemFlags);

    // value
    auto value = data->object->getVariableString(var.getId());
    if (var.isFk())
    {
        if (cls.getParent() && var.getName() == cls.getParentVariable().getName())
        {
            item = new QTableWidgetItem(value.toQString());
            item->setFlags(Qt::NoItemFlags);
            tableWidget->setItem(row_id, col_id++, item);
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
            if (disabled)
                cb->setEnabled(false);
            tableWidget->setCellWidget(row_id, col_id, cb);
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

        tableWidget->setCellWidget(row_id, col_id++, wdg);
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
        tableWidget->setCellWidget(row_id, col_id, cb);
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
            updateTextAndParent(currentTreeWidgetItem);
        });
        tableWidget->setCellWidget(row_id, col_id++, te);
        tableWidget->setRowHeight(row_id, te->height() + 2);
    }
    else
    {
        item = new QTableWidgetItem(value.toQString());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        tableWidget->setItem(row_id, col_id++, item);
        item->setData(Qt::UserRole, (uint64_t)schemaMm.create<Variable>(var));
        if (disabled)
            item->setFlags(Qt::NoItemFlags);
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

    updateTextAndParent(currentTreeWidgetItem);

    auto data = (TreeItem *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
    auto &cls = data->object->getClass();

    auto vars = cls.getVariables()({ fInline }, true);
    std::sort(vars.begin(), vars.end(),
        [](const auto &v1, const auto &v2) { return v1.getId() < v2.getId(); });

    int row_id = -1;

    tableWidget->setRowCount(vars.size());
    for (size_t i = 0; i < vars.size(); i++)
    {
        tableWidget->removeCellWidget(i, tableWidgetColumnCount - 1);
        tableWidget->setItem(i, 2, nullptr);
        tableWidget->setRowHeight(i, 24);
    }

    for (auto &var : vars)
    {
        printVariable(var, row_id, data, cls);
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

    auto var = (Variable *)currentTableWidgetItem->data(Qt::UserRole).toULongLong();
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

    auto var = (Variable *)currentTableWidgetItem->data(Qt::UserRole).toULongLong();
    auto data = (TreeItem *)currentTreeWidgetItem->data(0, Qt::UserRole).toULongLong();
    data->object->setVariableString(var->getId(), currentTableWidgetItem->data(Qt::DisplayRole).toString().toStdString());
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
    auto data = (polygon4::TreeItem *)item->data(0, Qt::UserRole).toULongLong();
    auto r = storage->addRecord(data);
    if (!r)
    {
        auto items = treeWidget->selectedItems();
        if (items.size())
            currentTreeWidgetItemChanged(items[0], 0);
        return;
    }
    auto new_item = addItem(r.get());
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
    if (!storage->deleteRecord(data))
        return;
    if (data->inlineVariable)
    {
        auto items = treeWidget->selectedItems();
        if (items.size())
        {
            updateText(items[0]);
            currentTreeWidgetItemChanged(items[0], 0);
        }
        return;
    }
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
