#include "settingsmanager.h"

#include <QSettings>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{}

// ── Getters ───────────────────────────────────────────────────────────────────

QString SettingsManager::serverUrl() const
{
    QSettings s;
    return s.value(QStringLiteral("auth/serverUrl"),
                   QStringLiteral("https://gitlab.com")).toString();
}

QString SettingsManager::accessToken() const
{
    QSettings s;
    return s.value(QStringLiteral("auth/accessToken")).toString();
}

QString SettingsManager::username() const
{
    QSettings s;
    return s.value(QStringLiteral("auth/username")).toString();
}

bool SettingsManager::rememberMe() const
{
    QSettings s;
    return s.value(QStringLiteral("auth/rememberMe"), false).toBool();
}

// ── Setters ───────────────────────────────────────────────────────────────────

void SettingsManager::setServerUrl(const QString &v)
{
    QSettings s;
    s.setValue(QStringLiteral("auth/serverUrl"), v);
    emit serverUrlChanged();
}

void SettingsManager::setAccessToken(const QString &v)
{
    QSettings s;
    s.setValue(QStringLiteral("auth/accessToken"), v);
    emit accessTokenChanged();
}

void SettingsManager::setUsername(const QString &v)
{
    QSettings s;
    s.setValue(QStringLiteral("auth/username"), v);
    emit usernameChanged();
}

void SettingsManager::setRememberMe(bool v)
{
    QSettings s;
    s.setValue(QStringLiteral("auth/rememberMe"), v);
    emit rememberMeChanged();
}

void SettingsManager::save()
{
    QSettings().sync();
}

void SettingsManager::clear()
{
    QSettings s;
    s.remove(QStringLiteral("auth/accessToken"));
    emit accessTokenChanged();
}
