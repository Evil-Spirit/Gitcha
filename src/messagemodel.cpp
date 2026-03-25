#include "messagemodel.h"

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_messages.size());
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_messages.size()))
        return {};

    const Message &msg = m_messages.at(index.row());
    switch (role) {
    case IdRole:        return msg.id;
    case SenderRole:    return msg.sender;
    case TextRole:      return msg.text;
    case TimestampRole: return msg.timestamp.isValid()
                               ? msg.timestamp.toLocalTime().toString(QStringLiteral("hh:mm"))
                               : QString{};
    case IsMineRole:    return msg.isMine;
    case HtmlRole:      return msg.htmlContent;
    default:            return {};
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    return {
        {IdRole,        "msgId"},
        {SenderRole,    "sender"},
        {TextRole,      "text"},
        {TimestampRole, "timestamp"},
        {IsMineRole,    "isMine"},
        {HtmlRole,      "htmlContent"},
    };
}

void MessageModel::setMessages(const QList<Message> &messages)
{
    beginResetModel();
    m_messages = messages;
    endResetModel();
    emit countChanged();
}

void MessageModel::appendMessage(const Message &message)
{
    const int row = static_cast<int>(m_messages.size());
    beginInsertRows({}, row, row);
    m_messages.append(message);
    endInsertRows();
    emit countChanged();
}

void MessageModel::clear()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
    emit countChanged();
}
