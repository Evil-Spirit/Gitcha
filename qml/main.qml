import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtMultimedia

ApplicationWindow {
    id: root
    visible: true
    width: 420
    height: 700
    title: "Gitcha"

    Material.theme: Material.Light
    Material.accent: Material.Teal
    Material.primary: Material.Teal

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

    // ── Notification sound ────────────────────────────────────────────────────
    SoundEffect {
        id: msgSound
        source: "qrc:/Gitcha/assets/message_notification.wav"
    }

    // ── New-message notification popup ────────────────────────────────────────
    Popup {
        id: msgNotification
        width: Math.min(parent.width - 32, 360)
        height: notifContent.implicitHeight + 24
        x: (parent.width - width) / 2
        y: 16
        padding: 12
        modal: false
        focus: false
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: "#1b5e20"
            radius: 8
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 180 }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 180 }
        }

        RowLayout {
            id: notifContent
            width: msgNotification.width - 24
            spacing: 10

            Label {
                text: "💬"
                font.pixelSize: 18
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    id: notifSender
                    font.bold: true
                    font.pixelSize: 13
                    color: "white"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Label {
                    id: notifPreview
                    font.pixelSize: 12
                    color: "#c8e6c9"
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        Timer {
            id: notifTimer
            interval: 4000
            onTriggered: msgNotification.close()
        }
    }

    Connections {
        target: app
        function onNewMessageReceived(sender, preview) {
            notifSender.text  = sender
            notifPreview.text = preview
            msgNotification.open()
            notifTimer.restart()
            msgSound.play()
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
            color: "#323232"
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
