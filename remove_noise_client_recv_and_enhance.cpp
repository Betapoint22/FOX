#include <WiFi.h>
#include <WiFiUdp.h>
#include "driver/i2s.h"

const char *ssid = "YourWiFiSSID";
const char *password = "YourWiFiPassword";
const uint16_t localPort = 12345;

WiFiUDP udp;
uint8_t audioBuffer[512]; // Buffer for audio data

// Parameters for noise gate and audio gain
const int NOISE_THRESHOLD = 10; // Adjust according to the noise level
const float GAIN_FACTOR = 1.5; // Adjust for desired audio enhancement level

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Start UDP
  udp.begin(localPort);

  // Initialize I2S for audio output
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // Master, transmitter
      .sample_rate = 44100,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_24BIT, // 24-bit
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // 2-channel
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 64
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, NULL); // Use default pins for I2S

  Serial.println("Setup complete");
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Receive audio data
    int bytesRead = udp.read(audioBuffer, sizeof(audioBuffer));

    // Apply noise reduction and audio enhancement
    for (int i = 0; i < bytesRead; i++) {
      // Apply noise gate
      if (abs(audioBuffer[i] - 128) < NOISE_THRESHOLD) {
        audioBuffer[i] = 128; // Set to neutral value (no change)
      }
      // Apply audio gain
      audioBuffer[i] = min(255, max(0, (int)(GAIN_FACTOR * (audioBuffer[i] - 128) + 128)));
    }

    // Write audio data to I2S output
    size_t bytesWritten;
    i2s_write(I2S_NUM_0, audioBuffer, bytesRead, &bytesWritten, portMAX_DELAY);
  }
}
