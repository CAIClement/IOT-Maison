# Portail — ouverture depuis le téléphone

Ouvre le portail (automatisme Nice CL201R10) depuis l'application Blynk, via un
ESP32 qui simule un appui sur bouton mural filaire.

Conception : `docs/superpowers/specs/2026-08-29-portail-blynk-esp32-design.md`

## Câblage

| Module relais | ESP32 |
| --- | --- |
| `VCC` | `3V3` |
| `GND` | `GND` |
| `IN` | `GPIO26` |

**Résistance 10 kΩ entre `GPIO26` et `GND`** — obligatoire. Sans elle, la broche
flotte pendant le boot et le relais peut se fermer tout seul, ouvrant le portail
à chaque coupure de courant.

Sorties `COM` et `NO` du relais → les deux bornes du bornier `SbS` de la CL201
(contact sec, pas de polarité).

Alimentation : chargeur USB-C 5 V / 2 A sur la prise étanche sous le coffret.

⚠️ Toute intervention dans le coffret Nice se fait **disjoncteur coupé** : il
contient du 230 V.

## Installation

1. `python -m pip install --upgrade platformio`
2. Copier `src/secrets.h.example` en `src/secrets.h` et le remplir.
3. `python -m platformio run -e esp32dev -t upload`

## Commandes

| But | Commande |
| --- | --- |
| Tests unitaires (sur PC) | `python -m platformio test -e native` |
| Compiler le firmware | `python -m platformio run -e esp32dev` |
| Flasher | `python -m platformio run -e esp32dev -t upload` |
| Moniteur série | `python -m platformio device monitor -b 115200` |
