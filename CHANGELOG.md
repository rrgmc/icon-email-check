# Changelog

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). A release's notes are its section here.

## [Unreleased]

### Added

- Packages built by CI carry a Google OAuth client, from the repository's secrets, so "Sign in with Google" works without setting one up. The CMake option `IEC_GOOGLE_CLIENT_SETTINGS` shows or hides the settings window's Google sign-in section; it is hidden when a client is built in.
- A privacy policy, published with GitHub Pages from `docs/`, and linked from the About window.

### Changed

- The tray icon is in colour only while mail is unread: a blue envelope with a red dot on macOS, and with the count in a red badge on Windows and Linux. With nothing unread it is plain. When an account cannot be checked, the Windows and Linux icon shows an orange warning mark.

### Fixed

- "Sign in with Google" failed with "client_secret is missing" when the client secret was typed in the settings and **Add…** clicked right after. The settings now save as they are typed, and the sign-in reads them when it starts. Without a client secret, the Add Account window now says so instead of starting a sign-in that Google refuses.
- In the Add Account window on macOS, text fields were too narrow for their content, "Signed in as" was cut off, and a Gmail account left an empty gap where the IMAP fields would be.
- On macOS, clicking the menu bar icon opened the window on the desktop it was last on, and switched to that desktop. It now opens on the current desktop.

## [0.1.0]

### Added

- A tray icon for macOS, Windows and Linux that counts unread email, and changes with the count.
- Gmail accounts through "Sign in with Google", and any IMAP server.
- Desktop notifications for new email.
- A window to manage the accounts and the settings.
- Starting at sign-in, and starting in the tray.
- Packages: a Windows setup, a macOS disk image and a Linux .deb package.
