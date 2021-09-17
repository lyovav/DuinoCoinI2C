/*
  DoinoCoin_Wire.ino
  JK-Rolling

  ESP8266
  Code concept taken from duino-coin and ricaun
*/

#include <Wire.h>
#include <DuinoCoin.h>        // https://github.com/ricaun/arduino-DuinoCoin
#include <StreamString.h>     // https://github.com/ricaun/StreamJoin
/* If during compilation the line below causes a
"fatal error: Crypto.h: No such file or directory"
message to occur; it means that you do NOT have the
latest version of the ESP8266/Arduino Core library.
To install/upgrade it, go to the below link and
follow the instructions of the readme file:
https://github.com/esp8266/Arduino */
#include <Crypto.h>  // experimental SHA1 crypto library
using namespace experimental::crypto;

// comment this line to enable self-assign address
// not reliable, use at own risk
// update the address manually
#define I2CS_ADDR 1

#ifdef ESP01
  // ESP-01
  #define SDA 0 // GPIO0
  #define SCL 2 // GPIO2
#else
  // ESP8266
  #define SDA 4 // D2
  #define SCL 5 // D1
#endif

#define WIRE_CLOCK 10000
#define WIRE_MAX 32

byte i2c = 1;
StreamString bufferReceive;
StreamString bufferRequest;
String chipID = "";

void DuinoCoin_setup()
{
  //pinMode(SCL, INPUT_PULLUP);
  //pinMode(SDA, INPUT_PULLUP);
  // delay still needed to set reliable i2c addr
  unsigned long time = getTrueRotateRandomByte() * 1000 + getTrueRotateRandomByte();
  Serial.println("random_time: "+ String(time));
  delayMicroseconds(time);
  
  #ifndef I2CS_ADDR
  Wire.begin();
  for (int address = 1; address < WIRE_MAX; address++ )
  {
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();
    if (error != 0)
    {
      i2c = address;
      break;
    }
  }
  #else
  i2c = I2CS_ADDR;
  #endif

  Wire.begin(SDA, SCL, i2c);
  //Wire.setClockStretchLimit(5000000L);
  Wire.setClock(WIRE_CLOCK);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  Serial.print(F("Wire Address: "));
  Serial.println(i2c);
  
  chipID = String(ESP.getChipId(), HEX);
}


void receiveEvent(int howMany) {
  if (howMany == 0)
  {
    return;
  }
  while (Wire.available()) {
    char c = Wire.read();
    bufferReceive.write(c);
  }
}

void requestEvent() {
  char c = '\n';
  if (bufferRequest.available() > 0 && bufferRequest.indexOf('\n') != -1)
  //if (bufferRequest.length() > 0)
  {
    c = bufferRequest.read();
  }
  Wire.write(c);
}

bool DuinoCoin_loop()
{
  if (bufferReceive.available() > 0 && bufferReceive.indexOf('\n') != -1) {

    Serial.print(F("Job: "));
    Serial.println(bufferReceive);
    
    // Read last block hash
    String lastblockhash = bufferReceive.readStringUntil(',');
    lastblockhash = str_GetAlpha(lastblockhash);
    // Read expected hash
    String newblockhash = bufferReceive.readStringUntil(',');
    newblockhash = str_GetAlpha(newblockhash);
    newblockhash.toUpperCase();
    // Read difficulty
    unsigned int difficulty = bufferReceive.readStringUntil('\n').toInt() * 100 + 1;
    while (bufferReceive.available()) bufferReceive.read();
    // Start time measurement
    unsigned long startTime = micros();
    max_micros_elapsed(startTime, 0);
    // Call DUCO-S1A hasher
    unsigned int ducos1result = 0;
    for (unsigned int duco_numeric_result = 0; duco_numeric_result < difficulty; duco_numeric_result++) {
      // Difficulty loop - 10.4KH/s
      String result = SHA1::hash(lastblockhash + String(duco_numeric_result));
      if (result == newblockhash) {
        ducos1result = duco_numeric_result;
        break;
      }
      if (max_micros_elapsed(micros(), 5000))
        handleSystemEvents();
    }
    // 8.06KH/s
    //ducos1result = Ducos1a.work(lastblockhash, newblockhash, difficulty);
    // End time measurement
    unsigned long endTime = micros();
    // Calculate elapsed time
    unsigned long elapsedTime = endTime - startTime;
    // Send result back to the program with share time
    while (bufferRequest.available()) bufferRequest.read();
    bufferRequest.print(String(ducos1result) + "," + String(elapsedTime) + "," + String(get_DUCOID()) + "\n");
    
    Serial.print(F("Done "));
    Serial.print(String(ducos1result) + "," + String(elapsedTime) + "," + String(get_DUCOID()) + "\n");

    return true;
  }
  return false;
}

String DuinoCoin_response()
{
  return bufferRequest;
}

String get_DUCOID() {
  return chipID;
}
