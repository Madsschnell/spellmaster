# SpellMaster

Fysisk duelspil med bevægelsesgenkendelse. SW3PRJ3, Gruppe 2, Forår 2026.

## Overblik

SpellMaster er et distribueret embedded system hvor to spillere kaster spells ved at tegne figurer i luften med en IR-tryllestav. Et NoIR-kamera opfanger bevægelsen, og en central Gamemaster styrer spillogik.

## Platform

- Raspberry Pi 5, Raspberry Pi OS Lite (Bookworm, 64-bit, headless)
- C++ med Qt6, Cage (Wayland kiosk-compositor)
- Lokalt netværk via GL.iNet GL-AXT1800 travel router

## Mappestruktur

```
poc/        Proof-of-concept testkode fra foranalysen
```

## Kompilering (på RPi 5)

```bash
g++ app.cpp -o app $(pkg-config --cflags --libs Qt6Widgets) -fPIC
sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- /home/spellmaster/app
```

## Gruppe

Luca Abildgaard, Oliver Bache, Morten Enghausen, Julius Gasberg, Phillip Lam, Viktor Laursen, Mads Posborg, Mads Schnell

Vejleder: Jesper Michael Kristensen
