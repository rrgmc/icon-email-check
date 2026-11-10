# icon-email-check

Icon Email Check is a tray app for macOS, Windows and Linux. It counts the unread email in one or more accounts and shows the count on its icon.

- **Gmail** through the Gmail API, after "Sign in with Google".
- **Any IMAP server**, such as iCloud, Yahoo, Fastmail, or Gmail with an app password.
- **The icon changes with the count.** On macOS the menu bar shows an envelope, a filled envelope with the count beside it when mail is unread, and a faded envelope when an account cannot be checked. On Windows and Linux the count is a red badge on the icon, and the icon turns grey when an account cannot be checked.
- **Desktop notifications for new email**, with the sender and subject. Clicking one opens the account's inbox.
- **The tray menu** lists each account with its count and its newest unread messages. Clicking an account opens its inbox in the browser.
- **Starts at sign-in** if you choose, and **starts in the tray** without a window.
- **Passwords and sign-in tokens stay in the system keychain**: the macOS Keychain, the Windows Credential Manager, or the Secret Service on Linux.
- **Nothing is marked read.** IMAP opens the folder read-only and reads headers with `BODY.PEEK`; Gmail uses a read-only scope.

## Using it

The first start shows the Accounts and Settings window. Choose **Add…** to add an account. Later starts go straight to the tray. To open the window again, click the tray icon, choose **Accounts and Settings…** from its menu, or start the app again.

On macOS, a left click on the menu bar icon shows or hides the window, and a right click or Control-click opens the menu. On Windows and Linux, a left click shows or hides the window, and a right click opens the menu.

Closing the window keeps the app running in the tray. **Quit** is in the tray menu, and in the File menu while the window is open.

Each account is checked at its own interval, 5 minutes by default. When the server refuses the password or sign-in, the account pauses until you edit it or choose **Check Now**. This stops the app from sending a wrong password repeatedly, which can get an account locked.

## Gmail

Gmail accounts can be added in two ways.

### Sign in with Google

Google needs an OAuth client to sign in. Make one in your own Google Cloud project once:

1. In the [Google Cloud console](https://console.cloud.google.com/), create a project.
2. Enable the **Gmail API** under **APIs & Services > Library**.
3. Under **Google Auth Platform > Audience** (the OAuth consent screen), choose **External**.
4. While the app is in **Testing**, add your Gmail addresses under **Test users**. Google ends a test app's sign-ins after 7 days. To avoid this, choose **Publish app**. A personal app does not need Google's review; the sign-in page then shows a warning that Google has not verified the app.
5. Under **Clients**, create a client of type **Desktop app**. Copy its client ID and client secret.
6. In Icon Email Check, paste both into **Google sign-in** in the settings window.

Then add an account of type **Gmail**, and choose **Sign in with Google…**. The browser opens Google's sign-in page and returns to the app when you are done.

The app asks for the `gmail.readonly` scope. Without a search, the count is the number of unread messages in the inbox. Gmail's web page may count conversations instead, so the two can differ. A search such as `is:unread category:primary` counts only what it finds.

A desktop app's client secret is not a secret: Google expects it to ship inside the app. A build can carry a default client with `-DIEC_GOOGLE_CLIENT_ID=… -DIEC_GOOGLE_CLIENT_SECRET=…`, or the environment variables of the same names, at configure time. A client set in the settings window replaces it.

### IMAP with an app password

This needs no Google Cloud project. Turn on 2-Step Verification for the Google account, create an **app password** under **Google Account > Security > App passwords**, and add an **IMAP** account with the provider **Gmail (app password)**.

## Building

You need CMake 3.21 or newer, Ninja, and Qt 6.8 or newer with Qt Network and Qt Svg. The first configure downloads [qtkeychain](https://github.com/frankosterfeld/qtkeychain) 0.15.0, so it needs the network.

```sh
cmake --preset dev -DCMAKE_PREFIX_PATH=<path to Qt, such as ~/Qt/6.8.3/macos>
cmake --build --preset dev
ctest --preset dev
```

The presets are `dev` (Debug), `release`, and `ci` (Release with warnings as errors). On Windows, `tools/build.ps1` does all of it: it downloads Qt if needed, enters the Visual Studio 2022 developer shell, then builds and tests the `dev` preset. `tools/setup-qt.sh` and `tools/setup-qt.ps1` download the Qt that CI uses; they need [uv](https://docs.astral.sh/uv/).

Each build makes a copy of the app with the Qt files it needs in `build/<preset>/bin`, so it starts without Qt on `PATH`. On macOS that folder holds the app bundle.

To make the package of a build, install it and run CPack in its build folder. The install folder must be an absolute path. On Windows, `tools/build.ps1 -Preset release -Package` does all of it, and needs [NSIS](https://nsis.sourceforge.io/).

```sh
cmake --install build/release --component App --prefix "$PWD/build/release/install"
python3 tools/check-package.py build/release/install
cd build/release && cpack
```

The packages are:

- **Windows:** `icon-email-check-<version>-win64.exe`, a setup. Its last page offers to run the app and to start it at sign-in.
- **macOS:** `icon-email-check-<version>-macos.dmg`, a disk image with a universal app for Apple silicon and Intel. Drag the app to Applications.
- **Linux:** `icon-email-check_<version>_amd64.deb`. The app and its own Qt go in `/opt/icon-email-check`, with `icon-email-check` in `/usr/bin` and an entry in the applications menu. GNOME shows tray icons only with the AppIndicator extension.

## Continuous integration and releases

`.github/workflows/ci.yml` builds, tests and packages every push to `master` and every pull request on Windows, macOS and Linux. A change that touches only Markdown files builds nothing. Each run keeps its packages as artifacts for a week. They are not signed with a Developer ID, so on macOS the app opens only after **Open Anyway** in **System Settings > Privacy & Security**.

Pushing a tag `vX.Y.Z` publishes a release with the three packages, and `vX.Y.Z-rc.N` a prerelease. The tag gives the version, so it need not match `IEC_VERSION` in `CMakeLists.txt`. The release notes are the version's section of [CHANGELOG.md](CHANGELOG.md), or GitHub's list of changes when it has none.

```sh
git tag v0.1.0
git push origin v0.1.0
```

To make the release again, run the Release workflow by hand from its tag, under **Use workflow from**.

The build jobs hold no secrets. On macOS a separate job signs the finished bundle and makes the disk image. For a release, it signs with a Developer ID and notarises the disk image when the GitHub environment `release` has these secrets; without them it signs ad hoc and the run shows a warning:

| Secret | What it holds |
| --- | --- |
| `MACOS_CERTIFICATE_P12` | The "Developer ID Application" certificate with its private key, exported from Keychain Access as a .p12 file, in base64: `base64 -i certificate.p12` |
| `MACOS_CERTIFICATE_PASSWORD` | The password of that .p12 file |
| `NOTARY_API_KEY` | An App Store Connect API key (the contents of its .p8 file), from **Users and Access > Integrations** |
| `NOTARY_API_KEY_ID` | That key's ID |
| `NOTARY_API_ISSUER` | The issuer ID shown above the keys |

Create the environment under **Settings > Environments**, and limit its deployment to tags `v*`, so only a release can read the secrets. The Windows setup is not signed, so Windows SmartScreen warns about it the first time.

On macOS, the build signs the bundle with a code signing certificate when one is available, so the keychain knows each build as the same app and does not ask again for access after each build. Set `IEC_MACOS_SIGN_IDENTITY` to the name of one of your certificates (`security find-identity -v -p codesigning` lists them). Without one, the app is signed ad hoc.

`tools/make-icons.sh` makes `resources/app/app.icns` and `resources/app/app.ico` from `resources/icons/app.svg`. Run it after changing the SVG.

## Platform notes

The tray, the Dock and starting at sign-in follow the approach of the Mirante project:

- **macOS tray.** The menu bar icon is a native `NSStatusItem`, because Qt 6.8's `QSystemTrayIcon` aborts on macOS 27 when it is clicked. Its pictures are template images, which macOS draws in the menu bar's colour.
- **macOS Dock.** The bundle sets `LSUIElement`, so the app starts without a Dock icon. The Dock icon appears while the window is shown and goes away when the window is closed. The Dock shows the bundle's icon file; the app sets no icon at run time, which would replace it with a larger one.
- **macOS reopen.** Opening the app again from the Finder shows the window, through the reopen Apple event.
- **One instance.** A second start asks the running instance to show its window, then exits. On Windows it first lets the running instance take the foreground.
- **Starting at sign-in.** Windows uses the per-user `Run` key, macOS 13 and later `SMAppService`, and Linux an XDG autostart file. The system holds the choice, so a change in the system's own settings shows in the app. At sign-in the app starts in the tray. On macOS it may need your approval under **System Settings > General > Login Items**; the settings window links there.
- **No tray yet.** At sign-in the taskbar may start after the app. The app tries for a minute to make its tray icon, then shows its window.
- **Notifications.** macOS uses the User Notifications framework, and asks for permission the first time. Windows and Linux use the tray icon's messages.

## License

MIT. See [LICENSE](LICENSE).
