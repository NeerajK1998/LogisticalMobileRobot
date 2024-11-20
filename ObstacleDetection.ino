#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>
#include <SPI.h>
#include <mcp_can.h>

// MCP2515 CAN Module
MCP_CAN CAN0(10); // Set CS to pin 10

// I2C Settings for VL53L5CX Sensors
#define SDA_PIN 8
#define SCL_PIN 9

SparkFun_VL53L5CX myImager1, myImager2, myImager3, myImager4, myImager5;
int sensorAddresses[] = {0x30, 0x31, 0x32, 0x33, 0x34};
int sensorLPn[] = {43, 44, 47, 48, 45};
VL53L5CX_ResultsData measurementData1, measurementData2, measurementData3, measurementData4, measurementData5;

// Unique CAN IDs for ESP1
const uint16_t sensorBaseCanId = 0x100; // Sensor CAN IDs for ESP1
const uint16_t heartbeatCanId = 0x700; // Heartbeat CAN ID for ESP1

// Heartbeat Timer
unsigned long lastHeartbeatTime = 0;
const unsigned long heartbeatInterval = 1000; // 1-second interval

void setup()
{
    // Serial for Debugging
    Serial.begin(115200);

    // Initialize I2C for VL53L5CX
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(1000000);

    // Initialize Sensor Pins
    for (int i = 0; i < 5; i++)
    {
        pinMode(sensorLPn[i], OUTPUT);
        digitalWrite(sensorLPn[i], LOW);
    }
    delay(100);

    // Initialize Sensors
    for (int i = 0; i < 5; i++)
    {
        digitalWrite(sensorLPn[i], HIGH);
        delay(50);

        SparkFun_VL53L5CX &imager = getImager(i);

        if (!imager.begin())
        {
            Serial.print(F("Sensor "));
            Serial.print(i + 1);
            Serial.println(F(" not found. Check wiring. Freezing..."));
            while (1)
                ;
        }

        if (!imager.setAddress(sensorAddresses[i]))
        {
            Serial.print(F("Failed to set address for sensor "));
            Serial.print(i + 1);
            Serial.println(F(". Freezing..."));
            while (1)
                ;
        }

        imager.setResolution(8 * 8);
        imager.setRangingFrequency(15);
        imager.startRanging();
        delay(10);
    }

    // Initialize MCP2515 CAN Controller
    if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK)
        Serial.println("MCP2515 Initialized Successfully!");
    else
        Serial.println("Error Initializing MCP2515...");

    CAN0.setMode(MCP_NORMAL); // Set to normal mode
}

void loop()
{
    // Read and Transmit Sensor Data
    for (int i = 0; i < 5; i++)
    {
        SparkFun_VL53L5CX &imager = getImager(i);
        VL53L5CX_ResultsData &data = getData(i);

        if (imager.isDataReady())
        {
            if (imager.getRangingData(&data))
            {
                byte canData[8] = {0}; // Data array to hold obstacle zone values

                // Process and format data for CAN
                for (int zone = 24; zone <= 31; zone++)
                {
                    int distance = data.distance_mm[zone];
                    uint8_t zoneIndex = zone - 24;

                    if (zoneIndex < 8)
                    {
                        if (0 <= distance && distance < 200)
                            canData[zoneIndex] = 1;
                        else if (200 <= distance && distance < 600)
                            canData[zoneIndex] = 2;
                        else if (600 <= distance && distance < 1200)
                            canData[zoneIndex] = 3;
                        else if (distance >= 1200)
                            canData[zoneIndex] = 4;
                        else
                            canData[zoneIndex] = 0; // No data
                    }
                }

                // Send Data via CAN
                byte sendStatus = CAN0.sendMsgBuf(sensorBaseCanId + i, 0, 8, canData);
                if (sendStatus == CAN_OK)
                {
                    Serial.print("Sensor ");
                    Serial.print(i + 1);
                    Serial.println(" Data Sent Successfully!");
                }
                else
                {
                    Serial.print("Error Sending Data for Sensor ");
                    Serial.println(i + 1);
                }

                delay(10);
            }
        }
    }

    // Send Heartbeat
    unsigned long currentTime = millis();
    if (currentTime - lastHeartbeatTime >= heartbeatInterval)
    {
        lastHeartbeatTime = currentTime;

        byte heartbeatData[1] = {0xAB}; // Heartbeat signal
        byte sendStatus = CAN0.sendMsgBuf(heartbeatCanId, 0, 1, heartbeatData);

        if (sendStatus == CAN_OK)
            Serial.println("Heartbeat Sent Successfully!");
        else
            Serial.println("Error Sending Heartbeat...");
    }
}

SparkFun_VL53L5CX &getImager(int index)
{
    switch (index)
    {
    case 0:
        return myImager1;
    case 1:
        return myImager2;
    case 2:
        return myImager3;
    case 3:
        return myImager4;
    case 4:
        return myImager5;
    default:
        return myImager1;
    }
}

VL53L5CX_ResultsData &getData(int index)
{
    switch (index)
    {
    case 0:
        return measurementData1;
    case 1:
        return measurementData2;
    case 2:
        return measurementData3;
    case 3:
        return measurementData4;
    case 4:
        return measurementData5;
    default:
        return measurementData1;
    }
}
