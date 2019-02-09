#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

int page=4;
uint8_t written=0;
String inString = "";
int cardData = 0;

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup(void) {
  Serial.begin(9600);
  Serial.println("Initializing..");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Did not find PN53x board");
    while (1);
  }
  
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  nfc.SAMConfig();

  nfc.setPassiveActivationRetries(0x01);

  Serial.println("Enter a number to write...");
}

void readSerial()
{
  char c = ' ';
  while (Serial.available() > 0) {
    int inChar = Serial.read();
    if (isDigit(inChar)) {
      // convert the incoming byte to a char and add it to the string:
      inString += (char)inChar;
    }
    // if you get a newline, print the string, then the string's value:
    if (inChar == '\n') {
      int intValue = inString.toInt();
      if (intValue < 255 && intValue > 0) {
        cardData = intValue;
        written = 0;
      } else {
        Serial.println("Invalid value");
      }
      Serial.print("Value: ");
      Serial.println(cardData);
      // clear the string for new input:
      inString = "";
    }
  }
}

void writeTag()
{
  if (written) {
    return;
  }
  
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    if (uidLength == 7)
    {
      uint8_t data[32];
      Serial.print("Writing: ");
      Serial.println(cardData);
      
      memset(data, 0, 4);

      data[0] = cardData;
      
      success = nfc.ntag2xx_WritePage(page, data);
      
      // Display the results, depending on 'success'
      if (success) 
      {
        Serial.println("Success");
        written = 1;
      }
      else
      {
        Serial.println("Unable to write to the requested page!");
      }    
    }
    else
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
  }
}

void readTag()
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    // Display some basic information about the card
    //Serial.println("Found an ISO14443A card");
    //Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    //Serial.print("  UID Value: ");
    //nfc.PrintHex(uid, uidLength);
    //Serial.println("");
    
    if (uidLength == 7)
    {
      uint8_t data[32];
      
      // We probably have an NTAG2xx card (though it could be Ultralight as well)
      //Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");    
      
      success = nfc.ntag2xx_ReadPage(page, data);
      
      // Display the current page number
      //Serial.print("PAGE ");
      //Serial.print(page);
      //Serial.print(":");

      // Display the results, depending on 'success'
      if (success) 
      {
        // Dump the page data
        //nfc.PrintHexChar(data, 4);

        Serial.print("Read value: ");
        Serial.println(data[0], DEC);
      }
      else
      {
        Serial.println("Unable to read the requested page!");
      }

      // Wait a bit before trying again
      Serial.println("\nEnter to write another tag");
      Serial.flush();
      while (!Serial.available());
      while (Serial.available()) { Serial.read(); }
      Serial.println("Ready");
    }
    else
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
  }
}

void loop(void) 
{
  readSerial();
    
  writeTag();

  readTag();
  
  delay(500);
}
