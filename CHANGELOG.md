# Changelog

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). A release's notes are its section here.

## [Unreleased]

### Fixed

- "Sign in with Google" failed with "client_secret is missing" when the client secret was typed in the settings and **Add…** clicked right after. The settings now save as they are typed, and the sign-in reads them when it starts. Without a client secret, the Add Account window now says so instead of starting a sign-in that Google refuses.

## [0.1.0]

### Added

- A tray icon for macOS, Windows and Linux that counts unread email, and changes with the count.
- Gmail accounts through "Sign in with Google", and any IMAP server.
- Desktop notifications for new email.
- A window to manage the accounts and the settings.
- Starting at sign-in, and starting in the tray.
- Packages: a Windows setup, a macOS disk image and a Linux .deb package.
