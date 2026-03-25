#pragma once

#include "messagestore.h"

#include <QAbstractListModel>
#include <QList>

/**
 * MessageModel is a QAbstractListModel that exposes a list of Message objects
 * to QML.
 *
 * Roles:
 *   - idRole      : QString  – stable message identifier
 *   - senderRole  : QString  – author username
 *   - textRole    : QString  – plain-text body
 *   - timestampRole : QString – human-readable timestamp
 *   - isMineRole  : bool     – true when the local user sent this message
 *   - htmlRole    : QString  – raw HTML <article> snippet
 */
class MessageModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        SenderRole,
        TextRole,
        TimestampRole,
        IsMineRole,
        HtmlRole
    };

    explicit MessageModel(QObject *parent = nullptr);

    // ── QAbstractListModel interface ──────────────────────────────────────────
    int      rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── Mutation ──────────────────────────────────────────────────────────────

    /** Replace the full list (used after a refresh from the remote). */
    void setMessages(const QList<Message> &messages);

    /** Append a single message (used optimistically after sending). */
    void appendMessage(const Message &message);

    /** Clear all messages. */
    void clear();

signals:
    void countChanged();

private:
    QList<Message> m_messages;
};
