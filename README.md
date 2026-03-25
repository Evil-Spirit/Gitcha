# Gitcha

**Gitcha** is a cross-platform, git-backed secure messenger built with Qt 6 / QML.  
It uses a standard GitLab instance (gitlab.com or self-hosted) as its only backend — no additional server software is required.

---

## Features

| Feature | Details |
|---------|---------|
| **Cross-platform** | Windows, Linux, macOS, Android, iOS |
| **Text messaging** | Real-time-style chat UI with sent/received bubbles |
| **HTML message storage** | Every conversation is stored as a single `messages.html` file — readable in any browser, supports links, markup, and references to git-repo files |
| **GitLab as backend** | Uses only the GitLab REST API (personal-access token auth) |
| **Per-pair repositories** | Each conversation creates a dedicated private repository on the local user's GitLab account |
| **Dual commit** | When a message is sent it is committed to the sender's repository; the receiver reads from their own copy |
| **No extra infrastructure** | No relay server, no database — only GitLab |

---

## How It Works

```
Alice (gitlab.com/alice)              Bob (gitlab.com/bob)
─────────────────────────             ────────────────────────
chat-with-bob/ (private repo)         chat-with-alice/ (private repo)
  messages.html  ◄── Alice commits       messages.html  ◄── Bob commits
                                                 ▲
                 Bob has Developer access ────────┘
                 Alice has Developer access on Bob's repo
```

1. Alice creates `chat-with-bob` on her GitLab and grants Bob developer access.  
2. Bob creates `chat-with-alice` on his GitLab and grants Alice developer access.  
3. Each user shares their repo URL with the other (via the 🔗 button in the chat screen).  
4. When Alice sends a message, it is appended to `messages.html` and committed to her repo.  
5. Bob's client polls his own repo to pick up Alice's messages (she committed to his repo).

---

## Prerequisites

| Dependency | Minimum version |
|------------|----------------|
| Qt | 6.4 |
| CMake | 3.16 |
| C++ compiler | C++17 (GCC 10+, Clang 12+, MSVC 2019+) |
| GitLab instance | Any version with REST API v4 |

---

## Building

### Desktop (Linux / macOS / Windows)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Gitcha          # Linux/macOS
build\Gitcha.exe        # Windows
```

### Android

```bash
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-28 \
  -DQT_HOST_PATH=$(qtpaths --qt-host-path) \
  -DCMAKE_PREFIX_PATH=/path/to/Qt6/android_arm64_v8a
cmake --build build-android --target apk
```

### iOS

Open the generated Xcode project after configuring with the iOS toolchain:

```bash
cmake -B build-ios -GXcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_PREFIX_PATH=/path/to/Qt6/ios
cmake --open build-ios
```

---

## First-Time Setup

1. **Create a GitLab Personal Access Token** with the `api` scope.  
   Go to: `https://gitlab.com/-/profile/personal_access_tokens` (or your self-hosted equivalent).

2. **Launch Gitcha** and sign in with your server URL and token.

3. **Add a contact** — tap **+** on the contacts screen.  
   Enter the remote user's GitLab username.  
   Gitcha will automatically:
   - Look up the user on your GitLab host
   - Create a private `chat-with-<username>` repository
   - Grant the contact Developer access
   - Initialise the `messages.html` file

4. **Share your repo URL** — tap 🔗 in the chat screen and send the displayed URL to your contact so they can add you on their side.

---

## Message File Format

Each conversation is stored in a single `messages.html` file that grows with each commit.  
The file is a valid HTML5 document so it can be opened in any browser for offline reading.

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Chat with bob</title>
  ...
</head>
<body>
  <article data-id="2024-06-01T10:00:00Z|alice"
           data-sender="alice"
           data-ts="2024-06-01T10:00:00Z"
           class="message">
    <header><b>alice</b> <time datetime="2024-06-01T10:00:00Z">2024-06-01 10:00</time></header>
    <p>Hello Bob! 👋</p>
  </article>
  ...
</body>
</html>
```

Messages can contain:
- Plain text
- HTML hyperlinks to internet resources
- References to files in linked git repositories

---

## Architecture

```
src/
  main.cpp            Application entry point + AppController (QML bridge)
  gitlabclient.h/cpp  GitLab REST API wrapper (async, callback-based)
  messagestore.h/cpp  HTML message serialisation / deserialisation
  contactmanager.h/cpp Contact list persistence + add-contact flow
  messagemodel.h/cpp  QAbstractListModel for messages (→ QML ListView)
  contactmodel.h/cpp  QAbstractListModel for contacts (→ QML ListView)
  settingsmanager.h/cpp QSettings wrapper exposed to QML

qml/
  main.qml            ApplicationWindow + StackView navigation
  LoginPage.qml       Sign-in screen
  ContactsPage.qml    Contact / chat list
  ChatPage.qml        Message bubbles + input bar
  AddContactDialog.qml Add-contact dialog
  SettingsPage.qml    Account info + sign-out
```

---

## License

MIT
