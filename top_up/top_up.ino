#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode card_status;

bool awaitingTopUp = false;
String currentPlate = "";
long currentBalance = 0;
String cardUID = "";
unsigned long inputStartTime = 0;
const unsigned long INPUT_TIMEOUT = 30000;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  Serial.println(F("==== TOP-UP MODE RFID ===="));
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
  if (!awaitingTopUp) {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

    cardUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) cardUID += "0";
      cardUID += String(mfrc522.uid.uidByte[i], HEX);
    }
    cardUID.toUpperCase();

    currentPlate = readBlockData(2, "Car Plate");
    String balanceStr = readBlockData(4, "Balance");

    if (currentPlate.startsWith("[") || balanceStr.startsWith("[")) {
      // Serial.println(F("⚠ Invalid card data. Try again."));
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(2000);
      return;
    }

    currentBalance = balanceStr.toInt();
    if (currentBalance < 0) {
      // Serial.println(F("⚠ Invalid balance value."));
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(2000);
      return;
    }

    Serial.print(F("Plate: "));
    Serial.println(currentPlate);
    Serial.print(F("Current Balance: "));
    Serial.println(currentBalance);
    Serial.println(F("Enter top-up amount:"));
    awaitingTopUp = true;
    inputStartTime = millis();
  }

  if (awaitingTopUp && Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    long topUpAmount = input.toInt();

    if (topUpAmount <= 0 || input.length() == 0 || (input == "0" && input != "0")) {
      Serial.println(F("[ERROR] Invalid top-up amount. Try again."));
      awaitingTopUp = false;
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(2000);
      return;
    }

    long newBalance = currentBalance + topUpAmount;
    Serial.print(F("[WRITING] New balance "));
    Serial.print(newBalance);
    Serial.println(F(" to card..."));
    bool writeSuccess = writeBlockData(4, String(newBalance));
    if (!writeSuccess) {
      // Serial.println(F("[ERROR] Failed to write to block 4. Trying block 1..."));
      writeSuccess = writeBlockData(1, String(newBalance));
    }

    if (writeSuccess) {
      Serial.println(F("✅ Balance updated on card."));
    } else {
      Serial.println(F("⚠ Failed to add balance to the card."));
    }

    Serial.print(F("DATA:"));
    Serial.print(currentPlate);
    Serial.print(F(","));
    Serial.print(newBalance);
    Serial.print(F(","));
    Serial.println(cardUID);
    Serial.println(F("DONE"));

    awaitingTopUp = false;
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(2000);
  }

  if (awaitingTopUp && millis() - inputStartTime > INPUT_TIMEOUT) {
    Serial.println(F("[TIMEOUT] No input received. Resetting."));
    awaitingTopUp = false;
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(2000);
  }
}

String readBlockData(byte blockNumber, String label) {
  byte buffer[18];
  byte bufferSize = sizeof(buffer);
  card_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));
  if (card_status != MFRC522::STATUS_OK) {
    // Serial.print(F("❌ Auth failed for "));
    // Serial.print(label);
    // Serial.println(F(":"));
    return "[Auth Fail]";
  }
  card_status = mfrc522.MIFARE_Read(blockNumber, buffer, &bufferSize);
  if (card_status != MFRC522::STATUS_OK) {
    // Serial.print(F("❌ Read failed for "));
    // Serial.print(label);
    // Serial.println(F(":"));
    return "[Read Fail]";
  }
  String data = "";
  for (uint8_t i = 0; i < 16; i++) data += (char)buffer[i];
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
    // Serial.println(F(":"));
    return false;
  }
  card_status = mfrc522.MIFARE_Write(blockNumber, buffer, 16);
  if (card_status != MFRC522::STATUS_OK) {
    // Serial.print(F("❌ Write failed for block "));
    // Serial.print(blockNumber);
    // Serial.println(F(":"));
    return false;
  }
  return true;
}