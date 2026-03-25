import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: chatRoot

    property string contactName: app.activeChatName

    title: contactName || "Chat"

    // ── Toolbar ───────────────────────────────────────────────────────────────
    header: ToolBar {
        Material.background: Material.Teal
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "‹"
                font.pixelSize: 22
                contentItem: Text {
                    text: parent.text; color: "white"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: ApplicationWindow.window.goBack()
            }

            // Avatar circle
            Rectangle {
                width: 32; height: 32; radius: 16
                color: "white"
                opacity: 0.3
                Label {
                    anchors.centerIn: parent
                    text: chatRoot.contactName.length > 0
                          ? chatRoot.contactName[0].toUpperCase()
                          : "?"
                    color: "white"
                    font.pixelSize: 16; font.bold: true
                }
            }

            Label {
                text: chatRoot.title
                font.pixelSize: 18
                color: "white"
                Layout.fillWidth: true
                leftPadding: 6
            }

            ToolButton {
                text: "↻"
                font.pixelSize: 18
                contentItem: Text {
                    text: parent.text; color: "white"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: app.refreshMessages()
                ToolTip.text: "Refresh messages"
                ToolTip.visible: hovered
            }

            ToolButton {
                text: "🔗"
                font.pixelSize: 16
                contentItem: Text {
                    text: parent.text; color: "white"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: repoUrlPopup.open()
                ToolTip.text: "Share your repo URL"
                ToolTip.visible: hovered
            }
        }
    }

    // ── Message list ──────────────────────────────────────────────────────────
    ListView {
        id: msgList
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: inputBar.top
        }
        model: messageModel
        clip: true
        spacing: 4
        topMargin: 8
        bottomMargin: 8
        verticalLayoutDirection: ListView.TopToBottom

        delegate: Item {
            width: msgList.width
            height: bubble.implicitHeight + 8

            // Bubble
            Rectangle {
                id: bubble
                width: Math.min(msgText.implicitWidth + 24, msgList.width * 0.75)
                implicitHeight: msgText.implicitHeight + 16
                radius: 12
                color: isMine ? "#dcf8c6" : "white"
                border.color: isMine ? "#b2d89a" : "#e0e0e0"
                border.width: 1

                x: isMine ? parent.width - width - 8 : 8
                y: 4

                ColumnLayout {
                    id: msgText
                    anchors {
                        left: parent.left; right: parent.right
                        top: parent.top; bottom: parent.bottom
                        margins: 8
                    }
                    spacing: 2

                    Label {
                        visible: !isMine
                        text: sender
                        font.pixelSize: 11
                        font.bold: true
                        color: Material.accent
                        Layout.fillWidth: true
                    }

                    Label {
                        text: model.text || ""
                        font.pixelSize: 14
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        // Allow links in messages to be opened
                        onLinkActivated: Qt.openUrlExternally(link)
                        textFormat: Text.StyledText
                    }

                    Label {
                        text: timestamp
                        font.pixelSize: 10
                        color: "#9e9e9e"
                        Layout.alignment: Qt.AlignRight
                    }
                }
            }
        }

        onCountChanged: positionViewAtEnd()

        // ── Empty state ────────────────────────────────────────────────────────
        Label {
            anchors.centerIn: parent
            visible: messageModel.count === 0 && !app.loading
            text: "No messages yet.\nSay hello! 👋"
            horizontalAlignment: Text.AlignHCenter
            color: "#9e9e9e"
            font.pixelSize: 14
        }
    }

    // ── Loading overlay ───────────────────────────────────────────────────────
    BusyIndicator {
        anchors.centerIn: parent
        running: app.loading
        visible: running
    }

    // ── Input bar ─────────────────────────────────────────────────────────────
    Pane {
        id: inputBar
        anchors {
            left: parent.left; right: parent.right; bottom: parent.bottom
        }
        padding: 0
        Material.elevation: 4

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            TextArea {
                id: msgInput
                Layout.fillWidth: true
                placeholderText: "Message…"
                wrapMode: TextArea.Wrap
                font.pixelSize: 14
                Keys.onReturnPressed: {
                    if (event.modifiers & Qt.ShiftModifier) {
                        event.accepted = false
                    } else {
                        sendBtn.clicked()
                        event.accepted = true
                    }
                }
            }

            Button {
                id: sendBtn
                text: app.sending ? "…" : "Send"
                enabled: !app.sending && msgInput.text.trim().length > 0
                Material.background: Material.Teal
                Material.foreground: "white"
                onClicked: {
                    const txt = msgInput.text.trim()
                    if (txt.length === 0) return
                    app.sendMessage(txt)
                    msgInput.text = ""
                }
            }
        }
    }

    // ── Repo URL popup (for sharing with contact) ─────────────────────────────
    Popup {
        id: repoUrlPopup
        modal: true
        focus: true
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 360)
        padding: 16

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: "Share your repo URL"
                font.bold: true
                font.pixelSize: 15
            }
            Label {
                text: "Share this URL with your contact so they can connect to your repository:"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                font.pixelSize: 13
                color: "#555"
            }
            TextArea {
                id: repoUrlArea
                Layout.fillWidth: true
                readOnly: true
                text: app.localRepoUrlForActiveChat()
                wrapMode: TextArea.Wrap
                font.pixelSize: 12
                selectByMouse: true
            }
            Button {
                text: "Copy"
                onClicked: {
                    repoUrlArea.selectAll()
                    repoUrlArea.copy()
                    repoUrlPopup.close()
                }
                Material.background: Material.Teal
                Material.foreground: "white"
            }
        }
    }

    // ── Load messages on open ─────────────────────────────────────────────────
    Component.onCompleted: {
        messageModel.clear()
        app.refreshMessages()
    }
}
