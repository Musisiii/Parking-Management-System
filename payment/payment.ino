#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode card_status;

bool awaitingUpdate = false;
bool sentReady = false;
String currentPlate = "";
long currentBalance = 0;
unsigned long readySentTime = 0;
const unsigned long RESPONSE_TIMEOUT = 15000; // 15 seconds

void setup() {
  Serial.begin(9600);
  SPI.begin();
  delay(10);
  mfrc522.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println(F("==== PAYMENT MODE RFID ===="));
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println(F("❌ ERROR: MFRC522 not detected."));
    while (true);
  } else {
    Serial.print(F("✅ MFRC522 detected, firmware version: 0x"));
    Serial.println(version, HEX);
  }
  Serial.println(F("Place your card near the reader..."));
}

void loop() {
  if (!awaitingUpdate) {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

    currentPlate = readBlockData(2, "Car Plate");
    String balanceStr = readBlockData(4, "Balance");

    if (currentPlate.startsWith("[") || balanceStr.startsWith("[")) {
    //   Serial.println(F("⚠ Invalid card data. Try again."));
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(2000);
      return;
    }

    currentBalance = balanceStr.toInt();
    if (currentBalance <= 0) {
      Serial.println(F("⚠ Invalid balance value."));
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(2000);
      return;
    }

    Serial.print(F("DATA:"));
    Serial.print(currentPlate);
    Serial.print(F(","));
    Serial.println(currentBalance);

    awaitingUpdate = true;
    sentReady = false;
  }

  if (awaitingUpdate && !sentReady) {
    Serial.println(F("READY"));
    sentReady = true;
    readySentTime = millis();
  }

  if (awaitingUpdate && sentReady) {
    if (millis() - readySentTime > RESPONSE_TIMEOUT) {
      Serial.println(F("[TIMEOUT] No response from PC. Resetting."));
      awaitingUpdate = false;
      sentReady = false;
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(1000);
      return;
    }

    if (Serial.available()) {
      String response = Serial.readStringUntil('\n');
      response.trim();
      Serial.print(F("[RECEIVED]: "));
      Serial.println(response);

      if (response == "I") {
        Serial.println(F("[DENIED] Insufficient balance."));
      } else {
        long newBalance = response.toInt();
        if (newBalance < 0 || response.length() == 0 || response == "0" && response != "0") {
          Serial.println(F("[ERROR] Invalid balance received."));
        } else {
          Serial.println(F("[WRITING] New balance to card..."));
          if (writeBlockData(4, String(newBalance))) {
            Serial.println(F("DONE"));
            Serial.print(F("[UPDATED] New Balance: "));
            Serial.println(newBalance);
          } else {
            Serial.println(F("[ERROR] Failed to write new balance. Trying block 1..."));
            if (writeBlockData(1, String(newBalance))) {
              Serial.println(F("DONE"));
              Serial.print(F("[UPDATED] New Balance on block 1: "));
              Serial.println(newBalance);
            } else {
              Serial.println(F("[ERROR] Failed to write to block 1."));
            }
          }
        }
      }
      awaitingUpdate = false;
      sentReady = false;
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(2000);
    }
  }
}

String readBlockData(byte blockNumber, String label) {
  byte buffer[18];
  byte bufferSize = sizeof(buffer);

  card_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));
  if (card_status != MFRC522::STATUS_OK) {
    // Serial.print(F("❌ Auth failed for "));
    // Serial.print(label);
    // Serial.print(F(" (block "));
    // Serial.print(blockNumber);
    // Serial.println(F("): "));
    return "[Auth Fail]";
  }

  card_status = mfrc522.MIFARE_Read(blockNumber, buffer, &bufferSize);
  if (card_status != MFRC522::STATUS_OK) {
    // Serial.print(F("❌ Read failed for "));
    // Serial.print(label);
    // Serial.print(F(" (block "));
    // Serial.print(blockNumber);
    // Serial.println(F("): "));
    return "[Read Fail]";
  }

  String data = "";
  for (uint8_t i = 0; i < 16; i++) {
    data += (char)buffer[i];
  }
  data.trim();
  return data;
}

bool writeBlockData(byte blockNumber, String data) {
  byte buffer[16];
  data.trim();
  while (data.length() < 16) data += ' ';
  data.substring(0, 16).getBytes(buffer, 16);

  card_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));
  if (card_status != MFRC522::STATUS_OK) {
    // Serial.print(F("❌ Auth failed for block "));
    // Serial.print(blockNumber);
    // Serial.println(F(": "));
    return false;
  }

  card_status = mfrc522.MIFARE_Write(blockNumber, buffer, 16);
  if (card_status != MFRC522::STATUS_OK) {
    // Serial.print(F("❌ Write failed for block "));
    // Serial.print(blockNumber);
    // Serial.println(F(": "));
    return false;
  }
  return true;
}