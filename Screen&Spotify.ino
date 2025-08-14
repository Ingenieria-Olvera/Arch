#include <WiFi.h>
#include <SpotifyEsp32.h>
#include <U8g2lib.h>
#include <BluetoothA2DPSink.h>
#include <AudioTools.h>
#include <Config.h>
#include <Arduino.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/12, /* dc=*/13, /* reset=*/5);

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

Spotify sp(client_id, client_secret);

#define R_BUTTON 21
#define L_BUTTON 5

bool is_connected_to_bl = false;
bool spotify_authed = false;

void connect_wifi();
void authenticate_spotify();

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(R_BUTTON, INPUT_PULLUP);
  pinMode(L_BUTTON, INPUT_PULLUP);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(32, 32, "Welcome to");
  u8g2.drawStr(42, 42, "ARCH");
  u8g2.sendBuffer();
  delay(1500);

  connect_wifi();
  authenticate_spotify();

  if (!spotify_authed) {
    u8g2.clearBuffer();
    u8g2.drawStr(10, 32, "Spotify Auth Failed");
    u8g2.drawStr(10, 50, "Restart device.");
    u8g2.sendBuffer();
    while (true);  //  SPOTIFY CHECK
  }

  // DAC SETUP
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = 14; // BCK
  cfg.pin_ws = 15; //  LRC
  cfg.pin_data = 22; //  DIN => MOSI
  i2s.begin(cfg);

  a2dp_sink.start("ARCH");

  // Bluetooth Connection
  while (!a2dp_sink.is_avrc_connected()) {
    u8g2.clearBuffer();
    u8g2.drawStr(20, 32, "Connect to ARCH");
    u8g2.drawStr(10, 50, "via Bluetooth.");
    u8g2.sendBuffer();
    delay(1000);
  }

  u8g2.clearBuffer();
  u8g2.drawStr(10, 32, "All Set!");
  u8g2.drawStr(10, 50, "Playing soon...");
  u8g2.sendBuffer();
  delay(2000);
}

void loop() {
  if (!spotify_authed) return;

  sp.handle_client();

  String song_name = sp.current_track_name();

  if (song_name.length() == 0) {
    song_name = "Unknown Track";
  }

  char song_name_array[64];
  song_name.toCharArray(song_name_array, sizeof(song_name_array));

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(30, 32, "You're Playing");
  u8g2.drawStr(0, 50, song_name_array);
  u8g2.sendBuffer();

  delay(10000);  
}

void connect_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid_h, password_h);
  delay(500);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nHome WiFi failed, trying secondary...");
    WiFi.begin(ssid_s, password_s);

    start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to any WiFi. Halting.");
    u8g2.clearBuffer();
    u8g2.drawStr(10, 32, "WiFi Failed.");
    u8g2.drawStr(10, 50, "Restart device.");
    u8g2.sendBuffer();
    while (true);  // If wifi fails then try again type
  }

  Serial.println("\nConnected to WiFi");
}

void authenticate_spotify() {
  Serial.println("Authenticating with Spotify.");
  unsigned long startAttempt = millis();

  while (!sp.is_auth() && millis() - startAttempt < 10000) {  
    sp.handle_client();
  }

  if (!sp.is_auth()) {
    Serial.println("Spotify authenticatoin failed");
  } else {
    Serial.println("Authenticated with Spotify");
  }
}

/*
void authenticate_spotify() {
  Serial.println("Authenticating with Spotify.");
  unsigned long startAttempt = millis();

  while (!sp.is_auth() && millis() - startAttempt < 10000) {  // 10 second timeout
    sp.handle_client();
  }

  if (!sp.is_auth()) {
    Serial.println("Spotify authenticatoin failed");
  } else {
    Serial.println("Authenticated with Spotify");
  }
}
*/
