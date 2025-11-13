#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

#include "crypto_icons.h"

// ========================
// CONFIG WIFI
// ========================
const char* ssid     = "BELL582";
const char* password = "157919169AD6";

const int SCREEN_WIDTH  = 240;
const int SCREEN_HEIGHT = 320;

// ========================
// OBJETS LVGL (global)
// ========================
lv_obj_t* label_btc;
lv_obj_t* label_eth;
lv_obj_t* label_tao;
lv_obj_t* label_update;

// ========================
// WIFI
// ========================
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connexion WiFi");

  for (int i = 0; i < 30; i++) {
    if (WiFi.status() == WL_CONNECTED) break;
    delay(500);
    Serial.print(".");
  }

  Serial.println(WiFi.status() == WL_CONNECTED ?
     "\n✅ WiFi connecté" : "\n❌ Échec WiFi");
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
String formatPrice(double v) {
  return String(v, 2);
}

// ========================
// API CRYPTO (COINGECKO)
// ========================
void get_crypto_data() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,bittensor&vs_currencies=usd");
  int code = http.GET();

  if (code == 200) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, http.getString());

    double btc = doc["bitcoin"]["usd"]     | 0.0;
    double eth = doc["ethereum"]["usd"]    | 0.0;
    double tao = doc["bittensor"]["usd"]   | 0.0;

    lv_label_set_text(label_btc, ("BTC  $" + formatPrice(btc)).c_str());
    lv_label_set_text(label_eth, ("ETH  $" + formatPrice(eth)).c_str());
    lv_label_set_text(label_tao, ("TAO  $" + formatPrice(tao)).c_str());
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

  // BTC
  lv_obj_t* btc_icon = lv_image_create(scr);
  lv_image_set_src(btc_icon, &image_btc);
  lv_obj_align(btc_icon, LV_ALIGN_TOP_LEFT, 20, 70);

  label_btc = lv_label_create(scr);
  lv_label_set_text(label_btc, "BTC : $---");
  lv_obj_set_style_text_font(label_btc, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label_btc, lv_color_white(), 0);
  lv_obj_align_to(label_btc, btc_icon, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

  // ETH
  lv_obj_t* eth_icon = lv_image_create(scr);
  lv_image_set_src(eth_icon, &image_eth);
  lv_obj_align(eth_icon, LV_ALIGN_TOP_LEFT, 20, 105);

  label_eth = lv_label_create(scr);
  lv_label_set_text(label_eth, "ETH : $---");
  lv_obj_set_style_text_font(label_eth, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label_eth, lv_color_white(), 0);
  lv_obj_align_to(label_eth, eth_icon, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

  // TAO
  lv_obj_t* tao_icon = lv_image_create(scr);
  lv_image_set_src(tao_icon, &image_tao);
  lv_obj_align(tao_icon, LV_ALIGN_TOP_LEFT, 20, 140);

  label_tao = lv_label_create(scr);
  lv_label_set_text(label_tao, "TAO : $---");
  lv_obj_set_style_text_font(label_tao, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label_tao, lv_color_white(), 0);
  lv_obj_align_to(label_tao, tao_icon, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

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

  if (millis() - lastUpd > 300000) {
    lastUpd = millis();
    get_crypto_data();
  }
}
