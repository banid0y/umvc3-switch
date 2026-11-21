# Common problems

| Symptom | Likely cause | What to do |
|---|---|---|
| Closes right after opening, or black screen | Opened from the Album (not enough memory) | Open the Homebrew Menu by holding R while launching a game |
| "shader pré-compilado ausente" in the log, missing graphics | `umvc3/dksh.pak` missing or incomplete | Run the installer again |
| The installer warns that `umvc3.exe` is another version | Game version differs from the supported one | Update the game on Steam and run the installer again |
| The CE version opens the original game | `umvc3/ce/` or `umvc3/ce-data/Characters.ini` is missing, or your CE is not 1.2.3b | Run the installer again and answer "Yes" to the Community Edition |
| VERSUS does not leave the controller screen | Fewer than two controllers paired | Pair two controllers (or two sideways Joy-Con) on the system screen |
| Frame drops in heavy fights | Handheld GPU limit | The port lowers the scene resolution by itself; see `docs/OPTIONS.md` |
| Choppy everywhere, even in menus or training | A clock module (sys-clk or similar) holding the CPU/GPU below the port's profile, or a slow SD card | See "Performance, sys-clk and overclock" below |

## Performance, sys-clk and overclock

- **No overclock is needed or used.** The port was developed and measured at stock clocks. In handheld mode it asks
  the system for its official maximum handheld performance profile (the same kind of profile some retail games
  use); this is not an overclock.
- **Clock modules can override that.** If you run sys-clk (or any module that sets CPU/GPU/memory clocks), check
  that it has no profile lowering the clocks for the Homebrew Menu or for the game you launch it through. A lower
  profile there makes everything choppy. To rule it out, disable the module and try again.
- **Overclocking is untested.** Raising the clocks above stock may help or may cause instability; that is at your
  own risk and the port gives no guarantee there.
- **Tested hardware.** So far the port has been measured on a single Switch OLED. Reports from other models
  (V1, V2, Lite) are welcome: turn on the FPS counter (`ovl_fps.txt`, see `docs/OPTIONS.md`) and send the log as described below.
- `umvc3/apm.txt` containing `0` stops the port from requesting the performance profile (see `docs/OPTIONS.md`).

## Sending a report

Attach the `umvc3/host-nx.log` file from the card (it is rewritten on every launch; the three previous ones stay
as `host-nx-NN.log`). Say which of the two versions you opened, which screen you were on and what you pressed.
The log itself is written in Portuguese.
