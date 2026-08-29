#pragma once

#include <stdint.h>

/** Etat du lien, du plus degrade au plus complet. */
enum class LinkState {
    Offline,   // pas de WiFi
    WifiOnly,  // WiFi present, Blynk absent
    Online     // WiFi et Blynk presents
};

/**
 * Niveau a appliquer a la LED d'etat.
 *
 * Offline  -> eteinte
 * WifiOnly -> clignotement 500 ms / 500 ms
 * Online   -> allumee fixe
 */
bool statusLedLevel(LinkState link, uint32_t nowMs);
