#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QTimer>
#include <algorithm>

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

        const QString commitMsg = QStringLiteral("[msg] %1: %2")
                                    .arg(m_currentUser, text.left(60));

        // Write only to the local repo (current user owns it; the contact has Developer
        // access there so they can read it via the dual-repo refresh below).
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
                        setStatus(QStringLiteral("Send failed: ") + e);
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
                        setStatus(QStringLiteral("Send failed: ") + e);
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
        // The contact's own repo where they write their messages to us.
        // e.g. for Alice reading Bob: "bob/chat-with-alice"
        const QString remoteRepoPath = ContactManager::remoteRepoPathForContact(
            contact.username, m_currentUser);

        if (!silent) setLoading(true);

        // Helper: combine, deduplicate and sort the two message lists, then
        // push to the model and clear the loading state.
        auto finalize = [this, silent](QList<Message> mine, QList<Message> theirs) {
            m_messageModel->setMessages(mergeMessages(mine, theirs));
            if (!silent) {
                setLoading(false);
                setStatus({});
            }
        };

        // Step 1: read the current user's own messages (their own repo).
        m_gitlab->getFileContent(contact.localRepoPath,
            ContactManager::messagesFilePath(),
            QStringLiteral("main"),
            [this, remoteRepoPath, finalize](QByteArray myData) {
                QList<Message> mine = MessageStore::parseMessages(myData, m_currentUser);
                // Step 2a: read the contact's messages.
                m_gitlab->getFileContent(remoteRepoPath,
                    ContactManager::messagesFilePath(),
                    QStringLiteral("main"),
                    [this, mine, finalize](QByteArray theirData) {
                        finalize(mine, MessageStore::parseMessages(theirData, m_currentUser));
                    },
                    [mine, finalize](QString) {
                        // Contact's repo not accessible yet — show only our messages.
                        finalize(mine, {});
                    });
            },
            [this, remoteRepoPath, finalize, silent](QString localErr) {
                // Step 2b: our repo unreadable — try the contact's repo alone.
                m_gitlab->getFileContent(remoteRepoPath,
                    ContactManager::messagesFilePath(),
                    QStringLiteral("main"),
                    [this, finalize](QByteArray theirData) {
                        finalize({}, MessageStore::parseMessages(theirData, m_currentUser));
                    },
                    [this, silent, localErr](QString) {
                        if (!silent) {
                            setLoading(false);
                            setStatus(QStringLiteral("Refresh failed: ") + localErr);
                        }
                    });
            });
    }

    // Merge two message lists: deduplicate by id and sort chronologically.
    static QList<Message> mergeMessages(QList<Message> a, QList<Message> b)
    {
        QHash<QString, Message> byId;
        for (const Message &m : a)
            byId.insert(m.id, m);
        for (const Message &m : b)
            if (!byId.contains(m.id))
                byId.insert(m.id, m);
        QList<Message> merged = byId.values();
        std::sort(merged.begin(), merged.end(), [](const Message &x, const Message &y) {
            return x.timestamp < y.timestamp;
        });
        return merged;
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
