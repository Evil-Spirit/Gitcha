#include "gitlabclient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>

// GitLab access level: Developer = 30
static constexpr int kDeveloperAccessLevel = 30;

// ── Constructor ────────────────────────────────────────────────────────────────

GitLabClient::GitLabClient(QObject *parent)
    : QObject(parent)
    , m_serverUrl(QStringLiteral("https://gitlab.com"))
{}

// ── Property setters ──────────────────────────────────────────────────────────

void GitLabClient::setServerUrl(const QString &url)
{
    if (m_serverUrl == url)
        return;
    // Normalise: strip trailing slash
    m_serverUrl = url.endsWith('/') ? url.chopped(1) : url;
    emit serverUrlChanged();
}

// ── Private helpers ───────────────────────────────────────────────────────────

QNetworkRequest GitLabClient::buildRequest(const QString &endpoint) const
{
    QUrl url(m_serverUrl + QStringLiteral("/api/v4") + endpoint);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_token.isEmpty())
        req.setRawHeader("PRIVATE-TOKEN", m_token.toUtf8());
    return req;
}

void GitLabClient::handleReply(QNetworkReply *reply,
                                std::function<void(QByteArray)> onData,
                                std::function<void(QString)>    onErr)
{
    connect(reply, &QNetworkReply::finished, this, [reply, onData, onErr]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const QByteArray body = reply->readAll();
            QString detail;
            auto doc = QJsonDocument::fromJson(body);
            if (!doc.isNull() && doc.isObject())
                detail = doc.object().value(QStringLiteral("message")).toString();
            if (detail.isEmpty())
                detail = reply->errorString();
            onErr(detail);
            return;
        }
        onData(reply->readAll());
    });
}

// ── Authentication ────────────────────────────────────────────────────────────

void GitLabClient::authenticate(const QString &token,
                                 std::function<void(int, QString, QString)> onOk,
                                 std::function<void(QString)>               onErr)
{
    m_token = token;
    m_authenticated = false;

    QNetworkRequest req = buildRequest(QStringLiteral("/user"));
    QNetworkReply *reply = m_nam.get(req);

    handleReply(reply,
        [this, onOk](QByteArray data) {
            auto doc = QJsonDocument::fromJson(data);
            if (!doc.isObject()) { return; }
            QJsonObject obj = doc.object();
            m_userId        = obj.value(QStringLiteral("id")).toInt();
            m_username      = obj.value(QStringLiteral("username")).toString();
            QString avatar  = obj.value(QStringLiteral("avatar_url")).toString();
            m_authenticated = true;
            emit authenticatedChanged();
            emit usernameChanged();
            onOk(m_userId, m_username, avatar);
        },
        [this, onErr](QString err) {
            m_token.clear();
            emit authenticatedChanged();
            onErr(err);
        });
}

// ── Repository operations ─────────────────────────────────────────────────────

void GitLabClient::createRepository(const QString &name,
                                     std::function<void(int, QString, QString)> onOk,
                                     std::function<void(QString)>               onErr)
{
    QJsonObject body;
    body[QStringLiteral("name")]       = name;
    body[QStringLiteral("visibility")] = QStringLiteral("private");
    body[QStringLiteral("initialize_with_readme")] = true;

    QNetworkRequest req = buildRequest(QStringLiteral("/projects"));
    QNetworkReply *reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    handleReply(reply,
        [onOk](QByteArray data) {
            auto doc = QJsonDocument::fromJson(data);
            if (!doc.isObject()) return;
            QJsonObject obj = doc.object();
            int    id  = obj.value(QStringLiteral("id")).toInt();
            QString http = obj.value(QStringLiteral("http_url_to_repo")).toString();
            QString ssh  = obj.value(QStringLiteral("ssh_url_to_repo")).toString();
            onOk(id, http, ssh);
        },
        onErr);
}

void GitLabClient::getRepository(const QString &projectPath,
                                  std::function<void(QJsonObject)> onOk,
                                  std::function<void(QString)>     onErr)
{
    QString encoded = QString::fromUtf8(
        QUrl::toPercentEncoding(projectPath));
    QNetworkRequest req = buildRequest(QStringLiteral("/projects/") + encoded);
    QNetworkReply *reply = m_nam.get(req);

    handleReply(reply,
        [onOk](QByteArray data) {
            auto doc = QJsonDocument::fromJson(data);
            if (doc.isObject())
                onOk(doc.object());
        },
        onErr);
}

void GitLabClient::addProjectMember(int projectId,
                                     int userId,
                                     std::function<void()>        onOk,
                                     std::function<void(QString)> onErr)
{
    QJsonObject body;
    body[QStringLiteral("user_id")]      = userId;
    body[QStringLiteral("access_level")] = kDeveloperAccessLevel;

    QString ep = QStringLiteral("/projects/%1/members").arg(projectId);
    QNetworkRequest req = buildRequest(ep);
    QNetworkReply *reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    handleReply(reply, [onOk](QByteArray) { onOk(); }, onErr);
}

// ── File / commit operations ──────────────────────────────────────────────────

void GitLabClient::getFileContent(const QString &projectPath,
                                   const QString &filePath,
                                   const QString &ref,
                                   std::function<void(QByteArray)> onOk,
                                   std::function<void(QString)>    onErr)
{
    QString encodedProject = QString::fromUtf8(QUrl::toPercentEncoding(projectPath));
    QString encodedFile    = QString::fromUtf8(QUrl::toPercentEncoding(filePath));
    QString refParam       = ref.isEmpty() ? QStringLiteral("main") : ref;

    QString ep = QStringLiteral("/projects/%1/repository/files/%2/raw?ref=%3")
                     .arg(encodedProject, encodedFile, refParam);

    QNetworkRequest req = buildRequest(ep);
    QNetworkReply *reply = m_nam.get(req);

    handleReply(reply, onOk, onErr);
}

void GitLabClient::commitFile(const QString   &projectPath,
                               const QString   &filePath,
                               const QByteArray &content,
                               const QString   &commitMessage,
                               const QString   &branch,
                               std::function<void()>        onOk,
                               std::function<void(QString)> onErr)
{
    QString encodedProject = QString::fromUtf8(QUrl::toPercentEncoding(projectPath));
    QString encodedFile    = QString::fromUtf8(QUrl::toPercentEncoding(filePath));

    // We use the "commits" API with an actions array so we can create-or-update
    // without a separate check.
    QJsonObject action;
    action[QStringLiteral("action")]    = QStringLiteral("create");
    action[QStringLiteral("file_path")] = filePath;
    action[QStringLiteral("content")]   = QString::fromUtf8(content.toBase64());
    action[QStringLiteral("encoding")]  = QStringLiteral("base64");

    QJsonObject body;
    body[QStringLiteral("branch")]         = branch.isEmpty() ? QStringLiteral("main") : branch;
    body[QStringLiteral("commit_message")] = commitMessage;
    body[QStringLiteral("actions")]        = QJsonArray{action};

    QString ep = QStringLiteral("/projects/%1/repository/commits").arg(encodedProject);
    QNetworkRequest req = buildRequest(ep);
    QNetworkReply *reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    // GitLab returns 400 if the file already exists with action=create.
    // Re-try with action=update in that case.
    connect(reply, &QNetworkReply::finished, this,
        [this, reply, projectPath, filePath, content, commitMessage, branch, onOk, onErr]() {
            reply->deleteLater();
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() == QNetworkReply::NoError) {
                onOk();
                return;
            }
            if (httpStatus == 400) {
                // Try update
                QString encodedProject2 = QString::fromUtf8(QUrl::toPercentEncoding(projectPath));

                QJsonObject action2;
                action2[QStringLiteral("action")]    = QStringLiteral("update");
                action2[QStringLiteral("file_path")] = filePath;
                action2[QStringLiteral("content")]   = QString::fromUtf8(content.toBase64());
                action2[QStringLiteral("encoding")]  = QStringLiteral("base64");

                QJsonObject body2;
                body2[QStringLiteral("branch")]         = branch.isEmpty() ? QStringLiteral("main") : branch;
                body2[QStringLiteral("commit_message")] = commitMessage;
                body2[QStringLiteral("actions")]        = QJsonArray{action2};

                QString ep2 = QStringLiteral("/projects/%1/repository/commits").arg(encodedProject2);
                QNetworkRequest req2 = buildRequest(ep2);
                QNetworkReply *reply2 = m_nam.post(req2, QJsonDocument(body2).toJson(QJsonDocument::Compact));
                handleReply(reply2, [onOk](QByteArray) { onOk(); }, onErr);
            } else {
                const QByteArray body3 = reply->readAll();
                QString detail;
                auto doc = QJsonDocument::fromJson(body3);
                if (!doc.isNull() && doc.isObject())
                    detail = doc.object().value(QStringLiteral("message")).toString();
                if (detail.isEmpty())
                    detail = reply->errorString();
                onErr(detail);
            }
        });
}

void GitLabClient::getFileCommits(const QString &projectPath,
                                   const QString &filePath,
                                   std::function<void(QJsonArray)> onOk,
                                   std::function<void(QString)>    onErr)
{
    QString encodedProject = QString::fromUtf8(QUrl::toPercentEncoding(projectPath));
    QString encodedFile    = QString::fromUtf8(QUrl::toPercentEncoding(filePath));

    QString ep = QStringLiteral("/projects/%1/repository/commits?path=%2&per_page=100")
                     .arg(encodedProject, encodedFile);

    QNetworkRequest req = buildRequest(ep);
    QNetworkReply *reply = m_nam.get(req);

    handleReply(reply,
        [onOk](QByteArray data) {
            auto doc = QJsonDocument::fromJson(data);
            if (doc.isArray())
                onOk(doc.array());
            else
                onOk(QJsonArray{});
        },
        onErr);
}

// ── User lookup ───────────────────────────────────────────────────────────────

void GitLabClient::findUser(const QString &username,
                             std::function<void(int, QString, QString)> onOk,
                             std::function<void(QString)>               onErr)
{
    QString ep = QStringLiteral("/users?username=") +
                 QString::fromUtf8(QUrl::toPercentEncoding(username));

    QNetworkRequest req = buildRequest(ep);
    QNetworkReply *reply = m_nam.get(req);

    handleReply(reply,
        [onOk, onErr, username](QByteArray data) {
            auto doc = QJsonDocument::fromJson(data);
            if (!doc.isArray() || doc.array().isEmpty()) {
                onErr(QStringLiteral("User '%1' not found").arg(username));
                return;
            }
            QJsonObject user = doc.array().first().toObject();
            int    id     = user.value(QStringLiteral("id")).toInt();
            QString uname = user.value(QStringLiteral("username")).toString();
            QString avatar = user.value(QStringLiteral("avatar_url")).toString();
            onOk(id, uname, avatar);
        },
        onErr);
}
