#include "messagestore.h"

#include <QRegularExpression>

// ── HTML escaping ─────────────────────────────────────────────────────────────

QString MessageStore::escapeHtml(const QString &text)
{
    QString out = text;
    out.replace(QLatin1Char('&'),  QStringLiteral("&amp;"));
    out.replace(QLatin1Char('<'),  QStringLiteral("&lt;"));
    out.replace(QLatin1Char('>'),  QStringLiteral("&gt;"));
    out.replace(QLatin1Char('"'),  QStringLiteral("&quot;"));
    out.replace(QLatin1Char('\''), QStringLiteral("&#39;"));
    return out;
}

// ── Constructor ───────────────────────────────────────────────────────────────

MessageStore::MessageStore(QObject *parent)
    : QObject(parent)
{}

// ── encodeMessage ─────────────────────────────────────────────────────────────

QString MessageStore::encodeMessage(const QString   &sender,
                                     const QString   &text,
                                     const QDateTime &timestamp)
{
    const QString ts       = timestamp.toUTC().toString(Qt::ISODate);
    const QString tsHuman  = timestamp.toLocalTime().toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    const QString msgId    = ts + QLatin1Char('|') + sender;
    const QString safeText = escapeHtml(text);
    const QString safeSend = escapeHtml(sender);

    return QStringLiteral(
        "  <article data-id=\"%1\" data-sender=\"%2\" data-ts=\"%3\" class=\"message\">\n"
        "    <header><b>%4</b> <time datetime=\"%3\">%5</time></header>\n"
        "    <p>%6</p>\n"
        "  </article>\n")
        .arg(escapeHtml(msgId), safeSend, ts, safeSend, tsHuman, safeText);
}

// ── emptyDocument ─────────────────────────────────────────────────────────────

QByteArray MessageStore::emptyDocument(const QString &title)
{
    QString html = QStringLiteral(
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "  <title>%1</title>\n"
        "  <style>\n"
        "    body { font-family: sans-serif; margin: 0; padding: 8px; background: #f9f9f9; }\n"
        "    .message { background: white; border-radius: 8px; margin: 6px 0;\n"
        "               padding: 8px 12px; box-shadow: 0 1px 2px rgba(0,0,0,.12); }\n"
        "    .message.mine { background: #dcf8c6; margin-left: 20%; }\n"
        "    header { font-size: .8em; color: #555; margin-bottom: 4px; }\n"
        "    p { margin: 0; white-space: pre-wrap; word-wrap: break-word; }\n"
        "    a { color: #0066cc; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "<!-- messages -->\n"
        "</body>\n"
        "</html>\n")
        .arg(escapeHtml(title));
    return html.toUtf8();
}

// ── appendMessage ─────────────────────────────────────────────────────────────

QByteArray MessageStore::appendMessage(const QByteArray &existingHtml,
                                        const QString    &sender,
                                        const QString    &text,
                                        const QDateTime  &timestamp)
{
    QString doc = existingHtml.isEmpty()
                  ? QString::fromUtf8(emptyDocument(QStringLiteral("Conversation")))
                  : QString::fromUtf8(existingHtml);

    const QString snippet = encodeMessage(sender, text, timestamp);

    // Insert before </body>
    const int insertPos = doc.lastIndexOf(QStringLiteral("</body>"));
    if (insertPos == -1) {
        // Malformed document – just append
        doc += snippet;
    } else {
        doc.insert(insertPos, snippet);
    }
    return doc.toUtf8();
}

// ── parseMessages ─────────────────────────────────────────────────────────────

QList<Message> MessageStore::parseMessages(const QByteArray &html,
                                            const QString    &currentUser)
{
    QList<Message> messages;
    if (html.isEmpty())
        return messages;

    const QString doc = QString::fromUtf8(html);

    // Match each <article ...> block
    static const QRegularExpression articleRx(
        QStringLiteral(R"(<article([^>]*)>(.*?)</article>)"),
        QRegularExpression::DotMatchesEverythingOption |
        QRegularExpression::CaseInsensitiveOption);

    static const QRegularExpression attrRx(
        QStringLiteral(R"(data-([a-z]+)=\"([^\"]*)\")"),
        QRegularExpression::CaseInsensitiveOption);

    static const QRegularExpression pRx(
        QStringLiteral(R"(<p>(.*?)</p>)"),
        QRegularExpression::DotMatchesEverythingOption |
        QRegularExpression::CaseInsensitiveOption);

    auto it = articleRx.globalMatch(doc);
    while (it.hasNext()) {
        auto m = it.next();
        const QString attribs  = m.captured(1);
        const QString bodyHtml = m.captured(2);

        // Parse attributes
        QMap<QString, QString> attrs;
        auto ait = attrRx.globalMatch(attribs);
        while (ait.hasNext()) {
            auto am = ait.next();
            attrs[am.captured(1).toLower()] = am.captured(2);
        }

        if (!attrs.contains(QStringLiteral("id")))
            continue;

        // Extract plain text from <p>
        QString plainText;
        auto pm = pRx.match(bodyHtml);
        if (pm.hasMatch()) {
            plainText = pm.captured(1);
            // Unescape common HTML entities
            plainText.replace(QStringLiteral("&amp;"),  QStringLiteral("&"));
            plainText.replace(QStringLiteral("&lt;"),   QStringLiteral("<"));
            plainText.replace(QStringLiteral("&gt;"),   QStringLiteral(">"));
            plainText.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
            plainText.replace(QStringLiteral("&#39;"),  QStringLiteral("'"));
        }

        Message msg;
        msg.id          = attrs.value(QStringLiteral("id"));
        msg.sender      = attrs.value(QStringLiteral("sender"));
        msg.timestamp   = QDateTime::fromString(attrs.value(QStringLiteral("ts")), Qt::ISODate);
        msg.text        = plainText;
        msg.htmlContent = m.captured(0);
        msg.isMine      = (msg.sender == currentUser);

        messages.append(msg);
    }

    return messages;
}
