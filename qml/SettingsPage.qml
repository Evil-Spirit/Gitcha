import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: settingsRoot
    title: "Settings"

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

            Label {
                text: "Settings"
                font.pixelSize: 18; color: "white"
                Layout.fillWidth: true
                leftPadding: 4
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width

        ColumnLayout {
            width: Math.min(parent.width, 480)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 0

            // ── Account section ──────────────────────────────────────────────
            Label {
                text: "ACCOUNT"
                font.pixelSize: 11
                color: Material.accent
                leftPadding: 16
                topPadding: 16
                bottomPadding: 4
            }

            Pane {
                Layout.fillWidth: true
                Material.elevation: 1
                padding: 16

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Label { text: "Logged in as:"; color: "#757575"; font.pixelSize: 13 }
                        Label {
                            text: app.currentUser || "(not logged in)"
                            font.bold: true
                            font.pixelSize: 13
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        Label { text: "Server:"; color: "#757575"; font.pixelSize: 13 }
                        Label {
                            text: settings.serverUrl
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Button {
                        text: "Sign Out"
                        Layout.alignment: Qt.AlignLeft
                        Material.background: Material.Red
                        Material.foreground: "white"
                        onClicked: {
                            app.logout()
                            ApplicationWindow.window.goBack()
                        }
                    }
                }
            }

            // ── About section ─────────────────────────────────────────────────
            Label {
                text: "ABOUT"
                font.pixelSize: 11
                color: Material.accent
                leftPadding: 16
                topPadding: 24
                bottomPadding: 4
            }

            Pane {
                Layout.fillWidth: true
                Material.elevation: 1
                padding: 16

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    Label {
                        text: "Gitcha v1.0"
                        font.bold: true
                        font.pixelSize: 14
                    }
                    Label {
                        text: "Git-backed cross-platform secure messenger.\n" +
                              "Messages are stored as HTML files committed to\n" +
                              "private GitLab repositories — no additional\n" +
                              "server software required."
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        color: "#555"
                        Layout.fillWidth: true
                    }
                    Label {
                        text: "Built with Qt 6 / QML"
                        font.pixelSize: 11
                        color: "#9e9e9e"
                    }
                }
            }

            Item { height: 24 }
        }
    }
}
