# Updating to a new version

Updating only replaces the app files. Your saves, settings, game data and shaders stay on the card.

## 1. Get the new files

Download this repository again (**Code → Download ZIP**) and extract it, or download just the files from the
[`NRO FILES`](../NRO%20FILES) folder (open the file on GitHub and use the download button):

- `umvc3.nro`: the original game.
- `umvc3-ce.nro`: the game with the Community Edition.

You only need the one you play. [CHANGELOG.md](../CHANGELOG.md) lists what changed in each version.

## 2. Replace them on the SD card

1. Close the game on the Switch.
2. Copy the new `.nro` over the old one in the **`switch`** folder at the root of the SD card, keeping the same
   file name. Use the same way you used to install: card reader, FTP or MTP (DBI).
3. Open the game as usual (hold R on any game → Homebrew Menu). The version number shown next to the app name in
   the Homebrew Menu tells you the copy worked.

Do not delete the `umvc3` folder: it holds the game data, the shaders and your saves.

## When the installer must run again

Only when the release notes of that version say so (for example, new shaders). In that case open the new
`Install.exe` and install to the same destination as before. Saves and game settings already on the card are kept.

If you want to be safe, copy `umvc3/userdata` (original game) and `umvc3/userdata-ce` (Community Edition) to the
PC first: those two folders are your saves and settings.

## Notes

- **Forwarder on the home menu:** it points to the `.nro` path, so it keeps working after the file is replaced.
- **Option files** you created in `umvc3/` (see [OPTIONS.md](OPTIONS.md)) stay in effect after an update.
- **Going back:** keep a copy of the old `.nro`; putting it back in `switch/` restores the previous version.
- **A newer Community Edition build** is not an update of the port: only the CE version named in the
  [README](../README.md) is supported.
- Something got worse after updating? Open a report with your log file:
  [new issue](https://github.com/banid0y/umvc3-switch/issues/new/choose).
