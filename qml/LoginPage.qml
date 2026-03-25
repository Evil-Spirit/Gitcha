import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: loginRoot
    title: "Gitcha – Sign In"

    // ── Background ────────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: "#f5f5f5"
    }

    // ── Auto-fill from saved settings ─────────────────────────────────────────
    Component.onCompleted: {
        serverField.text = settings.serverUrl
        if (settings.rememberMe) {
            tokenField.text = settings.accessToken
            rememberCheck.checked = true
        }
    }

    // ── Content ───────────────────────────────────────────────────────────────
    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width

        ColumnLayout {
            width: Math.min(parent.width, 400)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 16

            // ── Logo / title ─────────────────────────────────────────────────
            Item { height: 40 }

            Image {
                source: "qrc:/assets/logo.png"
                Layout.alignment: Qt.AlignHCenter
                width: 72; height: 72
                fillMode: Image.PreserveAspectFit
                // Fallback if logo is missing
                visible: status === Image.Ready
            }

            Label {
                text: "Gitcha"
                font.pixelSize: 28
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                color: Material.primary
            }

            Label {
                text: "Git-backed secure messaging"
                font.pixelSize: 14
                color: "#757575"
                Layout.alignment: Qt.AlignHCenter
            }

            Item { height: 8 }

            // ── Form card ────────────────────────────────────────────────────
            Pane {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Material.elevation: 2

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    Label { text: "GitLab Server URL"; font.pixelSize: 12; color: "#757575" }
                    TextField {
                        id: serverField
                        Layout.fillWidth: true
                        placeholderText: "https://gitlab.com"
                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                        text: "https://gitlab.com"
                    }

                    Label { text: "Personal Access Token"; font.pixelSize: 12; color: "#757575" }
                    TextField {
                        id: tokenField
                        Layout.fillWidth: true
                        placeholderText: "glpat-xxxxxxxxxxxxxxxxxxxx"
                        echoMode: TextInput.Password
                        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoAutoUppercase
                    }

                    RowLayout {
                        CheckBox {
                            id: rememberCheck
                            text: "Remember me"
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Button {
                        id: loginButton
                        text: app.loading ? "Signing in…" : "Sign In"
                        enabled: !app.loading && serverField.text.length > 0 &&
                                 tokenField.text.length > 0
                        Layout.fillWidth: true
                        Material.background: Material.Teal
                        Material.foreground: "white"
                        onClicked: {
                            app.login(serverField.text.trim(),
                                      tokenField.text.trim(),
                                      rememberCheck.checked)
                        }
                    }
                }
            }

            // ── Help text ────────────────────────────────────────────────────
            Label {
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                text: "Create a GitLab Personal Access Token with <b>api</b> scope at:\n" +
                      serverField.text + "/-/profile/personal_access_tokens"
                font.pixelSize: 11
                color: "#9e9e9e"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            Item { height: 20 }
        }
    }

    // ── Loading overlay ───────────────────────────────────────────────────────
    BusyIndicator {
        anchors.centerIn: parent
        running: app.loading
        visible: running
    }
}
