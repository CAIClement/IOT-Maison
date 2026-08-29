#pragma once

#include <stdint.h>

// --- Broches -----------------------------------------------------------------

// GPIO26 : ni strapping pin, ni broche emettant un signal au reset.
// IMPERATIF : resistance de 10 kOhm entre cette broche et GND.
constexpr uint8_t kRelayPin = 26;

// LED integree de la carte ESP32 DevKit.
constexpr uint8_t kStatusLedPin = 2;

// --- Temporisations (ms) -----------------------------------------------------

constexpr uint32_t kPulseMs = 400;              // duree d'un appui bouton typique
constexpr uint32_t kLockoutMs = 2000;           // empeche le double declenchement
constexpr uint32_t kBootGuardMs = 3000;         // ignore les commandes au demarrage
constexpr uint32_t kSyncGuardMs = 3000;         // ignore les valeurs rejouees a la reconnexion
constexpr uint32_t kReconnectTimeoutMs = 300000;  // 5 min avant redemarrage
constexpr uint32_t kWifiRetryMs = 30000;        // intervalle entre 2 tentatives WiFi
constexpr uint32_t kBlynkConnectTimeoutMs = 2000;  // tentative Blynk bornee
constexpr uint32_t kBlynkRetryMs = 5000;        // throttle des tentatives de connexion Blynk
