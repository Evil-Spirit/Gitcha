#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

struct Message {
    QString   id;          ///< Unique identifier (ISO timestamp + sender)
    QString   sender;      ///< GitLab username of the sender
    QString   text;        ///< Plain-text body of the message
    QString   htmlContent; ///< Full HTML representation stored in the file
    QDateTime timestamp;
    bool      isMine{false};
};

/**
 * MessageStore serialises and deserialises chat messages to/from a single
 * HTML file that is committed to a GitLab repository.
 *
 * Format (append-only):
 * ─────────────────────
 * The file is a valid HTML5 document.  Each message is wrapped in a
 * <article> element with a stable "data-id" attribute so it can be
 * identified and rendered in different contexts.
 *
 * Example snippet:
 *   <article data-id="2024-01-01T12:00:00Z|alice"
 *            data-sender="alice"
 *            data-ts="2024-01-01T12:00:00Z"
 *            class="message">
 *     <header><b>alice</b> <time>2024-01-01 12:00</time></header>
 *     <p>Hello world!</p>
 *   </article>
 *
 * Because messages are committed as a growing HTML file, any web browser
 * (or the in-app WebView) can render the conversation history including
 * hyperlinks to internet resources or local git-repo files.
 */
class MessageStore : public QObject
{
    Q_OBJECT

public:
    explicit MessageStore(QObject *parent = nullptr);

    /**
     * Serialises a single new message into an HTML snippet that can be
     * appended to the conversation file.
     *
     * @param sender   GitLab username of the author
     * @param text     Raw text entered by the user (HTML-escaped internally)
     * @param timestamp Point-in-time of the message
     * @return  HTML <article> block ready to be appended
     */
    static QString encodeMessage(const QString   &sender,
                                  const QString   &text,
                                  const QDateTime &timestamp);

    /**
     * Appends a new message snippet to an existing full HTML document.
     *
     * @param existingHtml  Current content of the messages.html file
     *                      (may be empty for a new conversation)
     * @param sender        Author username
     * @param text          Message text
     * @param timestamp     Timestamp
     * @return  Updated full HTML document
     */
    static QByteArray appendMessage(const QByteArray &existingHtml,
                                     const QString    &sender,
                                     const QString    &text,
                                     const QDateTime  &timestamp);

    /**
     * Parses a full HTML conversation document and returns an ordered list of
     * Message structs (oldest first).
     *
     * @param html          Raw bytes of the messages.html file
     * @param currentUser   Username of the local user (sets Message::isMine)
     */
    static QList<Message> parseMessages(const QByteArray &html,
                                         const QString    &currentUser);

    /** Returns an empty HTML document skeleton for a brand-new conversation. */
    static QByteArray emptyDocument(const QString &title);

private:
    static QString escapeHtml(const QString &text);
};
