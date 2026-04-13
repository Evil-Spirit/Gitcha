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
        usernameField.forceActiveFocus()
    }

    onAccepted: {
        const uname = usernameField.text.trim()
        if (uname.length === 0) return
        app.addContact(uname, "")
    }

    // ── Form ──────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            text: "Remote user's GitLab username:"
            font.pixelSize: 12
            color: "#B39DDB"
        }
        TextField {
            id: usernameField
            Layout.fillWidth: true
            placeholderText: "e.g. bob"
            inputMethodHints: Qt.ImhNoAutoUppercase
        }

        Label {
            text: "Both users must be on the same GitLab instance. The repository will be created and linked automatically."
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            font.pixelSize: 11
            color: "#9575CD"
        }
    }
}
