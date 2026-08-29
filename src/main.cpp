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
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

}  // namespace

// Bouton « Ouvrir » de l'application : datastream V0, widget Button en mode Push.
BLYNK_WRITE(V0) {
    if (param.asInt() != 1) return;  // on ignore le relachement du bouton
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

    g_relay.begin();
    g_watchdog.begin(millis());

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    g_lastWifiAttempt = millis();

    // config() et non begin() : begin() bloque jusqu'a la connexion, ce qui
    // rendrait la carte muette tant que le WiFi est absent.
    Blynk.config(BLYNK_AUTH_TOKEN);
}

void loop() {
    const uint32_t now = millis();

    // En premier : la temporisation du relais prime sur tout le reste.
    g_relay.update();

    maintainWifi(now);

    if (WiFi.status() == WL_CONNECTED) {
        if (Blynk.connected()) {
            Blynk.run();
        } else if (!g_relay.isBusy()) {
            // Blynk.connect() est bloquant. On ne l'appelle que relais au repos,
            // pour qu'il ne puisse jamais rallonger une impulsion en cours.
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
