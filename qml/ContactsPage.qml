import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: contactsRoot
    title: "Chats"

    // ── Toolbar ───────────────────────────────────────────────────────────────
    header: ToolBar {
        Material.background: Material.DeepPurple
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4

            Label {
                text: "Gitcha"
                font.pixelSize: 18
                font.bold: true
                color: "white"
                Layout.fillWidth: true
                leftPadding: 8
            }

            ToolButton {
                icon.source: "image://theme/go-home"
                icon.color: "white"
                text: "⚙"
                font.pixelSize: 18
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: ApplicationWindow.window.goToSettings()
            }
        }
    }

    // ── Contact list ──────────────────────────────────────────────────────────
    ListView {
        id: listView
        anchors.fill: parent
        model: contactModel
        clip: true

        delegate: ItemDelegate {
            width: listView.width
            height: 64

            contentItem: RowLayout {
                spacing: 12
                anchors.verticalCenter: parent.verticalCenter

                // Avatar circle
                Rectangle {
                    width: 40; height: 40
                    radius: 20
                    color: Material.accent
                    Label {
                        anchors.centerIn: parent
                        text: contactUsername.length > 0
                              ? contactUsername[0].toUpperCase()
                              : "?"
                        color: "white"
                        font.pixelSize: 18
                        font.bold: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        text: contactUsername
                        font.pixelSize: 15
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: localRepo || "No local repo"
                        font.pixelSize: 11
                        color: "#9575CD"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            onClicked: ApplicationWindow.window.goToChat(index)
        }

        // ── Empty state ───────────────────────────────────────────────────────
        Label {
            anchors.centerIn: parent
            visible: contactModel.count === 0
            text: "No contacts yet.\nTap + to add someone."
            horizontalAlignment: Text.AlignHCenter
            color: "#9575CD"
            font.pixelSize: 14
        }
    }

    // ── FAB: add contact ──────────────────────────────────────────────────────
    RoundButton {
        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: 16
        }
        icon.width: 24
        icon.height: 24
        text: "+"
        font.pixelSize: 24
        Material.background: Material.DeepPurple
        Material.foreground: "white"
        onClicked: addDialog.open()
    }

    // ── Loading overlay ───────────────────────────────────────────────────────
    BusyIndicator {
        anchors.centerIn: parent
        running: app.loading
        visible: running
    }

    // ── Add-contact dialog ────────────────────────────────────────────────────
    AddContactDialog {
        id: addDialog
    }
}
