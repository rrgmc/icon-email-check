---
title: Privacy policy
---

# Icon Email Check privacy policy

Last updated: 10 November 2026

Icon Email Check is a desktop app for macOS, Windows and Linux. It shows how many unread email messages your accounts have. This policy says what the app reads, where it keeps it, and who receives it.

## What the app reads

- **Gmail accounts, through "Sign in with Google".** The app asks Google for read-only access to Gmail (the `gmail.readonly` scope). With it, the app reads:
  - your Gmail address, to show which account is signed in;
  - the number of unread messages in your inbox, or in a search you set;
  - the sender and subject of the newest unread messages, at most five per account, to show them in the app's menu and in notifications.

  The app does not read message bodies or attachments. It does not send, delete, label or change any message, and it does not mark messages as read.
- **IMAP accounts.** The app logs in to the server you set with the user name and password you enter. It opens the folder read-only and reads the same information as for Gmail: the number of unread messages, and the sender and subject of the newest ones.

## Where the information is kept

- Passwords and Google sign-in tokens are kept in your computer's own keychain: the macOS Keychain, the Windows Credential Manager, or the Secret Service on Linux.
- The account settings, such as names, servers and check intervals, are kept in your computer's settings store.
- Unread counts, senders and subjects are kept in memory only, while the app runs. The app does not save them to disk.
- New email notifications, with their senders and subjects, are shown by your system's notification center, which may keep them in its own history.

## Who receives the information

The app connects only to:

- Google, for Gmail accounts: `accounts.google.com` and `oauth2.googleapis.com` to sign in, and `gmail.googleapis.com` to read the counts and headers;
- the IMAP servers you set, for IMAP accounts.

The app has no servers of its own. It sends no information to the developer or to anyone else. It has no analytics, no advertising, and no crash reporting. No information is sold or shared.

## Google API Services User Data Policy

Icon Email Check's use and transfer of information received from Google APIs to any other app will adhere to the [Google API Services User Data Policy](https://developers.google.com/terms/api-services-user-data-policy), including the Limited Use requirements.

## Removing your information

- Removing an account in the app removes its password or sign-in token from your keychain.
- To withdraw the app's access to your Google account, open [your Google Account's third-party connections](https://myaccount.google.com/connections) and remove Icon Email Check.
- Uninstalling the app leaves its settings in your computer's settings store. Remove the accounts first to remove their keychain entries.

## Changes to this policy

A change to this policy is published on this page, with a new date at the top.

## Contact

Questions about this policy go to the project's [issue tracker](https://github.com/rrgmc/icon-email-check/issues).
