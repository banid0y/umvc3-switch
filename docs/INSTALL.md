# Step-by-step install

Total time: 10 to 30 minutes, most of it copying files to the SD card.

## Before you start

- [ ] A Switch running Atmosphère with the Homebrew Menu.
- [ ] Ultimate Marvel vs. Capcom 3 installed from Steam on a PC with Windows 10 or 11.
- [ ] Free space on the SD card: 4 GB (original game) or 13 GB (with the Community Edition).
- [ ] A way to put files on the card: a card reader (fastest), or FTP/MTP from the Switch itself.
- [ ] Internet on the PC the first time (the installer downloads a small tool, Steamless).
- [ ] For the CE version only: your "UMvC3 Community Edition Build" folder, **version 1.2.3b** (the only one supported).

Nothing here touches firmware, NAND or SD partitions: only files under `switch/` and `umvc3/` on the SD card.
The game folder on the PC is not changed either.

## 1. Open the installer

Extract the package anywhere and open **`Install.exe`**. Keep the `NRO FILES` folder next to it.

The window opens in English; the box in the top right corner switches it to Portuguese.

If Windows warns about an unknown program, choose "More info" and "Run anyway".

## 2. Fill in the window and install

1. **Game folder:** the installer fills in the default Steam folder. If your game lives elsewhere, click
   "Browse..." and pick the "ULTIMATE MARVEL VS. CAPCOM 3" folder.
2. **Community Edition (optional):** tick the box and pick your "UMvC3 Community Edition Build" folder. Leave it
   unticked to install the original game only. If your CE is not 1.2.3b, the installer says so and installs the
   original game only.
3. **Destination:** pick the SD card (if it is in a reader) or any folder on the PC.
4. Click **Install**.
5. **Steamless:** the Steam `umvc3.exe` is wrapped in a protection layer the port cannot read. The first time,
   the installer asks whether it may download Steamless (a free program by atom0s) from its official page and
   apply it to a copy of your executable. Answer "Yes".

After that it works by itself, showing the current step and a progress bar: it checks that your game is the
supported version, compiles the shaders for the Switch GPU (about a minute) and verifies each one, prepares the
CE plugins and copies the game data (3.6 GB, plus 8.3 GB with the CE).

Result at the destination:

```
switch/umvc3.nro                  (BASE) ULTIMATE MARVEL VS CAPCOM 3: original game
switch/umvc3-ce.nro               (CE) ULTIMATE MARVEL VS CAPCOM 3: only if you installed the CE
umvc3/umvc3.exe.unpacked.exe
umvc3/dksh.pak                    compiled shaders
umvc3/nativePCx64/                game data
umvc3/texturas/                   Switch button icons
umvc3/ce/  and  umvc3/ce-data/    Community Edition only
```

## 3. Move it to the card (if you did not install straight to it)

Copy the `switch` and `umvc3` folders to the **root** of the SD card, merging with what is already there.

- **Card reader:** power the Switch off, take the card out, copy, put it back.
- **FTP:** start an FTP server on the Switch (ftpd, for example) and upload with any FTP client.
- **MTP (DBI):** plug in the USB cable, choose "MTP responder" in DBI and copy with File Explorer.

## 4. Launch the game

1. On the Switch home menu, **hold R** while opening **any game** (not the Album). This opens the Homebrew Menu
   with the console's full memory; from the Album the game does not fit.
2. Pick **(BASE) ULTIMATE MARVEL VS CAPCOM 3** (original game) or **(CE) ULTIMATE MARVEL VS CAPCOM 3** (with the
   Community Edition).
3. The first launch takes about a minute while the shaders load.

## Updating later

For a new version of the port, just replace the `.nro` files under `switch/`. Game data, shaders and saves stay.
Release notes say when shaders must be rebuilt: run the installer again.
Step by step: [UPDATE.md](UPDATE.md).

## Uninstalling

Delete `switch/umvc3.nro`, `switch/umvc3-ce.nro` and the `umvc3/` folder from the card. Saves live in
`umvc3/userdata/` and `umvc3/userdata-ce/`; keep those folders if you want them.

## If you already have Steamless or have no internet

Extract Steamless 3.1.0.5 into a folder named `steamless` next to `Install.exe` (with `Steamless.CLI.exe`
inside). The installer uses that copy and downloads nothing. If the game folder already holds a
`umvc3.exe.unpacked.exe`, the installer uses that file directly.
