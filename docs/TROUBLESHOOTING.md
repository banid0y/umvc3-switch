# Common problems

| Symptom | Likely cause | What to do |
|---|---|---|
| Closes right after opening, or black screen | Opened from the Album (not enough memory) | Open the Homebrew Menu by holding R while launching a game |
| "shader pré-compilado ausente" in the log, missing graphics | `umvc3/dksh.pak` missing or incomplete | Run the installer again |
| The installer warns that `umvc3.exe` is another version | Game version differs from the supported one | Update the game on Steam and run the installer again |
| The CE version opens the original game | `umvc3/ce/` or `umvc3/ce-data/Characters.ini` is missing, or your CE is not 1.2.3b | Run the installer again and answer "Yes" to the Community Edition |
| VERSUS does not leave the controller screen | Fewer than two controllers paired | Pair two controllers (or two sideways Joy-Con) on the system screen |
| Frame drops in heavy fights | Handheld GPU limit | The port lowers the scene resolution by itself; see `docs/OPTIONS.md` |

## Sending a report

Attach the `umvc3/host-nx.log` file from the card (it is rewritten on every launch; the three previous ones stay
as `host-nx-NN.log`). Say which of the two versions you opened, which screen you were on and what you pressed.
The log itself is written in Portuguese.
