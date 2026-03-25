#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

/**
 * Represents a remote user and the shared repository that stores messages
 * exchanged between the local user and that remote user.
 */
struct Contact {
    QString username;         ///< GitLab username
    QString avatarUrl;        ///< Avatar URL (may be empty)
    int     userId{0};        ///< GitLab numeric user id

    // ── Repository info ──────────────────────────────────────────────────────
    /// Full project path on the local user's GitLab host, e.g. "alice/chat-with-bob"
    QString localRepoPath;
    int     localRepoId{0};

    /// Full project URL on the remote user's GitLab host, e.g.
    /// "https://gitlab.example.com/bob/chat-with-alice"
    QString remoteRepoUrl;

    QString displayName() const { return username; }

    QJsonObject toJson() const;
    static Contact fromJson(const QJsonObject &obj);
};

/**
 * ContactManager persists the local user's contact list and handles the
 * GitLab repository negotiation required to start or resume a conversation.
 *
 * Each contact entry knows:
 *  - The remote user's GitLab username and numeric id
 *  - The path of the shared repo on the local user's GitLab account
 *  - The URL of the shared repo on the remote user's GitLab account
 *    (this is given to the local user by the remote user out-of-band)
 *
 * Message file convention:
 *   Both repositories contain a file named "messages.html" on the default
 *   branch.  When the local user sends a message, it is appended to the
 *   local copy and committed.  The receiving user's client polls their own
 *   repo to pick up new messages.
 */
class GitLabClient;

class ContactManager : public QObject
{
    Q_OBJECT

public:
    explicit ContactManager(GitLabClient *client, QObject *parent = nullptr);

    // ── Contact list ──────────────────────────────────────────────────────────

    QList<Contact> contacts() const { return m_contacts; }

    /** Persists the contact list to local QSettings. */
    void saveContacts();

    /** Loads the contact list from local QSettings. */
    void loadContacts();

    // ── Adding a new contact ──────────────────────────────────────────────────

    /**
     * Starts the "add contact" flow:
     *
     * 1. Look up the remote user by @p remoteUsername on the local GitLab host.
     * 2. Create a private repo "chat-with-<remoteUsername>" on the local host.
     * 3. Grant the remote user Developer access to that repo.
     * 4. Initialise it with an empty messages.html file.
     *
     * @param remoteUsername  GitLab username of the person to add
     * @param remoteRepoUrl   Full URL of the repo that the remote user has
     *                        set up on their side (shared out-of-band)
     * @param onOk   called with the newly created Contact on success
     * @param onErr  called with an error string on failure
     */
    void addContact(const QString &remoteUsername,
                    const QString &remoteRepoUrl,
                    std::function<void(Contact)>     onOk,
                    std::function<void(QString)>     onErr);

    /**
     * Returns the repo name used on the local GitLab for a conversation with
     * @p remoteUsername.
     */
    static QString repoNameForContact(const QString &remoteUsername);

    /**
     * Returns the file path within the repository where messages are stored.
     */
    static QString messagesFilePath() { return QStringLiteral("messages.html"); }

signals:
    void contactAdded(Contact contact);
    void contactsChanged();

private:
    GitLabClient   *m_client{nullptr};
    QList<Contact>  m_contacts;
};
