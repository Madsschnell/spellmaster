# SpellMaster

Fysisk duelspil med bevægelsesgenkendelse. SW3PRJ3, Gruppe 2, Forår 2026.

## Overblik

SpellMaster er et distribueret embedded system hvor to spillere kaster spells ved at tegne figurer i luften med en IR-tryllestav. Et NoIR-kamera opfanger bevægelsen, og en central Gamemaster styrer spillogik. Systemet kører på tre Raspberry Pi 5-enheder forbundet via WiFi.

## Mappestruktur

```
spellmaster/
├── common/                     Delte header-filer brugt af både player og gamemaster
│   └── gamestatus.h            GameStatus struct (gameState-interfacet)
│
├── player/                     Kode der kører på Player-noderne
│   ├── ui/                     Player UI (Qt6, touchskærm)
│   │   ├── CMakeLists.txt
│   │   └── spellmaster_ui.cpp
│   └── imageprocessing/        IR-tracking og mønstergenkendelse
│
├── gamemaster/                 Kode der kører på Gamemaster-noden
│   └── gamelogic/              Spillogik, state machine, spilregler
│
├── poc/                        Proof-of-concept fra foranalysen (ikke produktion)
│
├── CMakeLists.txt              Top-level CMake
├── .gitignore
└── README.md
```

### Oversigt over komponenter

| Mappe | Node | Beskrivelse |
|-------|------|-------------|
| `common/` | Begge | Delte datastrukturer. Ingen binærer, kun headers. |
| `player/ui/` | Player | Viser spilstatus på touchskærm. Modtager `GameStatus` via ZeroMQ. |
| `player/imageprocessing/` | Player | Kamera, IR-tracking, mønstergenkendelse. Sender genkendt spell til GameMaster via ZeroMQ. |
| `gamemaster/gamelogic/` | Gamemaster | State machine, skadeberegning, rundehåndtering. Sender `GameStatus` til begge UI'er. |
| `poc/` | Desktop | Testapplikationer fra foranalysen. Ikke en del af det endelige system. |

### common/

`common/` indeholder delte header-filer der inkluderes af komponenter på begge sider af netværket. Den centrale fil er `gamestatus.h`, som definerer `GameStatus`-structen og dens undertyper (`GamePhase`, `PlayerInfo`, `EffectInfo`). Denne header er kontrakten mellem GameLogic og UI.

Efterhånden som ZeroMQ-integrationen udvikles, vil mappen også indeholde serialisering og transportkode.

## Krav

### Platform
- Raspberry Pi 5, Raspberry Pi OS Lite (Bookworm, 64-bit, headless)
- C++17, kompileret med GCC
- Lokalt netværk via GL.iNet GL-AXT1800 travel router

### Dependencies

Ikke alle pakker er nødvendige for alle dele: Qt6 bruges kun af Player UI.

**RPi 5 (Raspberry Pi OS):**
```bash
sudo apt update
sudo apt install cmake g++ qt6-base-dev cage
```

**Arch Linux (desktop):**
```bash
sudo pacman -S cmake gcc qt6-base
```

## Byg

Projektet bruger CMake. Top-level `CMakeLists.txt` bygger alt:

```bash
cmake -B build
cmake --build build
```

### Player UI

UI'en har to modes:

**Normal mode** (RPi med touchskærm under Cage):
```bash
sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- ./build/player/ui/spellmaster_ui
```

**Test mode** (desktop, to spillervinduer + controller):
```bash
./build/player/ui/spellmaster_ui --test
```

Test mode controls:

| Tast | Funktion |
|------|----------|
| Space | Start spil |
| 1-4 | Player 1: Fireball / Lightning / Shield / Heal |
| 5-8 | Player 2: Fireball / Lightning / Shield / Heal |
| R | Resolve runde |
| N | Skip fase |
| A | Auto mode (tilfældige spells, fuldt tidsflow) |
| 0 | Reset spil |
| Esc | Afslut |

## Git workflow

- `main` stabilt, protected
- `dev` integration, protected, default branch
- Feature branches oprettes fra issues på GitLab (source: `dev`)

```bash
git fetch
git checkout <branch-navn>
git add -A
git commit -m "Beskrivelse"
git push
# Opret merge request til dev på GitLab
```

## Gruppe

Luca Abildgaard, Oliver Bache, Morten Enghausen, Julius Gasberg, Phillip Lam, Viktor Laursen, Mads Posborg, Mads Schnell

Vejleder: Jesper Michael Kristensen
