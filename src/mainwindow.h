#pragma once

#include <memory>

#include <QMainWindow>

#include <Polygon4/DataManager/MemoryManager.h>

class QWidget;
class QMenuBar;
class QMenu;
class QActionGroup;
class QAction;
class QPushButton;
class QPlainTextEdit;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QProgressDialog;
class QScrollArea;
class QStatusBar;

class QAbstractItemModel;
class QTableWidget;
class QTableWidgetItem;
class QTreeWidget;
class QTreeWidgetItem;

class QLabeledListWidget;
class QSignalledItemDelegate;

namespace polygon4
{
    namespace detail
    {
        class Storage;
        struct TreeItem;
    }
    struct Table;
    class Database;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
protected:
    void changeEvent(QEvent* event);
    void closeEvent(QCloseEvent *event);
    
signals:
private slots:
    void changeLanguage(QAction *action);
    void openDb(bool create = false);
    void saveDb();
    void loadStorage(bool create = false);
    void reloadTreeView();
    void currentTreeWidgetItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void currentTableWidgetItemChanged(QTableWidgetItem *item);
    void tableWidgetStartEdiding(QWidget *editor, const QModelIndex &index);
    void tableWidgetEndEdiding(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index);
    void addRecord();
    void deleteRecord();

private:
    void setupUi();
    void retranslateUi();
    
private: /* ui functions */
    void createActions();
    void createLanguageMenu();
    void createMenus();
    void createButtons();
    void createLayouts();
    void createWidgets();
    void createToolBar();
    void setTitle();
    void setTableHeaders();
    void buildTree(polygon4::detail::TreeItem *item, bool recurse = true, int depth = 0);
    QTreeWidgetItem *addItem(polygon4::detail::TreeItem *item);

private: /* ui components */
    QWidget *centralWidget;
    QStatusBar *statusBar;
    QProgressDialog *progressDialog;

    QMenuBar *mainMenu;
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *settingsMenu;
    QMenu *languageMenu;
    QMenu *helpMenu;

    QActionGroup *languageActionGroup;
    QAction *newDbAction;
    QAction *openDbAction;
    QAction *saveDbAction;
    QAction *saveDbAsAction;
    QAction *reloadDbAction;
    QAction *exitAction;
    QAction *aboutAction;
    QAction *addRecordAction;
    QAction *deleteRecordAction;

    QAction *dumpDbAction;
    QAction *loadDbAction;

    QLabel *dbLabel;

    QHBoxLayout *mainLayout;
    QHBoxLayout *leftLayout;
    QVBoxLayout *rightLayout;

    QTreeWidget *treeWidget;
    QTableWidget *tableWidget;

    QTreeWidgetItem *currentTreeWidgetItem = 0;
    QTableWidgetItem *currentTableWidgetItem = 0;
    QSignalledItemDelegate *signalledItemDelegate = 0;

private: /* data */
    std::shared_ptr<polygon4::Database> database;
    std::shared_ptr<polygon4::detail::Storage> storage;
    MemoryManager schemaMm;
    bool dataChanged = false;
    std::shared_ptr<polygon4::detail::TreeItem> tree;

    void printVariable(
            class Variable &var,
            int &row_id,
            polygon4::detail::TreeItem *data,
            const class Class &cls);
};