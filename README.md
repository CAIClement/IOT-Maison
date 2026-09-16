# Portail — ouverture depuis le téléphone

Ouvre le portail (automatisme Nice CL201R10) depuis l'application Blynk, via un
ESP32 qui simule un appui sur bouton mural filaire.

![ESP32](https://img.shields.io/badge/ESP32-Arduino-00979D)
![PlatformIO](https://img.shields.io/badge/build-PlatformIO-orange)
![Tests](https://img.shields.io/badge/tests%20unitaires-20%20%2F%2020-brightgreen)
![C++](https://img.shields.io/badge/C%2B%2B-14-blue)

## En bref

Le portail de la maison s'ouvre avec une télécommande radio. Ce projet ajoute un
bouton dans une application mobile, utilisable **de n'importe où** (4G ou WiFi),
sans rien retirer à l'existant : la télécommande continue de fonctionner.

L'ESP32 reçoit la commande depuis Blynk Cloud et ferme un relais 400 ms sur
l'entrée « bouton mural » de l'automatisme, exactement comme un appui manuel.

```
Téléphone ──► Blynk Cloud ──► Box ── WiFi ──► ESP32 ──► Relais ──► Entrée SbS du portail
```

## Points techniques

Un relais qui commande un portail doit surtout **ne jamais se déclencher tout
seul**. L'essentiel du travail porte là-dessus :

- **Aucun déclenchement au démarrage** — résistance de pull-down matérielle,
  broche mise à l'état bas *avant* d'être passée en sortie, et garde logicielle
  de 3 s après le boot. Version du framework épinglée, car arduino-esp32 3.x
  casse cet ordre d'initialisation sans prévenir.
- **Anti-rebond et verrou** — machine à états `Idle → Pulsing → Lockout` : une
  rafale d'appuis ne produit qu'une impulsion toutes les 2,4 s. Vérifié sur
  table avec ~20 appuis consécutifs.
- **Pas de commande rejouée** — une valeur restée en mémoire côté serveur et
  renvoyée à la reconnexion est ignorée, pour que le portail ne s'ouvre pas seul
  après une coupure réseau.
- **WiFi en limite de portée** — tentatives de reconnexion espacées et bornées
  dans le temps, lancées uniquement relais au repos pour ne jamais prolonger une
  impulsion, et redémarrage automatique après 5 min hors ligne.
- **Logique testable sans la carte** — le relais, le watchdog et la LED d'état
  ne dépendent pas d'Arduino : la sortie et l'horloge sont injectées. 20 tests
  unitaires tournent sur PC, y compris le débordement de `millis()` après
  49 jours.
- **Secrets hors du dépôt** — le jeton Blynk permet d'ouvrir le portail : il vit
  dans `src/secrets.h`, ignoré par git. Seul un modèle aux valeurs factices est
  versionné.

## Matériel

- ESP32 DevKit (ELEGOO, USB-C)
- Module relais 1 canal optocouplé, déclenchement niveau haut
- Résistance 10 kΩ
- Chargeur USB-C 5 V / 2 A, boîtier étanche IP65
- Automatisme de portail Nice CL201R10 (entrée pas-à-pas `SbS`)

## Structure du dépôt

```
include/config.h          broches et temporisations
include/, src/            GateRelay, ConnectionWatchdog, StatusLed (logique pure)
src/main.cpp              câblage Arduino : WiFi, Blynk, boucle principale
src/secrets.h.example     modèle des identifiants (à copier en secrets.h)
test/                     tests unitaires Unity, exécutés en natif
```

## Choix d'architecture

**ESP32 + Blynk Cloud**, retenu face à deux alternatives :

- **Serveur web local sur l'ESP32** — écarté : ne fonctionne qu'à portée du WiFi
  de la maison, donc inutilisable en arrivant en voiture, le cas d'usage
  principal.
- **Home Assistant / ESPHome** — écarté : impose un serveur allumé en
  permanence, disproportionné pour un bouton unique.

Le prix de ce choix est la dépendance à un service tiers et à la connexion
internet. Il est acceptable parce que la télécommande radio reste disponible en
repli permanent.

Volontairement hors périmètre de cette version : retour d'état du portail,
ouverture par géolocalisation, historique et notifications.

## Câblage

| Module relais | ESP32 |
| --- | --- |
| `VCC` | `3V3` |
| `GND` | `GND` |
| `IN` | `GPIO18` |

**Résistance 10 kΩ entre `GPIO18` et `GND`** — obligatoire. Sans elle, la broche
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
| 0 | Appui sur le bouton de l'appli | Deux clics du relais espacés d'environ 400 ms | 2026-08-30 | ✅ OK |
| 1 | Débrancher / rebrancher l'USB × 5 | Le relais **ne claque jamais** au démarrage | 2026-08-30 | ✅ OK |
| 2 | Couper le WiFi 2 min, puis le rétablir | Repasse *online* seul, sans intervention | | |
| 5 | Double appui rapide | Une seule impulsion ; la 2ᵉ commande est `refusee` au moniteur série | 2026-08-30 | ✅ OK — ~20 appuis en rafale, 16 acceptées, écart minimum **2,4 s** = `kPulseMs` + `kLockoutMs` |

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
| 7 | Relais relié à une LED ou un buzzer (**pas au portail**) : appuyer sur Ouvrir tout en coupant le point d'accès, ou en éloignant la carte. ~20 répétitions | Une coupure **en plein message** est le déclencheur du défaut corrigé en `e9c778d`. Aucune fermeture ne doit dépasser ~0,5 s | | |
| 8 | Mesurer la durée de fermeture sur ~20 appuis (2ᵉ ESP32, oscilloscope, ou ralenti du téléphone sur la LED du relais) | `pulseMs` est une borne **basse** : rien ne vérifie la borne haute | | |
| 9 | Appui long sur Ouvrir, tuer l'appli en plein appui, puis redémarrer l'ESP32 | Le portail ne doit **pas** s'ouvrir à la reconnexion | | |
| 10 | Ponter `SbS` volontairement 3 s, puis 10 s, et noter la réaction de la CL201 | Donne la marge réelle tolérable sur un contact maintenu. À faire **avant** de faire confiance au relais | | |
| 11 | Laisser tourner 24–48 h au portail, compter les redémarrages (LED ou moniteur série) | Le WiFi y est limite ; c'est la seule façon de savoir si le lien tient | | |

### Réglage Blynk du datastream `V0`

Constaté dans la console le 2026-08-29 : l'option **« Synchroniser avec la
dernière valeur du serveur lors de la reconnexion »** est **désactivée par
défaut**. La documentation Blynk ne l'indiquait nulle part.

La laisser désactivée. Si elle était activée, un `1` resté en mémoire côté
serveur — appui dont le relâchement n'a jamais atteint le serveur, appli tuée en
plein appui — serait rejoué à chaque reconnexion et ouvrirait le portail sans
personne. Combiné au redémarrage automatique après 5 min de déconnexion, ça
pourrait cycler le portail toute la nuit.

La garde `kSyncGuardMs` de 3 s dans `src/main.cpp` couvre ce cas de toute façon :
une valeur poussée par le serveur arrive immédiatement après la connexion, un
appui humain non. Le réglage et la garde sont deux barrières indépendantes.
