#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WIFI.h>
#include <HTTPClient.h>
#include <Servo.h>

#define RST_PIN 22 // Configurable, see typical pin layout above
#define SS_1_PIN 21
#define SS_2_PIN 4
#define NR_OF_READERS 2

Servo entryGate;
Servo exitGate;
// int entryGatePos = 0;

byte ssPins[] = {SS_1_PIN, SS_2_PIN};
MFRC522 mfrc522[NR_OF_READERS];

/*Put your SSID & Password*/
const char *ssid = "Dulanjana";    // Enter SSID here
const char *password = "password"; // Enter Password here

// Your Domain name with URL path or IP address with path
String baseUrl = "https://smart-car-park-production.up.railway.app/data/";

void cycleGate(int id)
{
  Servo gate;
  if (id == 0)
  {
    gate = entryGate;
  }
  else
  {
    gate = exitGate;
  }

  int pos = 0;
  for (pos = 0; pos <= 90; pos += 1)
  {
    gate.write(pos); // tell servo to go to position in variable 'pos'
    delay(15);       // waits 15ms for the servo to reach the position
  }
  delay(1000);

  for (pos = 90; pos >= 0; pos -= 1)
  {
    gate.write(pos);
    delay(15);
  }
}

String byteToHexString(byte byteValue)
{
  char hexChars[3];
  sprintf(hexChars, "%02X", byteValue);
  return String(hexChars);
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
String dump_byte_array(byte *buffer, byte bufferSize)
{
  String uid = "";
  for (byte i = 0; i < bufferSize; i++)
  {
    String leading = buffer[i] < 0x10 ? "-0" : "-";
    String hexString = byteToHexString(buffer[i]);
    uid = uid + leading + hexString;
  }
  return uid;
}

// void dump_byte_array(byte *buffer, byte bufferSize)
// {
//   for (byte i = 0; i < bufferSize; i++)
//   {
//     Serial.print(buffer[i] < 0x10 ? " 0" : " ");
//     Serial.print(buffer[i], HEX);
//   }
// }

void setup()
{
  Serial.begin(115200); // Initialize serial communications with the PC
  // Servo init
  entryGate.attach(13);
  exitGate.attach(13);

  // WIFI CONNECTION

  WiFi.begin(ssid, password);

  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  while (!Serial)
    ;          // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin(); // Init SPI bus
  Serial.println();

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++)
  {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Init each MFRC522 card
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
  }
}

void loop()
{

  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++)
  {
    // Look for new cards

    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial())
    {
      HTTPClient http;

      Serial.print(F("Reader "));
      Serial.print(reader);
      cycleGate(reader); // cycle the motor
      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F(": Card UID:"));
      // dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      String uid = dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      Serial.print(uid);

      String serverPath = baseUrl + "?rfid=" + uid + "&reader=" + reader;
      Serial.println(serverPath);
      http.begin(serverPath.c_str());
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0)
      {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else
      {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();

      Serial.println();
      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = mfrc522[reader].PICC_GetType(mfrc522[reader].uid.sak);
      Serial.println(mfrc522[reader].PICC_GetTypeName(piccType));

      // Halt PICC
      mfrc522[reader].PICC_HaltA();
      // Stop encryption on PCD
      mfrc522[reader].PCD_StopCrypto1();
    }
  }
}
