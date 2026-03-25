#include "contactmanager.h"
#include "gitlabclient.h"
#include "messagestore.h"

#include <QSettings>
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

// ── Persistence ───────────────────────────────────────────────────────────────

void ContactManager::saveContacts()
{
    QSettings settings;
    QJsonArray arr;
    for (const Contact &c : std::as_const(m_contacts))
        arr.append(c.toJson());
    settings.setValue(QStringLiteral("contacts"),
                      QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void ContactManager::loadContacts()
{
    QSettings settings;
    QByteArray raw = settings.value(QStringLiteral("contacts")).toByteArray();
    if (raw.isEmpty())
        return;
    auto doc = QJsonDocument::fromJson(raw);
    if (!doc.isArray())
        return;
    m_contacts.clear();
    for (const QJsonValue &v : doc.array())
        m_contacts.append(Contact::fromJson(v.toObject()));
    emit contactsChanged();
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
                                    saveContacts();
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
