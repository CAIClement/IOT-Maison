// secrets.h definit BLYNK_TEMPLATE_ID / BLYNK_TEMPLATE_NAME / BLYNK_AUTH_TOKEN,
// qui doivent etre connus AVANT l'inclusion de BlynkSimpleEsp32.h.
#include "secrets.h"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#include "config.h"
#include "connection_watchdog.h"
#include "gate_relay.h"
#include "status_led.h"

namespace {

void relayWrite(bool closed) {
    digitalWrite(kRelayPin, closed ? HIGH : LOW);
}

uint32_t clockMs() {
    return millis();
}

GateRelay g_relay(relayWrite, clockMs,
                  GateRelay::Config{kPulseMs, kLockoutMs, kBootGuardMs});

ConnectionWatchdog g_watchdog(kReconnectTimeoutMs);

uint32_t g_lastWifiAttempt = 0;
uint32_t g_lastBlynkAttempt = 0;
bool g_blynkWasConnected = false;
uint32_t g_blynkConnectedAt = 0;

LinkState currentLink() {
    if (Blynk.connected()) return LinkState::Online;
    if (WiFi.status() == WL_CONNECTED) return LinkState::WifiOnly;
    return LinkState::Offline;
}

/** Relance la connexion WiFi au plus une fois toutes les kWifiRetryMs. */
void maintainWifi(uint32_t now) {
    if (WiFi.status() == WL_CONNECTED) return;
    if (now - g_lastWifiAttempt < kWifiRetryMs) return;
    g_lastWifiAttempt = now;
    // Pas de WiFi.disconnect() : l'association peut etre en cours et la couper
    // relancerait tout depuis le debut, indefiniment, sur un lien faible.
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

}  // namespace

// Bouton « Ouvrir » de l'application : datastream V0, widget Button en mode Push.
BLYNK_WRITE(V0) {
    if (param.asInt() != 1) return;  // on ignore le relachement du bouton

    // Une valeur poussee par le serveur a la reconnexion arrive immediatement ;
    // un appui humain, non. On refuse donc toute commande trop proche de la
    // connexion, pour ne pas ouvrir le portail sur un « 1 » reste en memoire.
    if (millis() - g_blynkConnectedAt < kSyncGuardMs) {
        Serial.println(F("[portail] commande V0 ignoree (rejeu a la reconnexion)"));
        return;
    }

    const bool accepted = g_relay.trigger();
    Serial.print(F("[portail] commande V0 -> "));
    Serial.println(accepted ? F("acceptee") : F("refusee"));
}

void setup() {
    // Ordre critique : le registre de sortie est mis a 0 AVANT que la broche ne
    // devienne une sortie, pour eviter tout front haut d'un cycle d'horloge.
    digitalWrite(kRelayPin, LOW);
    pinMode(kRelayPin, OUTPUT);
    digitalWrite(kRelayPin, LOW);

    pinMode(kStatusLedPin, OUTPUT);
    digitalWrite(kStatusLedPin, LOW);

    Serial.begin(115200);
    Serial.println(F("\n[portail] demarrage"));

    // Filet de securite : si loop() se bloque malgre tout, le chien de garde
    // materiel redemarre la carte. Un reset libere GPIO26, et la resistance de
    // tirage ouvre le contact. Toutes les operations bloquantes de loop() sont
    // desormais bornees a ~2 s, bien en dessous du seuil du chien de garde.
    enableLoopWDT();

    g_relay.begin();
    g_watchdog.begin(millis());

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    g_lastWifiAttempt = millis();

    // config() et non begin() : begin() bloque jusqu'a la connexion, ce qui
    // rendrait la carte muette tant que le WiFi est absent.
    Blynk.config(BLYNK_AUTH_TOKEN);

    // BLYNK_TIMEOUT_MS (6000, en millisecondes) est passe a
    // WiFiClient::setTimeout() qui attend des SECONDES : le timeout reel serait
    // de 6000 s (~100 min). Pendant ce temps Blynk.run() boucle sans rendre la
    // main et le relais reste colle. On le ramene a 1 s.
    _blynkWifiClient.setTimeout(1);
}

void loop() {
    const uint32_t now = millis();

    // En premier : la temporisation du relais prime sur tout le reste.
    g_relay.update();

    maintainWifi(now);

    const bool blynkConnected = Blynk.connected();
    if (blynkConnected && !g_blynkWasConnected) {
        g_blynkConnectedAt = now;  // arme la garde anti-rejeu
    }
    g_blynkWasConnected = blynkConnected;

    if (WiFi.status() == WL_CONNECTED) {
        if (blynkConnected) {
            Blynk.run();
        } else if (!g_relay.isBusy() && now - g_lastBlynkAttempt >= kBlynkRetryMs) {
            // Blynk.connect() est bloquant. On ne l'appelle que relais au repos,
            // pour qu'il ne puisse jamais rallonger une impulsion en cours, et au
            // plus une fois toutes les kBlynkRetryMs : chaque appel commence par
            // fermer la socket en cours, ce qui saboterait un login en vol.
            g_lastBlynkAttempt = now;
            Blynk.connect(kBlynkConnectTimeoutMs);
        }
    }

    g_watchdog.update(Blynk.connected(), now);
    if (g_watchdog.shouldRestart()) {
        Serial.println(F("[portail] deconnecte trop longtemps, redemarrage"));
        Serial.flush();
        ESP.restart();
    }

    digitalWrite(kStatusLedPin, statusLedLevel(currentLink(), now) ? HIGH : LOW);
}
