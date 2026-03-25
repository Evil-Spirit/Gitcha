#include "contactmodel.h"

ContactModel::ContactModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int ContactModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_contacts.size());
}

QVariant ContactModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_contacts.size()))
        return {};

    const Contact &c = m_contacts.at(index.row());
    switch (role) {
    case UsernameRole:   return c.username;
    case AvatarRole:     return c.avatarUrl;
    case LocalRepoRole:  return c.localRepoPath;
    case RemoteRepoRole: return c.remoteRepoUrl;
    default:             return {};
    }
}

QHash<int, QByteArray> ContactModel::roleNames() const
{
    return {
        {UsernameRole,   "contactUsername"},
        {AvatarRole,     "contactAvatar"},
        {LocalRepoRole,  "localRepo"},
        {RemoteRepoRole, "remoteRepo"},
    };
}

void ContactModel::setContacts(const QList<Contact> &contacts)
{
    beginResetModel();
    m_contacts = contacts;
    endResetModel();
    emit countChanged();
}

void ContactModel::appendContact(const Contact &contact)
{
    const int row = static_cast<int>(m_contacts.size());
    beginInsertRows({}, row, row);
    m_contacts.append(contact);
    endInsertRows();
    emit countChanged();
}

Contact ContactModel::contactAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_contacts.size()))
        return {};
    return m_contacts.at(row);
}
