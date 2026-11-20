# UMvC3 on Switch

Unofficial port of **Ultimate Marvel vs. Capcom 3** (PC / Steam version) to Nintendo Switch running Atmosphère.

**You need your own copy of the game on Steam.** The game data is not included here: the installer builds
everything the Switch needs from your own installation.

Version 1.0 (test release). Two apps, which can live side by side on the SD card:

| Homebrew Menu name | File | What it is |
|---|---|---|
| (BASE) ULTIMATE MARVEL VS CAPCOM 3 | `umvc3.nro` | The original game |
| (CE) ULTIMATE MARVEL VS CAPCOM 3 | `umvc3-ce.nro` | The game with the Community Edition **1.2.3b** (also needs your own copy of that CE build) |

## What you need

- A Switch with **Atmosphère** and the **Homebrew Menu**.
- Ultimate Marvel vs. Capcom 3 installed from **Steam** on a PC with Windows 10 or 11.
- Free space on the SD card: about **4 GB** for the original game, **13 GB** with the Community Edition.
- For the CE version: your "UMvC3 Community Edition Build" folder, version **1.2.3b** (the only one supported).

## Install

1. Download this repository (**Code → Download ZIP**) and extract it.
2. Open **`Install.exe`** (keep the `NRO FILES` folder next to it). It opens in English; the box in the top
   right corner switches it to Portuguese.
3. Check the game folder, tick the Community Edition if you want it, pick the destination (the SD card or a
   folder on the PC) and click **Install**.
4. If you did not install straight to the card, copy the `switch` and `umvc3` folders to the root of the SD card.
5. On the Switch, **hold R while opening any game** to enter the Homebrew Menu, then pick the app.

Full guide: [docs/INSTALL.md](docs/INSTALL.md) · Controls: [docs/CONTROLS.md](docs/CONTROLS.md) ·
Options: [docs/OPTIONS.md](docs/OPTIONS.md) · Problems: [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)

Em português: [LEIAME.md](LEIAME.md).

## Good to know

- `Install.exe` is not digitally signed, so Windows shows an "unknown publisher" warning: choose
  "More info" → "Run anyway".
- The Steam `umvc3.exe` is wrapped in a protection layer the port cannot read. The installer asks before
  downloading [Steamless](https://github.com/atom0s/Steamless) (by atom0s) from its official page and applying it
  to a **copy** of your executable. Your game folder is never changed.
- Nothing here touches firmware, NAND or SD partitions: only files under `switch/` and `umvc3/` on the SD card.
- Open the Homebrew Menu through a game (hold R), not through the Album: from the Album there is not enough memory.

## Source

`source/` holds the complete source code of the installer, a build script and a description of every step it performs
(see [source/README.md](source/README.md)). The installer embeds the deko3d shader compiler `uam` from devkitPro
(license in `docs/LICENCA-uam.txt`).

## License

See [LICENSE.md](LICENSE.md): you may download, run, study and privately modify the port's own code and texts;
publishing changes or reusing them elsewhere needs the author's permission first (open an issue to ask).
Release notes: [CHANGELOG.md](CHANGELOG.md).

## Credits

See [CREDITS.md](CREDITS.md). Ultimate Marvel vs. Capcom 3 belongs to Capcom and Marvel. This project is not
affiliated with either of them and does not replace buying the game.
