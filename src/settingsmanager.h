#pragma once

#include <QObject>
#include <QString>

/**
 * SettingsManager wraps QSettings to provide typed read/write access to the
 * application's persistent configuration (server URL, access token, etc.).
 *
 * Exposed to QML as a context property named "settings".
 */
class SettingsManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString serverUrl   READ serverUrl   WRITE setServerUrl   NOTIFY serverUrlChanged)
    Q_PROPERTY(QString accessToken READ accessToken WRITE setAccessToken NOTIFY accessTokenChanged)
    Q_PROPERTY(QString username    READ username    WRITE setUsername    NOTIFY usernameChanged)
    Q_PROPERTY(bool    rememberMe  READ rememberMe  WRITE setRememberMe  NOTIFY rememberMeChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr);

    QString serverUrl()   const;
    QString accessToken() const;
    QString username()    const;
    bool    rememberMe()  const;

    void setServerUrl(const QString &v);
    void setAccessToken(const QString &v);
    void setUsername(const QString &v);
    void setRememberMe(bool v);

    Q_INVOKABLE void save();
    Q_INVOKABLE void clear();

signals:
    void serverUrlChanged();
    void accessTokenChanged();
    void usernameChanged();
    void rememberMeChanged();
};
