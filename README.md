# Icon Email Check

Icon Email Check is a tray app for macOS, Windows and Linux. It counts the unread email in one or more accounts and shows the count on its icon.

## Features

- **Gmail**, after "Sign in with Google".
- **Any IMAP server**, such as iCloud, Yahoo, Fastmail, or Gmail with an app password.
- **The icon changes with the count.** On macOS the menu bar shows a plain envelope in the menu bar's colour while nothing is unread, and a blue envelope, with the count beside it, while mail is unread. On Windows and Linux the icon is a grey outline while nothing is unread, and a blue envelope with the count in a red badge while mail is unread. When an account cannot be checked, the icon shows a warning mark.
- **Each account has a colour.** While mail is unread, a dot in the colour of each account that has unread mail shows along the top of the icon, up to three, in the order of the account list. A new account gets an unused colour; you can choose another in the account's settings.
- **Desktop notifications for new email**, with the sender and subject. Clicking one opens the account's inbox.
- **The tray menu** lists each account with its count and its newest unread messages. Clicking an account opens its inbox in the browser.
- **Starts at sign-in** if you choose, and **starts in the tray** without a window.
- **Passwords and sign-in tokens stay in the system keychain**: the macOS Keychain, the Windows Credential Manager, or the Secret Service on Linux.
- **Nothing is marked read.** The app only reads; it never changes your mail.

## Installing

Download the package for your system from the [latest release](https://github.com/rrgmc/icon-email-check/releases/latest).

- **Windows:** run `icon-email-check-<version>-win64.exe`. Its last page offers to run the app and to start it at sign-in. The setup is not signed, so Windows SmartScreen may warn about it the first time. Choose **More info**, then **Run anyway**.
- **macOS:** open `icon-email-check-<version>-macos.dmg` and drag the app to Applications. It runs on Apple silicon and Intel.
- **Linux:** install `icon-email-check_<version>_amd64.deb`, for example with `sudo apt install ./icon-email-check_<version>_amd64.deb`. The app appears in the applications menu. GNOME shows tray icons only with the AppIndicator extension.

## Using it

The first start shows the Accounts and Settings window. Choose **Add…** to add an account. Later starts go straight to the tray. To open the window again, choose **Accounts and Settings…** from the tray icon's menu, or start the app again.

A left or a right click on the tray icon opens its menu.

Closing the window keeps the app running in the tray. **Quit** is in the tray menu, and in the File menu while the window is open.

Each account is checked at its own interval, 5 minutes by default. When the server refuses the password or sign-in, the account pauses until you edit it or choose **Check Now**. This stops the app from sending a wrong password repeatedly, which can get an account locked.

At sign-in the app starts in the tray. On macOS, starting at sign-in may need your approval under **System Settings > General > Login Items**; the settings window links there. On macOS the app also asks for permission to show notifications the first time.

## Gmail

You can add a Gmail account in two ways.

### Sign in with Google

Choose **Add…**, keep the type **Gmail: sign in with Google**, choose **Sign in with Google…**, and sign in to your Google account in the browser. The app asks only to read your mail, and cannot change or send anything.

Google has not verified the app, so the sign-in page shows a warning. Choose **Advanced**, then **Go to Icon Email Check (unsafe)**.

The count is the number of unread messages in the inbox. Gmail's web page may count conversations instead, so the two can differ. To count only some messages, enter a Gmail search such as `is:unread category:primary` in the account's **Count** field.

### IMAP with an app password

This needs no Google sign-in in the app. Turn on 2-Step Verification for the Google account, create an **app password** under **Google Account > Security > App passwords**, and add an account of type **IMAP: any mail server** with the provider **Gmail (app password)**. Use the app password as the password.

## Privacy

The app reads only what it needs to count unread email, keeps passwords and sign-in tokens in your computer's keychain, and sends nothing to anyone but Google or your mail server. The [privacy policy](docs/privacy.md) says exactly what it reads.

## Building from source

[DEVELOPMENT.md](DEVELOPMENT.md) describes how to build, test, package and release the app.

## License

MIT. See [LICENSE](LICENSE).

## Author

Rangel Reale <realerangel@gmail.com>
