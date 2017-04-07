#include "appsettings.h"

#include <qmetaobject.h>

AppSetting::AppSetting(AppSettings *settings, QObject *appSetting, QObject *parent)
    : QObject(parent), settings(settings), appSetting(appSetting)
{
    object = appSetting->parent();
    property = appSetting->property("property").toByteArray();
    setting = appSetting->property("setting").toByteArray();
    defaultValue = appSetting->property("defaultValue");
    object->setProperty(property, settings->value(setting, defaultValue));

    auto mo = object->metaObject();
    auto i = mo->indexOfProperty(property);
    auto mp = mo->property(i);
    bool ns = mp.hasNotifySignal();
    QMetaMethod nm;
    if (ns)
    {
        nm = mp.notifySignal();
        connect(object, "2" + nm.methodSignature(), this, SLOT(onPropertyChanges()));
    }
}

AppSetting::~AppSetting()
{

}

void AppSetting::onPropertyChanges()
{
    settings->setValue(setting, object->property(property));
}

AppSettings::AppSettings(QObject *parent)
    : QSettings(parent)
{
}

void AppSettings::setValue(const QString &key, const QVariant &value)
{
    QSettings::setValue(key, value);
}

QVariant AppSettings::value(const QString &key, const QVariant &defaultValue) const
{
    return QSettings::value(key, defaultValue);
}

void AppSettings::registerSetting(QObject *o)
{
    settings[o] = new AppSetting(this, o, this);
}

void AppSettings::unregisterSetting(QObject *o)
{
    settings.erase(o);
}

AppSettings::Settings AppSettings::getSettings() const
{
    return settings;
}

void AppSettings::exportSettings(QString fn)
{
    QSettings s(fn, QSettings::IniFormat);
    copySettings(&s, this);
}

void AppSettings::importSettings(QString fn)
{
    QSettings s(fn, QSettings::IniFormat);
    copySettings(this, &s);
}

void AppSettings::copySettings(QSettings *dst, QSettings *src)
{
    auto keys = src->allKeys();
    for (auto &k : keys)
        dst->setValue(k, src->value(k));
}

SettingsModel::SettingsModel(const AppSettings &appSettings, QObject *parent)
    : QAbstractTableModel(parent), appSettings(appSettings)
{
}

int SettingsModel::columnCount(const QModelIndex&) const
{
    return 2;
}

bool SettingsModel::insertRows(int row, int count, const QModelIndex &)
{
    beginInsertRows(QModelIndex(), row, row+count-1);
    endInsertRows();
    emit dataChanged(QModelIndex(), QModelIndex());
    return true;
}

int SettingsModel::rowCount(const QModelIndex&) const
{
    return appSettings.allKeys().size();
}

QHash<int, QByteArray> SettingsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[SettingRole] = "setting";
    roles[ValueRole] = "value";
    return roles;
}

QVariant SettingsModel::data(const QModelIndex& index, int role) const
{
    auto keys = appSettings.allKeys();
    int r = index.row();
    switch (role)
    {
    case SettingRole:
        return keys[r];
    case ValueRole:
        return appSettings.value(keys[r]);
    }
    return QVariant();
}

void SettingsModel::refresh()
{
    QVector<int> roles = { ValueRole };
    emit dataChanged(QModelIndex(), QModelIndex(), roles);
}
