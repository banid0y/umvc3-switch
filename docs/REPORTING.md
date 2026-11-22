# Reporting lag, crashes and bugs

"It lags" or "it doesn't work" cannot be fixed without the port's log file. The port has been measured on a single
Switch OLED so far, so your log is the only way to see what happens on your console. It takes about five minutes.

## Before reporting

- Open the Homebrew Menu by **holding R while launching a game**, not from the Album.
- If you use sys-clk or another clock module, check that it is not lowering the clocks (see
  [TROUBLESHOOTING.md](TROUBLESHOOTING.md)).
- Check that you have the latest version ([UPDATE.md](UPDATE.md)).

## 1. Reproduce the problem

1. Optional, for lag: create a text file `umvc3/ovl_fps.txt` on the SD card containing just `1`. It shows the frame
   counter on screen.
2. Open the game and go to where the problem happens. For lag, stay there for **two minutes or more** (a full
   match, for example): the log records performance numbers every few seconds.
3. Close the game. **Do not open it again** before copying the log: each launch starts a new one.

## 2. Copy the log from the SD card

The file is **`umvc3/host-nx.log`**. Copy it to your PC or phone the same way you installed the game: card reader,
FTP or MTP (DBI).

If you already reopened the game, the session you want is one of the three older logs kept next to it,
`umvc3/host-nx-NN.log`: send them all.

After a crash with a system error screen, a photo of that screen helps too.

## 3. Send it

Open a report here: **https://github.com/banid0y/umvc3-switch/issues/new/choose**

- **Performance / lag report** for slowdowns and stutter.
- **Crash or bug** for everything else (closes, freezes, wrong image, controls).

Fill in the form and drag the log file into the "Log file" box (zip it if GitHub refuses the file). A free GitHub
account is needed. No account? Upload the log to any file or paste site and post the link where you reported the
problem, together with your console model, handheld or docked, and which app (BASE or CE).

One report per problem, please; if someone already reported the same thing, add your log to that report.

## What is in the log

Port messages and performance numbers (frame times, GPU and CPU load, scene resolution, SD card read times),
written in Portuguese. It has no account, serial number or other personal data, and nothing is sent anywhere by the
port itself: the file only leaves your card if you send it.
