# Port options

Most people never need this. Each option is a one-line text file in `umvc3/` on the card; deleting the file
restores the default.

| File | Content | Effect |
|---|---|---|
| `dk_rscale.txt` | `auto` (default), or `70` to `100` | 3D scene resolution in % (the HUD is always 720p). Automatic: the port picks 100, 90, 80 or 70 according to GPU load |
| `ovl_fps.txt` | `0` or `1` | On-screen frame counter |
| `menu_jp.txt` | `0` | Menus back to the Xbox layout (bottom button confirms) |
| `joyhold.txt` | `h` | Lets the system rotate a sideways Joy-Con instead of the port |
| `versus_applet.txt` | `0` | Does not open the system controller screen when entering VERSUS |
| `apm.txt` | `0` | Does not request the official handheld performance profile |

Texture and effect quality are set in the game's own `config.ini`, in
`umvc3/userdata/user/CAPCOM/ULTIMATE MARVEL VS. CAPCOM 3/` (or `userdata-ce/` for the CE version).
