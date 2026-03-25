import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: addContactRoot
    title: "Add Contact"
    modal: true
    anchors.centerIn: parent
    width: Math.min(ApplicationWindow.window.width - 32, 360)
    standardButtons: Dialog.Cancel | Dialog.Ok

    // ── Reset on open ─────────────────────────────────────────────────────────
    onOpened: {
        usernameField.text = ""
        repoUrlField.text  = ""
        usernameField.forceActiveFocus()
    }

    onAccepted: {
        const uname   = usernameField.text.trim()
        const repoUrl = repoUrlField.text.trim()
        if (uname.length === 0) return
        app.addContact(uname, repoUrl)
    }

    // ── Form ──────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "Remote user's GitLab username:"
            font.pixelSize: 12
            color: "#757575"
        }
        TextField {
            id: usernameField
            Layout.fillWidth: true
            placeholderText: "e.g. bob"
            inputMethodHints: Qt.ImhNoAutoUppercase
        }

        Label {
            text: "Remote user's repo URL (optional):"
            font.pixelSize: 12
            color: "#757575"
        }
        TextField {
            id: repoUrlField
            Layout.fillWidth: true
            placeholderText: "https://gitlab.com/bob/chat-with-alice"
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
        }

        Label {
            text: "Ask your contact to share their repo URL from the chat screen (🔗 button)."
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            font.pixelSize: 11
            color: "#9e9e9e"
        }
    }
}
