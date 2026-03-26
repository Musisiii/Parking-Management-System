#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN  9 
#define SS_PIN   10  

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode card_status;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println(F("==== CARD REGISTRATION ===="));
  Serial.println(F("Place your RFID card near the reader..."));
  Serial.println();
}

void loop() {
  // Set default key
  for(byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Wait for a card
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  Serial.println(F("📶 RFID Card detected!"));

  // Prompt user for car plate
  Serial.println(F("Enter car plate:"));
  while (!Serial.available());
  String carPlate = Serial.readStringUntil('\n');
  carPlate.trim(); // Remove any trailing whitespace

  // Prompt user for balance
  Serial.println(F("Enter balance:"));
  while (!Serial.available());
  String balance = Serial.readStringUntil('\n');
  balance.trim(); // Remove any trailing whitespace

  // Convert Strings to byte arrays for writing
  byte carPlateBuff[16];
  byte balanceBuff[16];
  stringToByteArray(carPlate, carPlateBuff, 16);
  stringToByteArray(balance, balanceBuff, 16);

  // Define RFID data blocks
  byte carPlateBlock = 2;
  byte balanceBlock = 4;

  // Write to RFID
  writeBytesToBlock(carPlateBlock, carPlateBuff);
  writeBytesToBlock(balanceBlock, balanceBuff);

  // // Loop until both blocks are successfully written
  // while (!carPlateWritten || !balanceWritten) {
  //   if (!carPlateWritten) {
  //     carPlateWritten = writeBytesToBlock(carPlateBlock, carPlateBuff);
  //     if (!carPlateWritten) {
  //       Serial.println(F("❌ Failed to write car plate. Retrying..."));
  //     }
  //   }

  //   if (!balanceWritten) {
  //     balanceWritten = writeBytesToBlock(balanceBlock, balanceBuff);
  //     if (!balanceWritten) {
  //       Serial.println(F("❌ Failed to write balance. Retrying..."));
  //     }
  //   }
  // }

  // // Ask user whether to continue
  // Serial.println(F("Would you like to continue? (y/n):"));
  // while (!Serial.available());
  // String userResponse = Serial.readStringUntil('\n');
  // userResponse.trim(); // Remove any trailing whitespace

  // if (userResponse.equalsIgnoreCase("n")) {
  //   Serial.println(F("Exiting program..."));
  //   while (true); // Stop the loop
  // }

  // Cleanup
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(2000);
}

// Convert a String to a byte array with padding
void stringToByteArray(String str, byte* buffer, byte maxLen) {
  for (byte i = 0; i < maxLen; i++) {
    buffer[i] = (i < str.length()) ? str[i] : ' '; // Pad with spaces if the string is shorter
  }
}

// Clear Serial input buffer
void flushSerial() {
  while (Serial.available()) {
    Serial.read();
  }
}

// Authenticate and write 16 bytes to a block
void writeBytesToBlock(byte block, byte buff[]) {
  card_status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));

  if (card_status != MFRC522::STATUS_OK) {
    return;
  } else {
    Serial.println(F("🔓 Authentication successful."));
  }

  Serial.print(F("🔄 Writing data to block "));
  Serial.println(block);

  card_status = mfrc522.MIFARE_Write(block, buff, 16);
  if (card_status != MFRC522::STATUS_OK) {
  } else {
    Serial.println(F("✅ Data written successfully."));
  }
}