#include <Adafruit_DS3502.h>

Adafruit_DS3502 ds3502_lower_a = Adafruit_DS3502();
Adafruit_DS3502 ds3502_lower_b = Adafruit_DS3502();
Adafruit_DS3502 ds3502_upper_a = Adafruit_DS3502();
Adafruit_DS3502 ds3502_upper_b = Adafruit_DS3502();

#define WIPER_VALUE_PIN A0

// const uint8_t stepVal = 2;
const int stepMs = 100;

const uint8_t scalePotMin = 0; //48;
const uint8_t scalePotMax = 127; //80;

const int DISTRIB_SIZE = 8;
const int stepRelCentreLower[] = {
  -2,
  -1,
  -1,
  -1,
  0,
  1,
  1,
  2,
};

const int stepRelCentreUpper[] = {
  -32,
  -16,
  -8,
  -8,
  0,
  8,
  16,
  32,
};

int wiperval[] = {63, 63, 63, 63};

void setup() {
  Serial.begin(115200);
  // Wait until serial port is opened
  while (!Serial) { delay(1); }

  Serial.println("Adafruit DS3502 Test");

  // 3 / Blue : top in-out
  // 2 / Grey : top left-right
  // 1 / Yellow : bottom in-out
  // 0 / Black : bottom left-right

  bool beginSuccess[4];
  beginSuccess[0] = ds3502_lower_a.begin(DS3502_I2CADDR_DEFAULT + 0);
  beginSuccess[1] = ds3502_lower_b.begin(DS3502_I2CADDR_DEFAULT + 1);
  beginSuccess[2] = ds3502_upper_a.begin(DS3502_I2CADDR_DEFAULT + 2);
  beginSuccess[3] = ds3502_upper_b.begin(DS3502_I2CADDR_DEFAULT + 3);

  bool initOk = true;
  for (int i = 0; i < 4; i++) {
    Serial.println(beginSuccess[i]);
    initOk = initOk && beginSuccess[i];
  }
  if (!initOk) {
    Serial.println("Couldn't find at least one DS3502 chip");
    // while (1);
  }
  else { Serial.println("Found all 4x DS3502 chips"); }

  Serial.println("Setting init pos and waiting 10secs...");
  ds3502_lower_a.setWiper(wiperval[0]);
  ds3502_lower_b.setWiper(wiperval[1]);
  ds3502_upper_a.setWiper(wiperval[2]);
  ds3502_upper_b.setWiper(wiperval[3]);

  delay(10000);

}

void loop() {
  Serial.print("wiperval = {");
  for (int i = 0; i < 4; i++) {
    Serial.print(wiperval[i]);
    if (i < 3) { Serial.print(","); }
  }
  Serial.print("} : ");
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 16; j++) {
      Serial.print(abs(wiperval[i] - j*8) < 4 ? '*' : '.');
    }
    if (i < 3) { Serial.print(" : "); }
  }
  Serial.print("\n");

  ds3502_lower_a.setWiper(wiperval[0]);
  ds3502_lower_b.setWiper(wiperval[1]);
  ds3502_upper_a.setWiper(wiperval[2]);
  ds3502_upper_b.setWiper(wiperval[3]);

  delay(stepMs);

  // Random walk!
  for (int i = 0; i < 4; i++) {
    int outcome = rand() % DISTRIB_SIZE;
    int curRelCentre = wiperval[i] - 63;
    const int* stepRelCentre = (i < 2) ? stepRelCentreLower : stepRelCentreUpper;
    wiperval[i] += (curRelCentre > 0) ? stepRelCentre[outcome] : -stepRelCentre[outcome];
    wiperval[i] = max(scalePotMin, min(wiperval[i], scalePotMax));
  }
}
