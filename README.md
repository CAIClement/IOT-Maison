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

## Validation matérielle

Aucun de ces tests n'est couvert par les tests unitaires : `src/main.cpp` est
exclu de la compilation native (`build_src_filter`). C'est la seule barrière.

**Sur table, rien de raccordé au portail** — le relais claque dans le vide.

| # | Test | Critère | Date | Résultat |
| --- | --- | --- | --- | --- |
| 0 | Appui sur le bouton de l'appli | Deux clics du relais espacés d'environ 400 ms | | |
| 1 | Débrancher / rebrancher l'USB × 5 | Le relais **ne claque jamais** au démarrage | | |
| 2 | Couper le WiFi 2 min, puis le rétablir | Repasse *online* seul, sans intervention | | |
| 5 | Double appui rapide | Une seule impulsion ; la 2ᵉ commande est `refusee` au moniteur série | | |

**Au portail** — ⚠️ disjoncteur coupé pour toute intervention dans le coffret.

| # | Test | Critère | Date | Résultat |
| --- | --- | --- | --- | --- |
| 3 | Ponter les 2 bornes `SbS` avec un fil, 1 s | Le portail s'ouvre | | |
| 4 | Appui depuis l'appli, en WiFi maison | Le portail s'ouvre | | |
| 6 | Appui depuis l'extérieur, en 4G | Le portail s'ouvre | | |

Le test 3 conditionne tout le reste : si le pontage manuel ne déclenche rien,
`SbS` est désactivé dans la configuration de la CL201 et aucune modification du
firmware n'y changera quoi que ce soit.

### Tests supplémentaires issus de la revue de code

Ceux-ci couvrent des défauts trouvés à la relecture. Ils ne figuraient pas au
plan initial et sont les plus importants des sept ci-dessus.

| # | Test | Pourquoi | Date | Résultat |
| --- | --- | --- | --- | --- |
| 7 | Relais relié à une LED ou un buzzer (**pas au portail**) : appuyer sur Ouvrir tout en coupant le point d'accès, ou en éloignant la carte. ~20 répétitions | Une coupure **en plein message** est le déclencheur du défaut corrigé en `2b8460c`. Aucune fermeture ne doit dépasser ~0,5 s | | |
| 8 | Mesurer la durée de fermeture sur ~20 appuis (2ᵉ ESP32, oscilloscope, ou ralenti du téléphone sur la LED du relais) | `pulseMs` est une borne **basse** : rien ne vérifie la borne haute | | |
| 9 | Dans la console Blynk, relever l'option *« Sync with latest server value every time device connects »* du datastream `V0`. Puis : appui long, tuer l'appli en plein appui, redémarrer l'ESP32 | Une valeur `1` restée en mémoire serveur pourrait rouvrir le portail à chaque reconnexion. La garde anti-rejeu de 3 s doit l'absorber | | |
| 10 | Ponter `SbS` volontairement 3 s, puis 10 s, et noter la réaction de la CL201 | Donne la marge réelle tolérable sur un contact maintenu. À faire **avant** de faire confiance au relais | | |
| 11 | Laisser tourner 24–48 h au portail, compter les redémarrages (LED ou moniteur série) | Le WiFi y est limite ; c'est la seule façon de savoir si le lien tient | | |
