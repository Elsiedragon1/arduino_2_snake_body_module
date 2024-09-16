#include <Adafruit_DS3502.h>
#include <ModbusRTUSlave.h>

Adafruit_DS3502 ds3502_lower_a = Adafruit_DS3502(); //  Bottom Stage - Left/Right
Adafruit_DS3502 ds3502_lower_b = Adafruit_DS3502(); //  Bottom Stage - Front/Back
Adafruit_DS3502 ds3502_upper_a = Adafruit_DS3502(); //  Top Stage - Left/Right
Adafruit_DS3502 ds3502_upper_b = Adafruit_DS3502(); //  Top Stage - Front/Back

// const uint8_t stepVal = 2;
const int stepMs = 100;

const uint8_t scalePotMin = 0; //48;
const uint8_t scalePotMax = 127; //80;

//  Modbus ID
const uint16_t id = 2;
//  Modbus Serial configuration
const uint32_t baud = 115200;
const uint8_t config = SERIAL_8E1;
const uint16_t bufferSize = 256;
const uint8_t dePin = A0;

const uint8_t holdingRegisters = 1;

uint8_t buffer[bufferSize];
ModbusRTUSlave modbus(Serial, buffer, bufferSize, dePin);

//  Modes
//  0   -   Stationary - Reset to mean value!
//  1   -   Animating!
//  2   -   Move to central value for calibration
//  3   -   Smaller Animation

//  Animation
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

const int stepRelCentreUpperSlow[] = {
    -4,
    -2,
    -1,
    -1,
    0,
    1,
    2,
    4
};

//  0 - Stationary - MEAN position      1 - Animating!      2 - Centered Position
uint8_t mode = 0;

//  Pot values!
//  The center position should be straight up! Good for when the snakes are lowered
//  The mean position is where the snakes will do their random walk from ... change these values to move the bias the snakebodies have in their animation.
//  Testing needed to see what the mean should be set to!
uint8_t centrePosition[] = { 63, 63, 63, 63 };
uint8_t meanPosition[] = { 63, 50, 63, 83 };

int wiperval[] = { 63, 63, 63, 63 };

int32_t readHoldingRegister(uint16_t address)
{
    if (address >= 0 && address < holdingRegisters)
    {
        return mode;
    }
    else
    {
        return false;
    }
}

int16_t writeHoldingRegister(uint16_t address, uint16_t data)
{
    if (address >= 0 && address < holdingRegisters)
    {
        if (data >= 0 && data <= 3)
        {
            mode = data;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

const bool enableSerialDebug = false;

//  Timing
uint32_t currentTick = 0;
uint32_t lastTick = 0;
uint32_t interval = 1000/10;    //  10fps - Do not change this without also changing the stepRelCentreUpper/Lower values!!!

bool ledState = true;

void setup()
{
    if (enableSerialDebug)
    {
        Serial.begin(115200);
        // Wait until serial port is opened
        while (!Serial)
        {
            delay(1);
        }
    }
    else
    {
        // RS485 Enable / Disable
        pinMode(dePin, OUTPUT);

        //  Setup Modbus
        Serial.begin(baud, config);
        modbus.begin(id, baud, config);
        modbus.configureHoldingRegisters(holdingRegisters, readHoldingRegister, writeHoldingRegister);
    }
    
    if (enableSerialDebug) Serial.println("Adafruit DS3502 Test");

    pinMode(OUTPUT, LED_BUILTIN);

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
    for (int i = 0; i < 4; i++)
    {
        if (enableSerialDebug) Serial.println(beginSuccess[i]);
        initOk = initOk && beginSuccess[i];
    }
    if (!initOk)
    {
        if (enableSerialDebug) Serial.println("Couldn't find at least one DS3502 chip");
        // while (1);
    }
    else
    {
        if (enableSerialDebug) Serial.println("Found all 4x DS3502 chips");
    }

    if (enableSerialDebug) Serial.println("Setting init pos and waiting 10secs...");
    ds3502_lower_a.setWiper(wiperval[0]);
    ds3502_lower_b.setWiper(wiperval[1]);
    ds3502_upper_a.setWiper(wiperval[2]);
    ds3502_upper_b.setWiper(wiperval[3]);

    delay(3000);
}

void loop()
{
    currentTick = millis();
    modbus.poll();

    if (currentTick - lastTick > interval)
    {
        if (enableSerialDebug)
        {
            printDebugInfo();
        }

        switch( mode )
        {
            case 0:
                for (uint8_t i = 0; i < 4; i++)
                {
                    wiperval[i] = meanPosition[i];
                }
                break;
            case 1:
                // Random walk!
                for (int i = 0; i < 4; i++)
                {
                    int outcome = rand() % DISTRIB_SIZE;
                    int curRelCentre = wiperval[i] - meanPosition[i];
                    const int* stepRelCentre = (i < 2) ? stepRelCentreLower : stepRelCentreUpper;
                    wiperval[i] += (curRelCentre > 0) ? stepRelCentre[outcome] : -stepRelCentre[outcome];
                    wiperval[i] = max(scalePotMin, min(wiperval[i], scalePotMax));
                }
                break;
            case 2:
                for (uint8_t i = 0; i < 4; i++)
                {
                    wiperval[i] = centrePosition[i];
                }
                break;
            case 3:
                // Random walk - but Slow!
                for (int i = 0; i < 4; i++)
                {
                    int outcome = rand() % DISTRIB_SIZE;
                    int curRelCentre = wiperval[i] - meanPosition[i];
                    const int* stepRelCentre = (i < 2) ? stepRelCentreLower : stepRelCentreUpperSlow;
                    wiperval[i] += (curRelCentre > 0) ? stepRelCentre[outcome] : -stepRelCentre[outcome];
                    wiperval[i] = max(scalePotMin, min(wiperval[i], scalePotMax));
                }
                break;
            default:
                for (uint8_t i = 0; i < 4; i++)
                {
                    wiperval[i] = meanPosition[i];
                }
                break;
        }
        
        ds3502_lower_a.setWiper(wiperval[0]);
        ds3502_lower_b.setWiper(wiperval[1]);
        ds3502_upper_a.setWiper(wiperval[2]);
        ds3502_upper_b.setWiper(wiperval[3]);

        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);

        lastTick = currentTick;
    }
}

void printDebugInfo()
{
    Serial.print("wiperval = {");
    for (int i = 0; i < 4; i++)
    {
        Serial.print(wiperval[i]);
        if (i < 3) { Serial.print(","); }
    }
    Serial.print("} : ");
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            Serial.print(abs(wiperval[i] - j*8) < 4 ? '*' : '.');
        }
        if (i < 3)
        {
            Serial.print(" : ");
        }
    }
    Serial.print("\n");
}
