#pragma once

#include <memory>

#include <QMainWindow>

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
class QScrollArea;

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
    }
    struct Table;
    class Database;
    struct DatabaseSchema;
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

    void addToTreeView(QTreeWidgetItem *parent, const polygon4::Table &table);
    
signals:
private slots:
    void changeLanguage(QAction *action);
    void openDb();
    void saveDb();
    void loadStorage();
    void currentTreeWidgetItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void currentTableWidgetItemChanged(QTableWidgetItem *item);
    void tableWidgetStartEdiding(QWidget *editor, const QModelIndex &index);
    void tableWidgetEndEdiding(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index);

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

private: /* ui components */
    QWidget *centralWidget;

    QMenuBar *mainMenu;
    QMenu *fileMenu;
    QMenu *settingsMenu;
    QMenu *languageMenu;
    QMenu *helpMenu;

    QActionGroup *languageActionGroup;
    QAction *openDbAction;
    QAction *saveDbAction;
    QAction *reloadDbAction;
    QAction *exitAction;
    QAction *aboutAction;

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
    polygon4::DatabaseSchema *schema = 0;
    bool dataChanged = false;
};