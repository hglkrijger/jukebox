#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>

#define PN532_IRQ   2
#define PN532_RESET 3
#define BUTTON      5
#define LED         7
#define DF_RX      10
#define DF_TX      11

#define VOL_UP     97
#define VOL_DN     98
#define STOP       99
 
int currentButtonState = 0;
int previousButtonState = 0;
int cardPage = 4;
int volume = 15;

bool stateChanged = false;
bool jukeboxOn = false;
bool isPlaying = false;

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
SoftwareSerial mySoftwareSerial(DF_RX, DF_TX);
DFRobotDFPlayerMini myDFPlayer;

/* button helpers */

void readButton()
{
  currentButtonState = digitalRead(BUTTON);

  if (previousButtonState != currentButtonState)
  {
    stateChanged = true;
    previousButtonState = currentButtonState;
  }
  else
  {
    stateChanged = false;
  }
}

bool isButtonPushed()
{
  return stateChanged && currentButtonState == HIGH;
}

bool isButtonReleased()
{
  return stateChanged && currentButtonState == LOW;
}

void lightOn()
{
  digitalWrite(LED, HIGH);
}

void lightOff()
{
  digitalWrite(LED, LOW);
}

void slowFlash()
{
  Serial.println("slow flash");
  flash(3, 500);
}

void fastFlash()
{
  Serial.println("fast flash");
  flash(6, 200);
}

void flash(int count, int speed)
{
  for (int i=0; i<count; i++)
  {
    lightOn();
    delay(speed);
    lightOff();
    delay(speed);
  }
}

void lightFlip()
{
  delay(350);
  digitalWrite(LED, !digitalRead(LED));
}

/* end button helpers */

/* nfc helpers */

void readTag()
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
    
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    if (uidLength == 7)
    {
      uint8_t data[32];
      success = nfc.ntag2xx_ReadPage(cardPage, data);

      if (success) 
      {
        Serial.print("tag id: ");
        int tagId = (int)data[0];
        Serial.println(tagId);

        if (tagId == STOP)
        {
          pause();
        } 
        else if (tagId == VOL_UP)
        {
          volume++;
          setVolume();
        } 
        else if (tagId == VOL_DN)
        {
          volume--;
          setVolume();
        } 
        else if (!isPlaying) {
          play(tagId);
        }
      }
      else
      {
        Serial.println("Unable to read the requested page!");
      }
    }
    else
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
  }
}

/* end nfc helpers */

/* mp3 helpers */

void setVolume()
{
  volume = max(0, min(30, volume));

  Serial.print("volume: ");
  Serial.println(volume);
  myDFPlayer.volume(volume);
}

void play(int song)
{
  isPlaying = true;
  myDFPlayer.play(song);
  fastFlash();
}

void pause()
{
  isPlaying = false;
  myDFPlayer.pause();
  if (currentButtonState == HIGH)
  {
    lightOn();
  } else {
    lightOff();
  }
}

void checkPlayState()
{
  if (myDFPlayer.available()) {
    uint8_t type = myDFPlayer.readType();
    int value = myDFPlayer.read();
    
    switch (type) {
      case DFPlayerPlayFinished:
        pause();
        break;

      default:
        break;
    }
  }
}

/* end mp3 helpers */

void setup() {
  Serial.begin(9600);
  
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT);
  slowFlash();

  Serial.println("setup nfc shield");
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

  Serial.println("setup dfplayer");
  mySoftwareSerial.begin(9600);
  while (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("dfplayer not ready");
    delay(5000);
    fastFlash();
  }
  int fileCount = myDFPlayer.readFileCounts();
  Serial.print("file count: ");
  Serial.println(fileCount);

  setVolume();
  Serial.println("dfplayer ready");
  
  Serial.println("ready");
}

void loop() {
  readButton();
  
  if (isButtonPushed())
  {
    Serial.println("button pushed");
    lightOn();
    jukeboxOn = true;
  }
  
  if (isButtonReleased())
  {
    Serial.println("button released");
    lightOff();
    jukeboxOn = false;
    pause();
  }

  if (jukeboxOn)
  {
    readTag();
    checkPlayState();
    if (isPlaying)
    {
      lightFlip();
    }
  }

  delay(500);
}
