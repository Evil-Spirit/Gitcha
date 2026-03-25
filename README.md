# Gitcha

**Gitcha** is a cross-platform, git-backed secure messenger built with Qt 6 / QML.  
It uses a standard GitLab instance (gitlab.com or self-hosted) as its only backend — no additional server software, no relay server, no database.

---

## Table of Contents

1. [How It Works](#how-it-works)
2. [Features](#features)
3. [Prerequisites](#prerequisites)
4. [Building](#building)
   - [Linux](#linux)
   - [macOS](#macos)
   - [Windows](#windows)
   - [Android](#android)
   - [iOS](#ios)
5. [Deployment (Bundling Qt Runtime Libraries)](#deployment-bundling-qt-runtime-libraries)
   - [Windows — windeployqt](#windows--windeployqt)
   - [macOS — macdeployqt](#macos--macdeployqt)
   - [Linux — linuxdeployqt](#linux--linuxdeployqt)
   - [Android & iOS](#android--ios)
6. [First-Time Setup](#first-time-setup)
7. [Using the App](#using-the-app)
   - [Signing In](#signing-in)
   - [Adding a Contact](#adding-a-contact)
   - [Sending Messages](#sending-messages)
   - [Sharing Your Repo URL](#sharing-your-repo-url)
   - [Receiving Messages](#receiving-messages)
   - [Settings & Sign Out](#settings--sign-out)
8. [Message File Format](#message-file-format)
9. [Architecture](#architecture)
10. [License](#license)

---

## How It Works

Every conversation between two users is backed by a pair of private GitLab repositories — one on each user's account. Messages are plain HTML files committed directly to these repos.

```
Alice (gitlab.com/alice)              Bob (gitlab.com/bob)
─────────────────────────             ────────────────────────
chat-with-bob/ (private repo)         chat-with-alice/ (private repo)
  messages.html  ◄── Alice commits       messages.html  ◄── Bob commits
        ▲                                       ▲
        └── Bob has Developer access ───────────┘
            Alice has Developer access on Bob's repo
```

**Message flow:**

1. Alice writes a message → it is appended to `messages.html` in her `chat-with-bob` repo and committed via the GitLab API.
2. Because Bob has Developer access to Alice's repo, Alice's client also commits the same message into Bob's `chat-with-alice` repo.
3. Bob's client polls his own repo to read new messages.

Both sides always have a local copy of the full conversation history in their own GitLab repository.

---

## Features

| Feature | Details |
|---------|---------|
| **Cross-platform** | Windows, Linux, macOS, Android, iOS |
| **Text messaging** | Real-time-style chat UI with sent/received bubbles |
| **HTML message storage** | Every conversation is a single `messages.html` file — readable in any browser, supports links, markup, and references to git-repo files |
| **GitLab as backend** | Uses only the GitLab REST API v4 (personal-access token auth) |
| **Per-pair repositories** | Each conversation gets a dedicated private repository on the local user's GitLab account |
| **Dual-repo commit** | Each message is committed to both sides' repositories |
| **No extra infrastructure** | No relay server, no database — only GitLab |
| **Offline-readable history** | Open `messages.html` in any browser to read your chat history |

---

## Prerequisites

| Dependency | Minimum version | Where to get it |
|------------|----------------|-----------------|
| **Qt** | 6.4 | [qt.io/download](https://www.qt.io/download) |
| **CMake** | 3.16 | [cmake.org](https://cmake.org/download/) or via your package manager |
| **C++ compiler** | C++17 | GCC 10+, Clang 12+, MSVC 2019+ |
| **GitLab instance** | Any (REST API v4) | [gitlab.com](https://gitlab.com) or self-hosted |
| **Git** | Any recent version | [git-scm.com](https://git-scm.com) |

---

## Building

### Linux

**Install Qt 6 via the online installer or your package manager:**

```bash
# Ubuntu / Debian (Qt 6 from apt)
sudo apt install qt6-base-dev qt6-declarative-dev qt6-tools-dev \
                 libqt6quick6 qml6-module-qtquick-controls \
                 cmake build-essential

# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel cmake gcc-c++
```

**Configure and build:**

```bash
git clone https://github.com/Evil-Spirit/Gitcha.git
cd Gitcha
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Gitcha
```

---

### macOS

**Install Qt 6 via Homebrew:**

```bash
brew install qt cmake
```

**Configure and build:**

```bash
git clone https://github.com/Evil-Spirit/Gitcha.git
cd Gitcha
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build --parallel
open build/Gitcha.app
```

---

### Windows

1. Download and run the [Qt Online Installer](https://www.qt.io/download-qt-installer).
2. Select **Qt 6.4** (or later) → **MSVC 2019 64-bit** (or MinGW).
3. Also install **CMake** and **Ninja** from the Qt Maintenance Tool, or separately.

**Open a Developer Command Prompt (MSVC) or Qt MinGW shell, then:**

```bat
git clone https://github.com/Evil-Spirit/Gitcha.git
cd Gitcha
cmake -B build -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2019_64
cmake --build build --config Release --parallel
build\Release\Gitcha.exe
```

---

### Android

> **Requirements:** Android Studio, Android NDK r25+, Qt 6 for Android (installed via Qt Maintenance Tool).

```bash
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DQT_HOST_PATH=$(qtpaths --qt-host-path) \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/android_arm64_v8a

cmake --build build-android --target apk
# APK is generated in build-android/android-build/build/outputs/apk/
```

Install on a connected device or emulator:

```bash
adb install build-android/android-build/build/outputs/apk/debug/android-build-debug.apk
```

---

### iOS

> **Requirements:** macOS with Xcode 14+, Qt 6 for iOS (installed via Qt Maintenance Tool).

```bash
cmake -B build-ios -GXcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/ios

# Open the generated Xcode project to set your signing team, then build:
cmake --open build-ios
```

Select your device or simulator in Xcode and press **Run (▶)**.

---

## Deployment (Bundling Qt Runtime Libraries)

After building, you need to copy the Qt runtime libraries (`.dll` / `.so` / `.dylib` / frameworks) alongside the executable so it can run on machines that do not have Qt installed.  
Qt ships dedicated tools for this purpose — they analyse the binary and copy only the required libraries.

### Windows — `windeployqt`

`windeployqt` is bundled with every Qt Windows installation and lives in `<Qt install>\<version>\<kit>\bin\`.

```bat
REM 1. Copy the executable to a clean deploy folder
mkdir deploy
copy build\Release\Gitcha.exe deploy\

REM 2. Run windeployqt (adjust path to match your Qt installation)
set QT_DIR=C:\Qt\6.x.x\msvc2019_64
%QT_DIR%\bin\windeployqt.exe ^
    --qmldir qml ^
    --release ^
    deploy\Gitcha.exe
```

`--qmldir qml` tells the tool to scan the project's QML directory and include all imported QML module plugins.

After the command completes, `deploy\` contains `Gitcha.exe` plus all required Qt DLLs, QML plugins, and platform plugins.  
You can zip and distribute the folder as-is, or wrap it with an installer (e.g. NSIS, Inno Setup, WiX).

---

### macOS — `macdeployqt`

`macdeployqt` is bundled with Qt for macOS and lives in `<Qt install>/<version>/macos/bin/`.

```bash
# 1. Build the app (produces build/Gitcha.app)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build --parallel

# 2. Deploy Qt frameworks into the app bundle
/path/to/Qt/6.x.x/macos/bin/macdeployqt \
    build/Gitcha.app \
    -qmldir=qml \
    -dmg          # optional: also create a distributable .dmg image
```

The result is a self-contained `Gitcha.app` bundle (and optionally `Gitcha.dmg`) that can be copied to any macOS machine without a Qt installation.

> **Tip:** If you installed Qt via Homebrew, run `$(brew --prefix qt)/bin/macdeployqt` instead of the path above.

---

### Linux — `linuxdeployqt`

Qt does not ship a first-party deploy tool for Linux; the community tool **linuxdeployqt** provides the same functionality.

**Install linuxdeployqt:**

```bash
# Download the AppImage (works on any modern x86-64 Linux)
wget -O linuxdeployqt \
  https://github.com/probonopd/linuxdeployqt/releases/latest/download/linuxdeployqt-continuous-x86_64.AppImage
chmod +x linuxdeployqt
```

**Deploy:**

```bash
# 1. Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# 2. Stage the binary under an AppDir tree
mkdir -p AppDir/usr/bin
cp build/Gitcha AppDir/usr/bin/

# 3. Run linuxdeployqt
#    -qmldir points to the QML sources so QML plugins are included
#    -appimage creates a portable single-file AppImage (optional)
./linuxdeployqt AppDir/usr/bin/Gitcha \
    -qmldir=qml \
    -appimage
```

This produces:
- `AppDir/` — a self-contained directory with the binary, Qt `.so` libraries, and QML plugins.
- `Gitcha-x86_64.AppImage` — a portable single-file executable that runs on any modern x86-64 Linux without installing Qt.

> **Alternative (system Qt):** If your target machines all run the same distribution, you can skip linuxdeployqt and instead list the Qt packages as run-time dependencies in a `.deb` or `.rpm` package.

---

### Android & iOS

No manual deployment step is required for mobile platforms.

- **Android:** `cmake --build build-android --target apk` already bundles all Qt `.so` libraries and QML plugins inside the APK.  
- **iOS:** Xcode bundles all Qt frameworks inside the `.app` when you archive for distribution (Product → Archive → Distribute App).

---

## First-Time Setup

Before using Gitcha you need a **GitLab Personal Access Token (PAT)** with the `api` scope.

### Creating a Personal Access Token

1. Open your GitLab instance in a browser.
   - For **gitlab.com**: go to <https://gitlab.com/-/profile/personal_access_tokens>
   - For **self-hosted**: go to `https://<your-gitlab>/profile/personal_access_tokens`
2. Click **Add new token**.
3. Give it a name (e.g. `Gitcha`).
4. Set an expiry date (or leave blank for no expiry).
5. Check the **`api`** scope (this is the only scope required).
6. Click **Create personal access token**.
7. **Copy the token immediately** — it will not be shown again.

> The `api` scope allows Gitcha to create repositories, manage members, and commit files on your behalf.

---

## Using the App

### Signing In

1. Launch Gitcha.
2. Enter your **GitLab Server URL** (default: `https://gitlab.com`).
3. Paste your **Personal Access Token**.
4. Optionally check **Remember me** to save credentials for future launches.
5. Tap **Sign In**.

Once authenticated, you will land on the **Chats** screen.

---

### Adding a Contact

Both sides need to add each other. Here is the step-by-step for Alice adding Bob:

#### Alice's side

1. On the **Chats** screen, tap the **+** button (bottom-right).
2. In the dialog that appears:
   - **Remote user's GitLab username** — enter `bob` (Bob's exact GitLab username).
   - **Remote user's repo URL** — leave blank for now (you'll fill it in after Bob shares his URL).
3. Tap **OK**.

Gitcha will automatically:
- Look up Bob's account on your GitLab host
- Create a **private** repository called `chat-with-bob` on Alice's account
- Grant Bob **Developer** access to that repository
- Initialise it with an empty `messages.html` file

#### Alice shares her repo URL with Bob

4. Tap on **Bob** in the chat list to open the conversation.
5. Tap the **🔗** (link) button in the top-right toolbar.
6. Copy the displayed URL (e.g. `https://gitlab.com/alice/chat-with-bob`) and send it to Bob via any other channel (email, phone, etc.).

#### Bob's side

Bob does the same steps in reverse:
1. Tap **+** on his **Chats** screen.
2. Enter `alice` as the username.
3. Paste Alice's repo URL (`https://gitlab.com/alice/chat-with-bob`) into the **Remote user's repo URL** field.
4. Tap **OK**.

Bob's Gitcha will create `chat-with-alice` on his account, grant Alice access, and the two sides are now connected.

> **Note:** Both users must be registered on the **same GitLab instance** (e.g. both on `gitlab.com`, or both on the same self-hosted server). Cross-instance messaging is not yet supported.

---

### Sending Messages

1. Tap a contact name in the **Chats** list to open the conversation.
2. Type your message in the text box at the bottom.
3. Press **Send** or hit **Enter** (Shift+Enter inserts a new line without sending).

Messages appear immediately (optimistic update) and are committed to your GitLab repository in the background. A brief status note at the bottom of the screen confirms success or shows an error.

---

### Sharing Your Repo URL

Your contact needs the URL of your `chat-with-<them>` repository to complete their setup.

1. Open the chat with your contact.
2. Tap the **🔗** button in the toolbar.
3. A popup displays your repo URL — tap **Copy** to copy it to the clipboard.
4. Share this URL with your contact out-of-band (email, SMS, etc.).

---

### Receiving Messages

Gitcha does not push notifications. To check for new messages:

1. Open the conversation with the contact.
2. Tap the **↻ (refresh)** button in the toolbar.

The client fetches the latest `messages.html` from your GitLab repository and re-renders the conversation.

> **Tip:** Your contact commits their messages into your repository (they have Developer access), so new messages are always available in your own GitLab repo without needing their server to be reachable.

---

### Settings & Sign Out

1. On the **Chats** screen, tap the **⚙** (gear) button in the top-right.
2. The **Settings** screen shows:
   - Your logged-in username
   - The GitLab server URL
3. Tap **Sign Out** to log out and clear saved credentials.

---

## Message File Format

Each conversation is stored as a single `messages.html` file that grows with each new message. It is a valid HTML5 document — open it in any web browser to read your conversation history, even without the app.

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Chat with bob</title>
  <style>
    /* ... styling ... */
  </style>
</head>
<body>

  <article data-id="2024-06-01T10:00:00Z|alice"
           data-sender="alice"
           data-ts="2024-06-01T10:00:00Z"
           class="message">
    <header><b>alice</b> <time datetime="2024-06-01T10:00:00Z">2024-06-01 10:00</time></header>
    <p>Hello Bob! 👋</p>
  </article>

  <article data-id="2024-06-01T10:01:00Z|bob"
           data-sender="bob"
           data-ts="2024-06-01T10:01:00Z"
           class="message">
    <header><b>bob</b> <time datetime="2024-06-01T10:01:00Z">2024-06-01 10:01</time></header>
    <p>Hey Alice! Great to hear from you.</p>
  </article>

</body>
</html>
```

**Each message `<article>` element has:**

| Attribute | Description |
|-----------|-------------|
| `data-id` | Unique identifier: `<ISO-timestamp>\|<username>` |
| `data-sender` | GitLab username of the author |
| `data-ts` | ISO 8601 UTC timestamp |
| `class="message"` | Used for browser CSS styling |

Messages can contain plain text, HTML hyperlinks (to internet resources or git-repo file URLs), and any other HTML markup.

**To read a conversation in your browser:**

```bash
# Clone your chat repo (you already have access via your PAT)
git clone https://gitlab.com/alice/chat-with-bob
cd chat-with-bob
open messages.html   # macOS
xdg-open messages.html  # Linux
start messages.html  # Windows
```

---

## Architecture

```
Gitcha/
├── CMakeLists.txt              Qt6 CMake project (Desktop + Android + iOS)
├── android/
│   └── AndroidManifest.xml     Android package manifest
├── assets/
│   └── logo.png                Application icon
├── qml/
│   ├── main.qml                ApplicationWindow + StackView navigation
│   ├── LoginPage.qml           Sign-in screen (server URL + PAT)
│   ├── ContactsPage.qml        Contacts / chat list + FAB
│   ├── ChatPage.qml            Message bubbles, input bar, refresh, share
│   ├── AddContactDialog.qml    Add-contact dialog
│   └── SettingsPage.qml        Account info + sign-out
└── src/
    ├── main.cpp                Entry point + AppController (QML↔C++ bridge)
    ├── gitlabclient.h/cpp      GitLab REST API v4 wrapper (async callbacks)
    ├── messagestore.h/cpp      HTML message serialisation / deserialisation
    ├── contactmanager.h/cpp    Contact list persistence + add-contact flow
    ├── messagemodel.h/cpp      QAbstractListModel for messages → QML ListView
    ├── contactmodel.h/cpp      QAbstractListModel for contacts → QML ListView
    └── settingsmanager.h/cpp   QSettings wrapper exposed to QML
```

### Data flow

```
User types message
       │
       ▼
AppController::sendMessage()
       │  fetch current messages.html via GitLab API
       │  append new <article> block (MessageStore)
       │  commit updated file via GitLab API
       ▼
GitLabClient::commitFile()   ──►  GitLab repository (your account)
                                        │
                                        │  (contact has Developer access)
                                        ▼
                                Contact's GitLab repo
                                contact's Gitcha polls and reads
```

---

## Troubleshooting

| Problem | Solution |
|---------|---------|
| "Login failed" | Check that your PAT has the `api` scope and has not expired |
| "User not found" | Confirm the exact GitLab username (case-sensitive on some instances) |
| "Failed to add contact" | Make sure the remote user exists on **your** GitLab instance |
| Messages not appearing | Tap **↻** to refresh; check that the contact has committed to your repo |
| "Send failed" | Check your internet connection; verify the PAT is still valid |

---

## License

MIT
