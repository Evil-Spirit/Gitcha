#include "contactmanager.h"
#include "gitlabclient.h"
#include "messagestore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ── Contact serialisation ─────────────────────────────────────────────────────

QJsonObject Contact::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("username")]      = username;
    o[QStringLiteral("avatarUrl")]     = avatarUrl;
    o[QStringLiteral("userId")]        = userId;
    o[QStringLiteral("localRepoPath")] = localRepoPath;
    o[QStringLiteral("localRepoId")]   = localRepoId;
    o[QStringLiteral("remoteRepoUrl")] = remoteRepoUrl;
    return o;
}

Contact Contact::fromJson(const QJsonObject &obj)
{
    Contact c;
    c.username      = obj.value(QStringLiteral("username")).toString();
    c.avatarUrl     = obj.value(QStringLiteral("avatarUrl")).toString();
    c.userId        = obj.value(QStringLiteral("userId")).toInt();
    c.localRepoPath = obj.value(QStringLiteral("localRepoPath")).toString();
    c.localRepoId   = obj.value(QStringLiteral("localRepoId")).toInt();
    c.remoteRepoUrl = obj.value(QStringLiteral("remoteRepoUrl")).toString();
    return c;
}

// ── ContactManager ────────────────────────────────────────────────────────────

ContactManager::ContactManager(GitLabClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{}

QString ContactManager::repoNameForContact(const QString &remoteUsername)
{
    return QStringLiteral("chat-with-") + remoteUsername.toLower();
}

QString ContactManager::remoteRepoPathForContact(const QString &contactUsername,
                                                   const QString &localUsername)
{
    return contactUsername.toLower() + QLatin1String("/chat-with-") + localUsername.toLower();
}

// ── Loading contacts from GitLab ──────────────────────────────────────────────

void ContactManager::loadContactsFromGitLab(const QString &currentUser,
                                             std::function<void()>        onDone,
                                             std::function<void(QString)> onErr)
{
    m_client->listProjects(QStringLiteral("chat-with-"),
        [this, currentUser, onDone](QJsonArray projects) {
            m_contacts.clear();
            const QString prefix = QStringLiteral("chat-with-");
            for (const QJsonValue &v : projects) {
                QJsonObject proj = v.toObject();
                const QString name = proj.value(QStringLiteral("name")).toString();
                if (!name.startsWith(prefix))
                    continue;
                const QString remoteUsername = name.mid(prefix.length());
                if (remoteUsername.isEmpty())
                    continue;

                Contact c;
                c.username      = remoteUsername;
                c.localRepoId   = proj.value(QStringLiteral("id")).toInt();
                c.localRepoPath = proj.value(QStringLiteral("path_with_namespace")).toString();
                c.remoteRepoUrl = m_client->serverUrl() + QLatin1Char('/') +
                                  remoteRepoPathForContact(remoteUsername, currentUser);
                m_contacts.append(c);
            }
            emit contactsChanged();
            onDone();
        },
        onErr);
}

// ── addContact flow ───────────────────────────────────────────────────────────

void ContactManager::addContact(const QString &remoteUsername,
                                 const QString &remoteRepoUrl,
                                 std::function<void(Contact)>  onOk,
                                 std::function<void(QString)>  onErr)
{
    // Step 1: look up the remote user by username to get their numeric id
    m_client->findUser(remoteUsername,
        [this, remoteUsername, remoteRepoUrl, onOk, onErr]
        (int userId, QString uname, QString avatar)
        {
            Contact partial;
            partial.username    = uname;
            partial.avatarUrl   = avatar;
            partial.userId      = userId;
            partial.remoteRepoUrl = remoteRepoUrl;

            // Step 2: create the local repo
            const QString repoName = repoNameForContact(uname);
            m_client->createRepository(repoName,
                [this, partial, onOk, onErr]
                (int projectId, QString httpUrl, QString /*ssh*/) mutable
                {
                    partial.localRepoId   = projectId;
                    // Derive path from http url: strip server prefix and ".git"
                    QUrl url(httpUrl);
                    QString path = url.path();
                    if (path.startsWith('/'))
                        path = path.mid(1);
                    if (path.endsWith(QStringLiteral(".git")))
                        path.chop(4);
                    partial.localRepoPath = path;

                    // Step 3: grant access to the remote user
                    m_client->addProjectMember(projectId, partial.userId,
                        [this, partial, onOk, onErr]() mutable
                        {
                            // Step 4: initialise messages.html
                            const QString title = QStringLiteral("Chat with ") + partial.username;
                            QByteArray empty = MessageStore::emptyDocument(title);
                            m_client->commitFile(partial.localRepoPath,
                                ContactManager::messagesFilePath(),
                                empty,
                                QStringLiteral("Initialise conversation"),
                                QStringLiteral("main"),
                                [this, partial, onOk]() mutable
                                {
                                    m_contacts.append(partial);
                                    // Contacts are sourced from GitLab; no local persistence needed.
                                    emit contactAdded(partial);
                                    emit contactsChanged();
                                    onOk(partial);
                                },
                                onErr);
                        },
                        onErr);
                },
                onErr);
        },
        onErr);
}
