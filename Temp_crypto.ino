#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <array>

#include "crypto_icons.h"

// ========================
// CONFIG WIFI
// ========================
constexpr char WIFI_SSID[]     = "BELL582";
constexpr char WIFI_PASSWORD[] = "157919169AD6";

constexpr int SCREEN_WIDTH  = 240;
constexpr int SCREEN_HEIGHT = 320;
constexpr unsigned long UPDATE_INTERVAL_MS = 5UL * 60UL * 1000UL;
constexpr uint8_t WIFI_MAX_ATTEMPTS = 30;
constexpr size_t JSON_DOC_SIZE = 768;
constexpr lv_coord_t ROW_X = 20;
constexpr lv_coord_t ROW_FIRST_Y = 70;
constexpr lv_coord_t ROW_STEP = 35;
constexpr char COINGECKO_URL[] = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,bittensor&vs_currencies=usd";

// ========================
// OBJETS LVGL (global)
// ========================
lv_obj_t* label_update;

struct CryptoRow {
  const char* apiId;
  const char* ticker;
  const lv_image_dsc_t* icon;
  lv_coord_t yOffset;
  lv_obj_t* label;
};

std::array<CryptoRow, 3> cryptoRows = {{
  {"bitcoin",  "BTC", &image_btc, ROW_FIRST_Y,     nullptr},
  {"ethereum", "ETH", &image_eth, ROW_FIRST_Y + ROW_STEP, nullptr},
  {"bittensor","TAO", &image_tao, ROW_FIRST_Y + 2 * ROW_STEP, nullptr},
}};

// ========================
// WIFI
// ========================
bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connexion WiFi à %s", WIFI_SSID);

  for (uint8_t attempt = 0; attempt < WIFI_MAX_ATTEMPTS && WiFi.status() != WL_CONNECTED; ++attempt) {
    delay(500);
    Serial.print('.');
  }

  const bool connected = WiFi.status() == WL_CONNECTED;
  Serial.println(connected ? "\n✅ WiFi connecté" : "\n❌ Échec WiFi");
  return connected;
}

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.println("Connexion WiFi perdue, tentative de reconnexion...");
  WiFi.disconnect(true);
  delay(100);
  return connectToWiFi();
}

// ========================
// LVGL TICK
// ========================
void lv_tick_task(void *arg) {
  (void)arg;
  while (1) {
    lv_tick_inc(5);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ========================
// FORMATAGE PRIX
// ========================
String formatPrice(double value) {
  char buffer[32];
  dtostrf(value, 0, 2, buffer);

  String formatted(buffer);
  formatted.trim();

  int dotIndex = formatted.indexOf('.');
  if (dotIndex < 0) {
    dotIndex = formatted.length();
  }

  for (int i = dotIndex - 3; i > 0; i -= 3) {
    formatted = formatted.substring(0, i) + "," + formatted.substring(i);
  }

  return formatted;
}

void updateStatusLabel(const String& message) {
  if (label_update != nullptr) {
    lv_label_set_text(label_update, message.c_str());
  }
}

void updateTimestampLabel() {
  if (label_update == nullptr) {
    return;
  }

  const unsigned long seconds = millis() / 1000UL;
  const unsigned long minutes = seconds / 60UL;
  const unsigned long remainingSeconds = seconds % 60UL;

  char buffer[48];
  snprintf(buffer, sizeof(buffer), "Maj: %lum %lus (CoinGecko)", minutes, remainingSeconds);
  lv_label_set_text(label_update, buffer);
}

void updateCryptoLabels(const DynamicJsonDocument& doc) {
  for (auto& crypto : cryptoRows) {
    const double price = doc[crypto.apiId]["usd"] | 0.0;
    const String text = String(crypto.ticker) + "  $" + formatPrice(price);
    lv_label_set_text(crypto.label, text.c_str());
  }
}

// ========================
// API CRYPTO (COINGECKO)
// ========================
void get_crypto_data() {
  if (!ensureWiFiConnected()) {
    updateStatusLabel("WiFi indisponible");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);

  HTTPClient http;
  if (!http.begin(client, COINGECKO_URL)) {
    Serial.println("Impossible d'initialiser la requête HTTP");
    updateStatusLabel("Erreur initialisation HTTP");
    return;
  }

  const int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    const String payload = http.getString();
    const DeserializationError err = deserializeJson(doc, payload);

    if (!err) {
      updateCryptoLabels(doc);
      updateTimestampLabel();
    } else {
      Serial.printf("Erreur JSON: %s\n", err.c_str());
      updateStatusLabel("Réponse JSON invalide");
    }
  } else {
    Serial.printf("Erreur HTTP: %d\n", httpCode);
    updateStatusLabel(String("HTTP ") + httpCode);
  }

  http.end();
}

// ========================
// ECRAN CRYPTO
// ========================
void create_crypto_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101010), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  // TITRE
  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Crypto Prices");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);

  for (auto& crypto : cryptoRows) {
    lv_obj_t* icon = lv_image_create(scr);
    lv_image_set_src(icon, crypto.icon);
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, ROW_X, crypto.yOffset);

    crypto.label = lv_label_create(scr);
    lv_label_set_text(crypto.label, (String(crypto.ticker) + " : $---").c_str());
    lv_obj_set_style_text_font(crypto.label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(crypto.label, lv_color_white(), 0);
    lv_obj_align_to(crypto.label, icon, LV_ALIGN_OUT_RIGHT_MID, 12, 0);
  }

  // Bas de page
  label_update = lv_label_create(scr);
  lv_label_set_text(label_update, "Source: CoinGecko");
  lv_obj_set_style_text_font(label_update, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label_update, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(label_update, LV_ALIGN_BOTTOM_MID, 0, -10);

  lv_scr_load(scr);
}

// ========================
// SETUP / LOOP
// ========================
unsigned long lastUpd = 0;

void setup() {
  Serial.begin(115200);

  lv_init();

  static uint32_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10];
  lv_display_t* disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, buf, sizeof(buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  xTaskCreatePinnedToCore(lv_tick_task, "LVGL tick", 2048, NULL, 1, NULL, 1);

  connectToWiFi();
  create_crypto_screen();
  get_crypto_data();
}

void loop() {
  lv_timer_handler();
  delay(5);

  const unsigned long now = millis();
  if (now - lastUpd > UPDATE_INTERVAL_MS) {
    lastUpd = now;
    get_crypto_data();
  }
}
