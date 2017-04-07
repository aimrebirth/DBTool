#pragma once

#include <unordered_map>

#include <qabstractitemmodel.h>
#include <qsettings.h>

class AppSettings;

class AppSetting : public QObject
{
    Q_OBJECT
public:
    explicit AppSetting(AppSettings *settings, QObject *appSetting, QObject *parent = 0);
    virtual ~AppSetting();

private slots:
    void onPropertyChanges();

private:
    AppSettings *settings;
    QObject *appSetting;
    QObject *object;
    QByteArray property;
    QByteArray setting;
    QVariant defaultValue;
};

class AppSettings : public QSettings
{
    Q_OBJECT

    typedef std::unordered_map<QObject *, AppSetting *> Settings;

public:
    explicit AppSettings(QObject *parent = 0);

    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    Q_INVOKABLE void registerSetting(QObject *o);
    Q_INVOKABLE void unregisterSetting(QObject *o);

    Settings getSettings() const;

    void exportSettings(QString fn);
    void importSettings(QString fn);
    static void copySettings(QSettings *dst, QSettings *src);

private:
    Settings settings;
};

class SettingsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Roles {
        SettingRole = Qt::UserRole + 1,
        ValueRole,
    };

public:
    explicit SettingsModel(const AppSettings &appSettings, QObject *parent = 0);
    ~SettingsModel(){}

    virtual int	columnCount(const QModelIndex& = QModelIndex()) const;
    virtual int	rowCount(const QModelIndex& = QModelIndex()) const;
    virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    virtual QHash<int, QByteArray> roleNames() const override;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    Q_INVOKABLE void refresh();

private:
    const AppSettings &appSettings;
};
