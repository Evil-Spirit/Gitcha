#pragma once

#include "contactmanager.h"

#include <QAbstractListModel>
#include <QList>

/**
 * ContactModel is a QAbstractListModel wrapping a QList<Contact>.
 *
 * Roles:
 *   - usernameRole    : QString – GitLab username
 *   - avatarRole      : QString – avatar URL
 *   - localRepoRole   : QString – local repo path (e.g. "alice/chat-with-bob")
 *   - remoteRepoRole  : QString – remote repo URL
 */
class ContactModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        UsernameRole = Qt::UserRole + 1,
        AvatarRole,
        LocalRepoRole,
        RemoteRepoRole,
    };

    explicit ContactModel(QObject *parent = nullptr);

    // ── QAbstractListModel interface ──────────────────────────────────────────
    int      rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── Mutation ──────────────────────────────────────────────────────────────

    /** Replace the entire contact list. */
    void setContacts(const QList<Contact> &contacts);

    /** Append a single contact. */
    void appendContact(const Contact &contact);

    /** Returns the Contact at @p row (used from C++). */
    Contact contactAt(int row) const;

signals:
    void countChanged();

private:
    QList<Contact> m_contacts;
};
