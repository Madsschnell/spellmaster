# SpellMaster

Fysisk duelspil med bevægelsesgenkendelse. SW3PRJ3, Gruppe 2, Forår 2026.

To spillere kaster spells ved at tegne figurer i luften med en IR-tryllestav. Et NoIR-kamera opfanger bevægelsen, og en central GameMaster styrer spillogik. Systemet kører på tre Raspberry Pi 5-enheder forbundet via WiFi.

## Mappestruktur

```
spellmaster/
├── common/                              Delte headers brugt af både player og gamemaster
│   ├── GameStatus.h                     Spillets tilstand (faser, HP, mana, effekter)
│   ├── PlayerAction.h                   Spillerhandlinger (READY)
│   ├── SpellMessage.h                   Spell-beskeder fra ImageProcessing
│   ├── Serialization.h                  nlohmann/json to_json/from_json for alle typer
│   └── Transport.h                      ZeroMQ wrapper-funktioner (PUB/SUB, PUSH/PULL)
├── gamemaster/
│   ├── spells.json                      Spell-definitioner (damage, mana, heal)
│   └── gamelogic/
│       ├── CMakeLists.txt
│       ├── GameLogic.h                  Spillogik: faser, spell resolution, player state
│       ├── GameLogic.cpp
│       ├── Player.h                     Spillerklasse (HP, mana)
│       ├── Player.cpp
│       └── main.cpp                     GameMaster-applikation (ZeroMQ netværk)
├── player/
│   ├── CMakeLists.txt
│   ├── main.cpp                         Player-applikation (UI + ImageProcessing + ZeroMQ)
│   ├── ui/
│   │   ├── CMakeLists.txt
│   │   ├── SpellMasterUI.h              UI-klasse (lobby, battle, result)
│   │   ├── SpellMasterUI.cpp
│   │   └── main.cpp                     Standalone UI-test (ingen netværk)
│   └── imageprocessing/
│       ├── ImageProcessing.h            Kamera-baseret IR-tracking og gesture-genkendelse
│       ├── ImageProcessing.cpp
│       ├── ImageProcessingStub.h        Tastatur-stub til desktop-test
│       ├── GestureClassifier.h          Mønstergenkendelse (cirkel, firkant, zigzag, ...)
│       ├── GestureClassifier.cpp
│       └── Spells.h                     Gesture-typer og display-navne
├── poc/                                 Proof-of-concept fra foranalysen
│   ├── camview.cpp
│   ├── colortest.cpp
│   └── zmq_latency_test.cpp
├── CMakeLists.txt                       Top-level CMake
├── Doxyfile                             Doxygen-konfiguration
├── .gitignore
└── README.md
```

## Dependencies

| Pakke | Formål | Påkrævet |
|-------|--------|----------|
| CMake >= 3.16 | Build system | Ja |
| C++17 compiler (GCC, Clang) | Kompilering | Ja |
| Qt6 | Player UI | Ja |
| ZeroMQ (libzmq) | Netværkskommunikation | Ja |
| nlohmann_json | JSON serialisering | Ja (FetchContent fallback) |
| cppzmq | ZeroMQ C++ bindings | Ja (FetchContent fallback) |
| OpenCV | IR-detektion og gesture-genkendelse | Ja |
| Cage | Wayland kiosk compositor | Kun Player RPi |
| Doxygen + Graphviz | Kodedokumentation | Valgfrit |

### Raspberry Pi OS (Bookworm 64-bit)

```bash
sudo apt update
sudo apt install cmake g++ qt6-base-dev libzmq3-dev nlohmann-json3-dev libopencv-dev cage
```

Cage installeres kun på Player-enhederne. GameMaster har ikke brug for det.

### Arch Linux

```bash
sudo pacman -S cmake gcc qt6-base zeromq cppzmq nlohmann-json opencv doxygen graphviz
```

### macOS (Homebrew)

```bash
brew install cmake qt@6 zeromq cppzmq nlohmann-json opencv doxygen graphviz
```

### Windows

To muligheder:

**Remote SSH (anbefalet):** Brug VS Code med Remote SSH extension til at udvikle direkte på en RPi. Se [VS Code Remote SSH](https://code.visualstudio.com/docs/remote/ssh). Dependencies installeres på Pi'en (se RPi OS ovenfor).

**Lokal build med MSYS2:** Installér [MSYS2](https://www.msys2.org/) og MinGW-toolchain via [VS Code C++ guide](https://code.visualstudio.com/docs/cpp/config-mingw). Installér derefter dependencies i MSYS2 UCRT64 terminalen:

```bash
pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc \
         mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-zeromq \
         mingw-w64-ucrt-x86_64-cppzmq mingw-w64-ucrt-x86_64-nlohmann-json \
         mingw-w64-ucrt-x86_64-opencv
```

## Byg

```bash
cmake -B build
cmake --build build
```

Bygger tre executables:

| Executable | Placering | Beskrivelse |
|------------|-----------|-------------|
| `gamemaster` | `build/gamemaster/gamelogic/` | GameMaster: spillogik og netværk |
| `spellmaster_player` | `build/player/` | Player: UI + ImageProcessing + netværk |
| `spellmaster_ui` | `build/player/ui/` | Standalone UI rendertest (ingen netværk) |

## Kør

### Test fra egen computer (3 terminaler)

Spillet kan testes lokalt på din egen computer uden kamera og RPi. Brug `--stub` for tastatur-input og `--windowed` for at køre i et vindue:

```bash
# Terminal 1: GameMaster
./build/gamemaster/gamelogic/gamemaster

# Terminal 2: Player 1
./build/player/spellmaster_player player1 localhost --stub --windowed

# Terminal 3: Player 2
./build/player/spellmaster_player player2 localhost --stub --windowed
```

Tastatur (stub mode):

| Tast | Funktion |
|------|----------|
| 1 | Fireball |
| 2 | Lightning |
| 3 | Shield |
| 4 | Heal |
| 5 | Ukendt gesture (test af fejlvisning) |
| Space | READY |
| Esc | Afslut |

### RPi produktion (under Cage)

```bash
# GameMaster (RPi 1)
./build/gamemaster/gamelogic/gamemaster

# Player 1 (RPi 2, med kamera)
sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- ./build/player/spellmaster_player player1 <gamemaster-ip>

# Player 2 (RPi 3, med kamera)
sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- ./build/player/spellmaster_player player2 <gamemaster-ip>
```

### Player flags

| Flag | Beskrivelse |
|------|-------------|
| `--stub` | Tastatur-input i stedet for kamera (til desktop-test uden kamera) |
| `--auto` | Auto-cast tilfældige spells (sætter implicit --stub) |
| `--windowed` | Kør i et vindue i stedet for fullscreen (til desktop-test) |
| `--debug` | Aktivér MJPEG debug-stream af kamerabillede på port 8080 |

### Debug-stream

Med `--debug` sender Player en MJPEG-stream af kamerabilledet med IR-crosshair, gesture-sti og fase-indikator. Forbind fra en anden maskine på samme netværk:

```bash
# mpv
mpv http://<player-ip>:8080

# VLC
vlc http://<player-ip>:8080

# Eller åbn URL'en direkte i en browser
```

## Doxygen

Generér kodedokumentation (kræver `doxygen` og `graphviz`):

```bash
doxygen
```

Åbn `docs/html/index.html` i en browser. Output-mappen er i `.gitignore` og committes ikke.

## Versionering

Versionsnummer opdateres manuelt to steder ved release:

- `player/ui/SpellMasterUI.h`: `UI_VERSION`
- `Doxyfile`: `PROJECT_NUMBER`

## Netværk

| Port | Protokol | Retning | Indhold |
|------|----------|---------|---------|
| 5555 | ZeroMQ PUB/SUB | GameMaster til Player | GameStatus (hvert sekund) |
| 5556 | ZeroMQ PUSH/PULL | Player til GameMaster | SpellMessage og PlayerAction |
| 8080 | HTTP MJPEG | Player (kun med --debug) | Kamerabillede med IR-overlay |

## Spilflow

LOBBY, GET_READY (5s), CASTING (10s), RESOLVING (5s), GET_READY, ..., GAME_OVER

Tider konfigureres i GameLogic. UI og ImageProcessing reagerer kun på den fase de modtager.

## Hardware

| Komponent | Antal | Beskrivelse |
|-----------|-------|-------------|
| Raspberry Pi 5 | 3 | 1x GameMaster, 2x Player |
| RPi 7" DSI touchskærm | 2 | Player UI (800x480) |
| RPi NoIR Camera Module | 2 | IR-tracking |
| IR-passfilter | 2 | Monteret på kameraet, blokerer synligt lys |
| IR-stav | 2 | Spillerens tryllestav |
| GL-AXT1800 router | 1 | Lokalt WiFi-netværk |

## Git workflow

- `main`: stabilt, protected
- `dev`: integration, protected, default branch
- Feature/refactor/hotfix branches oprettes fra `dev`
- Merge via MR på GitLab, delete source branch efter merge

## Gruppe

Luca Abildgaard, Oliver Bache, Morten Enghausen, Julius Gasberg, Phillip Lam, Viktor Laursen, Mads Posborg, Mads Schnell

Vejleder: Jesper Michael Kristensen
