#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <functional>

/**
 * GitLabClient wraps the GitLab REST API.
 *
 * All methods are asynchronous and deliver results via callback lambdas.
 * The class handles authentication using a personal-access token (PAT).
 *
 * API reference: https://docs.gitlab.com/ee/api/rest/
 */
class GitLabClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ isAuthenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QString username   READ username        NOTIFY usernameChanged)
    Q_PROPERTY(QString serverUrl  READ serverUrl       WRITE setServerUrl  NOTIFY serverUrlChanged)

public:
    explicit GitLabClient(QObject *parent = nullptr);

    // ── Properties ────────────────────────────────────────────────────────────
    bool    isAuthenticated() const { return m_authenticated; }
    QString username()        const { return m_username; }
    QString serverUrl()       const { return m_serverUrl; }
    void    setServerUrl(const QString &url);

    // ── Authentication ────────────────────────────────────────────────────────

    /**
     * Validates the personal-access token against the /api/v4/user endpoint.
     * On success the user's profile is cached and authenticatedChanged() is emitted.
     *
     * @param token  GitLab personal access token
     * @param onOk   called with (userId, username, avatarUrl) on success
     * @param onErr  called with an error string on failure
     */
    void authenticate(const QString &token,
                      std::function<void(int, QString, QString)> onOk,
                      std::function<void(QString)>               onErr);

    // ── Repository operations ─────────────────────────────────────────────────

    /**
     * Creates a private repository on the authenticated user's GitLab account.
     *
     * @param name      repository name (will be slugified by GitLab)
     * @param onOk      called with (projectId, httpUrl, sshUrl) on success
     * @param onErr     called with an error string on failure
     */
    void createRepository(const QString &name,
                          std::function<void(int, QString, QString)> onOk,
                          std::function<void(QString)>               onErr);

    /**
     * Returns JSON metadata for a repository identified by its full path
     * (e.g. "alice/chat-with-bob").
     */
    void getRepository(const QString &projectPath,
                       std::function<void(QJsonObject)> onOk,
                       std::function<void(QString)>     onErr);

    /**
     * Grants the target user Developer access to a repository so that they
     * can push commits from their own GitLab account.
     *
     * @param projectId  numeric project id
     * @param userId     numeric user id to invite
     * @param onOk       called on success
     * @param onErr      called with an error string on failure
     */
    void addProjectMember(int projectId,
                          int userId,
                          std::function<void()>        onOk,
                          std::function<void(QString)> onErr);

    // ── File / commit operations ──────────────────────────────────────────────

    /**
     * Returns the raw content of a file in a repository.
     *
     * @param projectPath  "namespace/repo"
     * @param filePath     path inside the repo (e.g. "messages.html")
     * @param ref          branch or commit sha (default "main")
     */
    void getFileContent(const QString &projectPath,
                        const QString &filePath,
                        const QString &ref,
                        std::function<void(QByteArray)> onOk,
                        std::function<void(QString)>    onErr);

    /**
     * Creates or updates a file in a repository via a single commit.
     *
     * @param projectPath   "namespace/repo"
     * @param filePath      path inside the repo
     * @param content       new file content (UTF-8)
     * @param commitMessage git commit message
     * @param branch        target branch (default "main")
     * @param onOk          called on success
     * @param onErr         called with an error string on failure
     */
    void commitFile(const QString  &projectPath,
                    const QString  &filePath,
                    const QByteArray &content,
                    const QString  &commitMessage,
                    const QString  &branch,
                    std::function<void()>        onOk,
                    std::function<void(QString)> onErr);

    /**
     * Returns the list of commits for a file in a repository.
     *
     * @param projectPath  "namespace/repo"
     * @param filePath     path inside the repo
     * @param onOk         called with an array of commit objects
     * @param onErr        called with an error string on failure
     */
    void getFileCommits(const QString &projectPath,
                        const QString &filePath,
                        std::function<void(QJsonArray)> onOk,
                        std::function<void(QString)>    onErr);

    // ── User lookup ───────────────────────────────────────────────────────────

    /**
     * Looks up a user by username.
     *
     * @param onOk   called with (userId, username, avatarUrl)
     * @param onErr  called with an error string
     */
    void findUser(const QString &username,
                  std::function<void(int, QString, QString)> onOk,
                  std::function<void(QString)>               onErr);

signals:
    void authenticatedChanged();
    void usernameChanged();
    void serverUrlChanged();
    void networkError(QString message);

private:
    // ── Helpers ───────────────────────────────────────────────────────────────
    QNetworkRequest buildRequest(const QString &endpoint) const;
    void handleReply(QNetworkReply *reply,
                     std::function<void(QByteArray)> onData,
                     std::function<void(QString)>    onErr);

    QString              m_serverUrl;
    QString              m_token;
    QString              m_username;
    int                  m_userId{0};
    bool                 m_authenticated{false};
    QNetworkAccessManager m_nam;
};
