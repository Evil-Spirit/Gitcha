#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QTimer>

#include "gitlabclient.h"
#include "contactmanager.h"
#include "contactmodel.h"
#include "messagemodel.h"
#include "settingsmanager.h"

// ── Application controller ────────────────────────────────────────────────────
// This QObject acts as the "glue" between the C++ backend and QML.
// It owns all singletons and exposes them as context properties.

class AppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QString currentUser READ currentUser NOTIFY currentUserChanged)

    Q_PROPERTY(QString activeChatName READ activeChatName NOTIFY activeChatIndexChanged)

    Q_PROPERTY(int    activeChatIndex READ activeChatIndex WRITE setActiveChatIndex
               NOTIFY activeChatIndexChanged)
    Q_PROPERTY(bool   sending  READ sending  NOTIFY sendingChanged)
    Q_PROPERTY(bool   loading  READ loading  NOTIFY loadingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    // Polling interval in milliseconds (15 seconds)
    static constexpr int kPollIntervalMs = 15'000;

    AppController(GitLabClient   *gitlab,
                  ContactManager *contacts,
                  ContactModel   *contactModel,
                  MessageModel   *messageModel,
                  SettingsManager *settings,
                  QObject *parent = nullptr)
        : QObject(parent)
        , m_gitlab(gitlab)
        , m_contacts(contacts)
        , m_contactModel(contactModel)
        , m_messageModel(messageModel)
        , m_settings(settings)
    {
        connect(m_contacts, &ContactManager::contactAdded, this, [this](const Contact &c) {
            m_contactModel->appendContact(c);
        });

        m_pollTimer = new QTimer(this);
        m_pollTimer->setInterval(kPollIntervalMs);
        connect(m_pollTimer, &QTimer::timeout,
                this, [this]() { doRefreshMessages(/*silent=*/true); });
    }

    bool    authenticated()    const { return m_authenticated; }
    QString currentUser()      const { return m_currentUser; }
    int     activeChatIndex()  const { return m_activeChatIndex; }
    bool    sending()          const { return m_sending; }
    bool    loading()          const { return m_loading; }
    QString statusMessage()    const { return m_statusMessage; }

    QString activeChatName() const {
        if (m_activeChatIndex < 0)
            return {};
        const auto &contacts = m_contacts->contacts();
        if (m_activeChatIndex >= contacts.size())
            return {};
        return contacts.at(m_activeChatIndex).username;
    }

    void setActiveChatIndex(int idx) {
        if (m_activeChatIndex == idx) return;
        m_activeChatIndex = idx;
        emit activeChatIndexChanged();
        if (idx >= 0) {
            refreshMessages();
            m_pollTimer->start();
        } else {
            m_pollTimer->stop();
        }
    }

    // ── QML-invokable methods ─────────────────────────────────────────────────

    Q_INVOKABLE void login(const QString &serverUrl,
                           const QString &token,
                           bool rememberMe)
    {
        setStatus(QStringLiteral("Connecting…"));
        setLoading(true);

        m_gitlab->setServerUrl(serverUrl);
        m_gitlab->authenticate(token,
            [this, serverUrl, token, rememberMe](int /*id*/, QString uname, QString /*av*/) {
                m_authenticated = true;
                m_currentUser   = uname;
                emit authenticatedChanged();
                emit currentUserChanged();

                if (rememberMe) {
                    m_settings->setServerUrl(serverUrl);
                    m_settings->setAccessToken(token);
                    m_settings->setUsername(uname);
                    m_settings->setRememberMe(true);
                }

                setStatus(QStringLiteral("Loading contacts…"));
                m_contacts->loadContactsFromGitLab(uname,
                    [this, uname]() {
                        m_contactModel->setContacts(m_contacts->contacts());
                        setLoading(false);
                        setStatus(QStringLiteral("Logged in as %1").arg(uname));
                    },
                    [this, uname](QString err) {
                        m_contactModel->setContacts({});
                        setLoading(false);
                        setStatus(QStringLiteral("Logged in as %1 (contacts: %2)").arg(uname, err));
                    });
            },
            [this](QString err) {
                setLoading(false);
                setStatus(QStringLiteral("Login failed: ") + err);
            });
    }

    Q_INVOKABLE void logout()
    {
        m_pollTimer->stop();
        m_authenticated   = false;
        m_currentUser.clear();
        m_activeChatIndex = -1;
        m_contactModel->setContacts({});
        m_messageModel->clear();
        m_settings->clear();
        emit authenticatedChanged();
        emit currentUserChanged();
        emit activeChatIndexChanged();
        setStatus({});
    }

    Q_INVOKABLE void addContact(const QString &remoteUsername,
                                const QString &remoteRepoUrl)
    {
        // Auto-derive the remote repo URL when not supplied.
        // The convention is that the remote user names their repo
        // "chat-with-<localUser>" on the same GitLab instance.
        QString resolvedUrl = remoteRepoUrl.trimmed();
        if (resolvedUrl.isEmpty()) {
            resolvedUrl = m_gitlab->serverUrl() + QLatin1Char('/') +
                          ContactManager::remoteRepoPathForContact(remoteUsername, m_currentUser);
        }

        setStatus(QStringLiteral("Adding contact…"));
        setLoading(true);
        m_contacts->addContact(remoteUsername, resolvedUrl,
            [this](Contact c) {
                setLoading(false);
                setStatus(QStringLiteral("Contact %1 added").arg(c.username));
            },
            [this](QString err) {
                setLoading(false);
                setStatus(QStringLiteral("Failed to add contact: ") + err);
            });
    }

    Q_INVOKABLE void sendMessage(const QString &text)
    {
        if (text.trimmed().isEmpty() || m_activeChatIndex < 0)
            return;

        const Contact contact = m_contacts->contacts().at(m_activeChatIndex);
        const QDateTime now   = QDateTime::currentDateTimeUtc();

        // Build the optimistic local message
        Message localMsg;
        localMsg.sender    = m_currentUser;
        localMsg.text      = text;
        localMsg.timestamp = now;
        localMsg.isMine    = true;
        localMsg.id        = now.toString(Qt::ISODate) + QLatin1Char('|') + m_currentUser;
        m_messageModel->appendMessage(localMsg);

        setSending(true);

        // The remote user's repo is where we write the message so they can read it.
        // Format: "<contactUsername>/chat-with-<localUser>", e.g. "bob/chat-with-alice"
        const QString remoteRepoPath = ContactManager::remoteRepoPathForContact(
            contact.username, m_currentUser);
        const QString commitMsg = QStringLiteral("[msg] %1: %2")
                                    .arg(m_currentUser, text.left(60));

        // After the remote commit succeeds, also append the same message to the
        // local repo so that the sender's own message feed stays in sync.
        auto syncToLocal = [this, contact, text, now, commitMsg]() {
            m_gitlab->getFileContent(contact.localRepoPath,
                ContactManager::messagesFilePath(),
                QStringLiteral("main"),
                [this, contact, text, now, commitMsg](QByteArray existing) {
                    QByteArray updated = MessageStore::appendMessage(
                        existing, m_currentUser, text, now);
                    m_gitlab->commitFile(contact.localRepoPath,
                        ContactManager::messagesFilePath(),
                        updated, commitMsg, QStringLiteral("main"),
                        [this]() {
                            setSending(false);
                            setStatus(QStringLiteral("Message sent"));
                        },
                        [this](QString e) {
                            setSending(false);
                            setStatus(QStringLiteral("Send failed (sync): ") + e);
                        });
                },
                [this, contact, text, now, commitMsg](QString) {
                    QByteArray updated = MessageStore::appendMessage(
                        {}, m_currentUser, text, now);
                    m_gitlab->commitFile(contact.localRepoPath,
                        ContactManager::messagesFilePath(),
                        updated, commitMsg, QStringLiteral("main"),
                        [this]() {
                            setSending(false);
                            setStatus(QStringLiteral("Message sent"));
                        },
                        [this](QString e) {
                            setSending(false);
                            setStatus(QStringLiteral("Send failed (sync): ") + e);
                        });
                });
        };

        // Step 1: commit message to the remote user's repository
        m_gitlab->getFileContent(remoteRepoPath,
            ContactManager::messagesFilePath(),
            QStringLiteral("main"),
            [this, remoteRepoPath, text, now, commitMsg, syncToLocal](QByteArray existing) {
                QByteArray updated = MessageStore::appendMessage(
                    existing, m_currentUser, text, now);
                m_gitlab->commitFile(remoteRepoPath,
                    ContactManager::messagesFilePath(),
                    updated, commitMsg, QStringLiteral("main"),
                    [syncToLocal]() { syncToLocal(); },
                    [this](QString err) {
                        setSending(false);
                        setStatus(QStringLiteral("Send failed: ") + err);
                    });
            },
            [this, remoteRepoPath, text, now, commitMsg, syncToLocal](QString) {
                // messages.html may not exist yet in remote repo
                QByteArray updated = MessageStore::appendMessage(
                    {}, m_currentUser, text, now);
                m_gitlab->commitFile(remoteRepoPath,
                    ContactManager::messagesFilePath(),
                    updated, commitMsg, QStringLiteral("main"),
                    [syncToLocal]() { syncToLocal(); },
                    [this](QString err) {
                        setSending(false);
                        setStatus(QStringLiteral("Send failed: ") + err);
                    });
            });
    }

    Q_INVOKABLE void refreshMessages()
    {
        doRefreshMessages(/*silent=*/false);
    }

    Q_INVOKABLE QString localRepoUrlForActiveChat() const
    {
        if (m_activeChatIndex < 0)
            return {};
        const auto &contacts = m_contacts->contacts();
        if (m_activeChatIndex >= contacts.size())
            return {};
        return m_gitlab->serverUrl() + QLatin1Char('/') +
               contacts.at(m_activeChatIndex).localRepoPath;
    }

signals:
    void authenticatedChanged();
    void currentUserChanged();
    void activeChatIndexChanged();
    void sendingChanged();
    void loadingChanged();
    void statusMessageChanged();

private:
    // ── Refresh implementation ────────────────────────────────────────────────
    // When silent=true the loading indicator is suppressed; used by the poll timer.
    void doRefreshMessages(bool silent)
    {
        if (m_activeChatIndex < 0)
            return;

        const Contact contact = m_contacts->contacts().at(m_activeChatIndex);
        if (!silent) setLoading(true);

        m_gitlab->getFileContent(contact.localRepoPath,
            ContactManager::messagesFilePath(),
            QStringLiteral("main"),
            [this, silent](QByteArray data) {
                auto msgs = MessageStore::parseMessages(data, m_currentUser);
                m_messageModel->setMessages(msgs);
                if (!silent) {
                    setLoading(false);
                    setStatus({});
                }
            },
            [this, silent](QString err) {
                if (!silent) {
                    setLoading(false);
                    setStatus(QStringLiteral("Refresh failed: ") + err);
                }
            });
    }

    void setStatus(const QString &s) {
        if (m_statusMessage == s) return;
        m_statusMessage = s;
        emit statusMessageChanged();
    }
    void setSending(bool v) {
        if (m_sending == v) return;
        m_sending = v;
        emit sendingChanged();
    }
    void setLoading(bool v) {
        if (m_loading == v) return;
        m_loading = v;
        emit loadingChanged();
    }

    GitLabClient    *m_gitlab{nullptr};
    ContactManager  *m_contacts{nullptr};
    ContactModel    *m_contactModel{nullptr};
    MessageModel    *m_messageModel{nullptr};
    SettingsManager *m_settings{nullptr};
    QTimer          *m_pollTimer{nullptr};

    bool    m_authenticated{false};
    QString m_currentUser;
    int     m_activeChatIndex{-1};
    bool    m_sending{false};
    bool    m_loading{false};
    QString m_statusMessage;
};

#include "main.moc"

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Gitcha"));
    app.setOrganizationName(QStringLiteral("Gitcha"));
    app.setOrganizationDomain(QStringLiteral("gitcha.app"));
    app.setApplicationVersion(QStringLiteral("1.0"));
    app.setWindowIcon(QIcon(QStringLiteral(":/assets/logo.png")));

    // Use a consistent Material style on all platforms
    QQuickStyle::setStyle(QStringLiteral("Material"));

    // ── Backend objects ───────────────────────────────────────────────────────
    SettingsManager settings;
    GitLabClient    gitlab;
    ContactManager  contactManager(&gitlab);
    ContactModel    contactModel;
    MessageModel    messageModel;

    AppController app_ctrl(&gitlab, &contactManager, &contactModel,
                            &messageModel, &settings);

    // ── QML engine ────────────────────────────────────────────────────────────
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("app"),     &app_ctrl);
    engine.rootContext()->setContextProperty(QStringLiteral("gitlab"),  &gitlab);
    engine.rootContext()->setContextProperty(QStringLiteral("settings"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("contactModel"), &contactModel);
    engine.rootContext()->setContextProperty(QStringLiteral("messageModel"), &messageModel);

    const QUrl url(QStringLiteral("qrc:/Gitcha/qml/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
