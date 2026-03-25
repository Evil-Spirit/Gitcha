import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 420
    height: 700
    title: "Gitcha"

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple
    Material.primary: Material.DeepPurple

    // ── Page stack ────────────────────────────────────────────────────────────
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: app.authenticated ? contactsPage : loginPage
    }

    Component { id: loginPage;    LoginPage    {} }
    Component { id: contactsPage; ContactsPage {} }
    Component { id: chatPageComp; ChatPage     {} }
    Component { id: settingsComp; SettingsPage {} }

    // ── Navigation helpers (callable from child pages) ────────────────────────
    function goToContacts() {
        stackView.replace(contactsPage)
    }

    function goToChat(index) {
        app.activeChatIndex = index
        stackView.push(chatPageComp)
    }

    function goToSettings() {
        stackView.push(settingsComp)
    }

    function goBack() {
        if (stackView.depth > 1)
            stackView.pop()
    }

    // ── Reactivity ───────────────────────────────────────────────────────────
    Connections {
        target: app
        function onAuthenticatedChanged() {
            if (app.authenticated)
                stackView.replace(contactsPage)
            else
                stackView.replace(loginPage)
        }
    }

    // ── Global status snack-bar ───────────────────────────────────────────────
    Popup {
        id: snackBar
        width: Math.min(parent.width - 32, 360)
        height: snackLabel.implicitHeight + 24
        x: (parent.width - width) / 2
        y: parent.height - height - 16
        padding: 12
        modal: false
        focus: false
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: "#2D1B69"
            radius: 4
        }

        Label {
            id: snackLabel
            anchors.centerIn: parent
            color: "white"
            font.pixelSize: 13
            wrapMode: Text.Wrap
            width: snackBar.width - 24
        }

        Timer {
            id: snackTimer
            interval: 3000
            onTriggered: snackBar.close()
        }
    }

    Connections {
        target: app
        function onStatusMessageChanged() {
            if (app.statusMessage.length > 0) {
                snackLabel.text = app.statusMessage
                snackBar.open()
                snackTimer.restart()
            }
        }
    }
}
