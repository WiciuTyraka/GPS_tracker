#include <STM32FreeRTOS.h>
#include "queue.h"
// #include <RH_E32.h>
#include <RadioTyraka.h>
#include <TinyGPS++.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
// #include "Logger.h"
#include "logParser.h"
#include "LoraStructs.h"
#include "FlashSPI.h"
#include "barometric_formula.h"
#include <Adafruit_BMP280.h>

//Define configuration interupt
#define configInterupt PB15

//Define led pins
#define led1 PB0  //error
#define led2 PB1  // setup done
#define led3 PB2  // lora init done
#define led4 PB10 // flesh write

// Define lora pins
#define tx PA9
#define rx PA10
#define m0 PA11
#define m1 PA12
#define aux PC13

// define flash pins:
#define flashCs PA8
//#define flashMiso PA6
//#define flashMosi PA7
//#define flashWp PC4
//#define flashHold PA4
//#define clk PA5

// define bmp280 pins
#define sda PB7
#define scl PB6

// inicialize queue for BMP Task
QueueHandle_t xQueue_Pressure;
QueueHandle_t xQueue_Temp;
QueueHandle_t xQueue_Altitude;
// inicialize queue for GPS Task
QueueHandle_t xQueue_GpsLat;
QueueHandle_t xQueue_GpsLon;
QueueHandle_t xQueue_GpsHour;
QueueHandle_t xQueue_GpsMinute;
QueueHandle_t xQueue_GpsSecond;
// inicialize queue for Logger Task
QueueHandle_t xQueue_Logger;

// Define Hardware Serials:
HardwareSerial logSerial(PC7, PC6); // RX, TX
// Logger logger(&logSerial);                 // Logger inicialization
FlashSPI flash(&logSerial); // Flash inicialization
//-----------------------------------------------------------------------------------

// gpsSerial - gps comunication
HardwareSerial gpsSerial(USART2);
TinyGPSPlus gps;
//-----------------------------------------------------------------------------------

// loraSerial - lora comunication
HardwareSerial loraSerial(rx, tx);
RadioTyraka driver(&loraSerial, m0, m1, aux, 127);
// RH_E32 driver(&loraSerial, m0, m1, aux); //M0,M1,AUX
//-----------------------------------------------------------------------------------

// bmp280 pressure sensor inicialization
Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();

sensors_event_t temp_event, pressure_event;
//-----------------------------------------------------------------------------------

// trucker configuration
typedef struct TruckerConfig
{
    uint16_t dataRate;
    uint16_t frequency;
    uint16_t power;
};

TruckerConfig trackerConfig;

int eeAddress = 0; //EEPROM address to start reading from

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars]; // temporary array for use when parsing

boolean newData = false;
boolean configBool;

const size_t capacity = JSON_OBJECT_SIZE(3);
DynamicJsonDocument doc(capacity);
//-----------------------------------------------------------------------------------

// Lora tracker packet
// TRACKER_PACKET dataFrame;
//-----------------------------------------------------------------------------------

void setup()
{
    //  ----------SETUP SERIAL-------------
    logSerial.begin(115200);
    while (!gpsSerial)
        ;
    logSerial.println("logSerial begin");

    gpsSerial.begin(9600);
    while (!gpsSerial)
        ;
    logSerial.println("gpsSerial begin");

    loraSerial.begin(9600);
    while (!loraSerial)
        ;
    logSerial.println("loraSerial begin");
    //---------------------------------------------------------------------------

    //  Inicialize pinMode for leds
    pinMode(led1, OUTPUT);
    //    logger.info(F("setup"), "led1 - error set to pin " + String(led1, DEC));
    pinMode(led2, OUTPUT);
    //    logger.info(F("setup"), "led2 - setup done set to pin " + String(led2, DEC));
    pinMode(led3, OUTPUT);
    //    logger.info(F("setup"), "led3 - init success set to pin " + String(led3, DEC));
    pinMode(led4, OUTPUT);
    //    logger.info(F("setup"), "led4 - flesh write set to pin " + String(led4, DEC));
    //---------------------------------------------------------------------------

    // Begin BMP280 I2C
    Wire.setSDA(sda);
    Wire.setSCL(scl);
    Wire.begin();

    // BMP280 setup
    if (!bmp.begin(0x76))
    {
        logSerial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
        digitalWrite(led1, HIGH);
        while (1)
            delay(10);
    }

    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    //-----------------------------------------------------------------------------------

    // Flash setup
    logSerial.println("Attempting to initialize flash memory.");
    while (!flash.beginFlash(flashCs))
    {
        logSerial.println("Error: Flash initialization fail!");
        delay(500);
    }
    //-----------------------------------------------------------------------------------

    // lora setup
    if (!driver.init())
    {
        //        logger.error(F("setup"), F("init failed"));
        logSerial.println("init failed");
        digitalWrite(led1, HIGH);
        while (1)
            ;
    }

    // lora manuall configuration
    //    if (!driver.setDataRate(RH_E32::DataRate2kbps)) // 1kbps, 2kbps, 5kbps, 8kbps, 10kbps, 15kbps, 20kbps, 25kbps
    //        logSerial.println("setDataRate failed");
    //    if (!driver.setPower(RH_E32::Power21dBm)) //21dBm, 24dBm, 27dBm, 30dBm
    //        logSerial.println("setPower failed");
    //    if (!driver.setFrequency(411)) // 410 < uint16_t < 441
    //        logSerial.println("setFrequency failed");

    // lora configuration via TyrakaApp
    readConfig();
    showJSONData();
    resolveConfig();
    //---------------------------------------------------------------------------

    digitalWrite(led3, HIGH); //led for init compleat

    if (digitalRead(configInterupt) == LOW)
    {
        config(); // tracker configuration
    }

    // Create queue for BMP Task
    xQueue_Altitude = xQueueCreate(1, sizeof(float));
    xQueue_Pressure = xQueueCreate(1, sizeof(float));
    xQueue_Temp = xQueueCreate(1, sizeof(float));
    // Create queue for GPS Task
    xQueue_GpsLat = xQueueCreate(1, sizeof(float));
    xQueue_GpsLon = xQueueCreate(1, sizeof(float));
    xQueue_GpsHour = xQueueCreate(1, sizeof(float));
    xQueue_GpsMinute = xQueueCreate(1, sizeof(float));
    xQueue_GpsSecond = xQueueCreate(1, sizeof(float));
    // Create queue for Loggger Task
    xQueue_Logger = xQueueCreate(5, sizeof(char *));

    logSerial.println("Kolejki iniit");

    // RTOS TASK CREATION
    xTaskCreate(vBmp280ControllerTask, "BMP 280 CONTROLLER TASK", 128, NULL, 1, NULL);
    xTaskCreate(vGpsControllerTask, "GPS CONTROLLER TASK", 128, NULL, 1, NULL);
    xTaskCreate(vFlashControllerTask, "FLASH CONTROLLER TASK", 128, NULL, 1, NULL);
    xTaskCreate(vLoraControllerTask, "LORA CONTROLLER TASK", 128, NULL, 1, NULL);
    xTaskCreate(vLoggerTask, "LOGGER TASK", 128, NULL, 1, NULL);

    // start scheduler
    vTaskStartScheduler();
    logSerial.println("Insufficient RAM");
    while (1)
        ;
}

void loop()
{
}

void vBmp280ControllerTask(void *pvParameters)
{

    float pressure = 0;
    float altitude = 0;
    float sum = 0;
    float temperature = 0;
    float sea_lvl_pres = 1011.45;
    char bmpChar[25];
    char *msg;
    //    String data = "";

    bmp_pressure->getEvent(&pressure_event);

    for (int i = 0; i < 100; i++)
    {
        sum = sum + pressure_event.pressure;
        bmp_pressure->getEvent(&pressure_event);
    }
    sea_lvl_pres = sum / 100;
    //        logSerial.println(sea_lvl_pres);

    while (1)
    {
        bmp_temp->getEvent(&temp_event);
        bmp_pressure->getEvent(&pressure_event);

        pressure = pressure_event.pressure;
        temperature = temp_event.temperature;
        altitude = barinetricAltitude(pressure, temperature, sea_lvl_pres);

        xQueueOverwrite(xQueue_Pressure, &pressure);
        xQueueOverwrite(xQueue_Temp, &temperature);
        xQueueOverwrite(xQueue_Altitude, &altitude);

        msg = debug("BMP TASK", "bmp data collected");
        xQueueSend(xQueue_Logger, &(msg), portMAX_DELAY);
        //
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vFlashControllerTask(void *pvParameters)
{
    // flash setup
    float bmpPressure, bmpTemperature, bmpAltitude;
    float gpsLat, gpsLon, gpsHour, gpsMinute, gpsSecond;
    int counter = 0;
    float mtime = 0;
    char data[128];
    int startTime = millis();
    char *msg;
    String dataString, label, gpsString, bmpString;

    flash.writeFlash("counter,miliseconds,latitude,longitude,timeUTC,pressure,temperature,altitude", 77);

    while (1) // A Task shall never return or exit.
    {
        if (xQueuePeek(xQueue_GpsLat, &(gpsLat), (TickType_t)5) == pdFALSE)
        {
            gpsLat = -1;
        }
        if (xQueuePeek(xQueue_GpsLon, &(gpsLon), (TickType_t)5) == pdFALSE)
        {
            gpsLon = -1;
        }
        if (xQueuePeek(xQueue_GpsHour, &(gpsHour), (TickType_t)5) == pdFALSE)
        {
            gpsHour = -1;
        }
        if (xQueuePeek(xQueue_GpsMinute, &(gpsMinute), (TickType_t)5) == pdFALSE)
        {
            gpsMinute = -1;
        }
        if (xQueuePeek(xQueue_GpsSecond, &(gpsSecond), (TickType_t)5) == pdFALSE)
        {
            gpsSecond = -1;
        }
        if (xQueuePeek(xQueue_Pressure, &(bmpPressure), (TickType_t)5) == pdFALSE)
        {
            bmpPressure = -1;
        }
        if (xQueuePeek(xQueue_Temp, &(bmpTemperature), (TickType_t)5) == pdFALSE)
        {
            bmpTemperature = -1;
        }
        if (xQueuePeek(xQueue_Altitude, &(bmpAltitude), (TickType_t)5) == pdFALSE)
        {
            bmpAltitude = -1;
        }

        mtime = (float)(millis() - startTime) / 1000;

        label = String(counter++, DEC) + "," + String(mtime, DEC);
        gpsString = String(gpsLat, DEC) + "," + String(gpsLon, DEC) + "," + String(gpsHour) + ":" + String(gpsMinute) + ":" + String(gpsSecond);
        bmpString = String(bmpPressure, DEC) + "," + String(bmpTemperature, DEC) + "," + String(bmpAltitude, DEC);
        dataString = label + "," + gpsString + "," + bmpString;

        dataString.toCharArray(data, sizeof(data));

        flash.writeFlash(data, strlen(data) + 1);
        msg = debug("FLASH TASK", "DATA SAVE !!!!");
        xQueueSend(xQueue_Logger, &(msg), portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vGpsControllerTask(void *pvParameters)
{
    // GPS Setup
    float gpsLat, gpsLon;
    int gpsHour, gpsMinute, gpsSecond;
    char *gpsChar;
    char *msg;

    while (1)
    {
        while (gpsSerial.available() > 0)
            if (gps.encode(gpsSerial.read()))
            {
                if (gps.location.isValid())
                {
                    gpsLat = gps.location.lat();
                    gpsLon = gps.location.lng();
                    gpsHour = gps.time.hour();
                    gpsMinute = gps.time.minute();
                    gpsSecond = gps.time.second();
                    xQueueOverwrite(xQueue_GpsLat, &gpsLat);
                    xQueueOverwrite(xQueue_GpsLon, &gpsLon);
                    xQueueOverwrite(xQueue_GpsHour, &gpsHour);
                    xQueueOverwrite(xQueue_GpsMinute, &gpsMinute);
                    xQueueOverwrite(xQueue_GpsSecond, &gpsSecond);

                    msg = debug("GPS TASK", "FIX AVAIABLE !!!!");
                    xQueueSend(xQueue_Logger, &(msg), portMAX_DELAY);
                }
                else
                {
                    msg = debug("GPS TASK", "FIX NOT AVAIABLE !!!!");
                    xQueueSend(xQueue_Logger, &(msg), portMAX_DELAY);
                }
            }
        // vTaskDelay(1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vLoraControllerTask(void *pvParameters)
{
    struct TRACKER_PACKET dataFrame;
    char send_data[RH_E32_MAX_MESSAGE_LEN];
    char *msg;

    while (1)
    {
        if (xQueuePeek(xQueue_GpsLat, &(dataFrame.gpsLat), (TickType_t)5) == pdFALSE)
        {
            dataFrame.gpsLat = -1;
        }
        if (xQueuePeek(xQueue_GpsLon, &(dataFrame.gpsLon), (TickType_t)5) == pdFALSE)
        {
            dataFrame.gpsLon = -1;
        }
        if (xQueuePeek(xQueue_GpsHour, &(dataFrame.gpsHour), (TickType_t)5) == pdFALSE)
        {
            dataFrame.gpsHour = -1;
        }
        if (xQueuePeek(xQueue_GpsMinute, &(dataFrame.gpsMinute), (TickType_t)5) == pdFALSE)
        {
            dataFrame.gpsMinute = -1;
        }
        if (xQueuePeek(xQueue_GpsSecond, &(dataFrame.gpsSecond), (TickType_t)5) == pdFALSE)
        {
            dataFrame.gpsSecond = -1;
        }
        if (xQueuePeek(xQueue_Pressure, &(dataFrame.bmpPressure), (TickType_t)5) == pdFALSE)
        {
            dataFrame.bmpPressure = -1;
        }
        if (xQueuePeek(xQueue_Temp, &(dataFrame.bmpTemperature), (TickType_t)5) == pdFALSE)
        {
            dataFrame.bmpTemperature = -1;
        }
        if (xQueuePeek(xQueue_Altitude, &(dataFrame.bmpAltitude), (TickType_t)5) == pdFALSE)
        {
            dataFrame.bmpAltitude = -1;
        }

        dataFrame.counter++;
        Status status = driver.sendStruct(&dataFrame, sizeof(dataFrame));

        if (status == L_SUCCESS) // check if Status of transmition is successful.
        {
            msg = ok("LORA TASK", "packet send !!!!");
            xQueueSend(xQueue_Logger, &(msg), portMAX_DELAY);
        }
        else
        {
            msg = error("LORA TASK", "packet not send !!!!");
            xQueueSend(xQueue_Logger, &(msg), portMAX_DELAY);
        }
    }
}

void vLoggerTask(void *pvParameters)
{
    char *pcMsgToPrint;
    while (1)
    {
        xQueueReceive(xQueue_Logger, &pcMsgToPrint, portMAX_DELAY);
        logSerial.println(pcMsgToPrint);
    }
}
//---------------------------------------------------------------------------

// Function to confugure tracker by uart
void config()
{
    // logSerial.println(F("lora configuration"));
    // logSerial.println(F("Enter data in this style <dataRate,frequency,power>"));
    // logSerial.println(F("Example: <10,411,30>"));
    // logSerial.println(F("Default: <5,433,21>"));
    // logSerial.println(F("Current configuration settings: "));
    // readConfig();
    configBool = true;
    while (configBool)
    {
        if (logSerial.available() > 0)
        {
            String msg = logSerial.readString();
            msg.trim();
            // logSerial.println(msg);

            if (msg.startsWith("readConfig"))
            {
                readConfig();
            }
            else if (msg.startsWith("writeConfig"))
            {
                msg.remove(0, strlen("writeConfig") + 1); // Get parameters
                msg.toCharArray(receivedChars, numChars); // Copy String to char array
                receivedChars[msg.length() + 1] = '\0';   // Terminate the string

                // this temporary copy is necessary to protect the original data
                // because strtok() used in parseData() replaces the commas with \0
                strcpy(tempChars, receivedChars);

                parseData(); // split the data into its parts

                EEPROM.put(eeAddress, trackerConfig); // Saving data to flesh/eeproom
                // EEPROM.commit();     // only use for esp32

                showJSONData();

                resolveConfig();
            }
            else if (msg.startsWith("eraseAll"))
            {
                logSerial.println("Erase flash started");
                digitalWrite(led4, HIGH);
                flash.eraseAll();
                digitalWrite(led4, LOW);
                Serial.println("Flash erased");
            }
            else if (msg.startsWith("listFiles"))
            {
                flash.listFiles();
            }
            else if (msg.startsWith("readFile"))
            {
                msg.remove(0, strlen("readFile") + 1);    // Get parameters
                msg.toCharArray(receivedChars, numChars); // Copy String to char array
                receivedChars[msg.length() + 1] = '\0';   // Terminate the string

                // this temporary copy is necessary to protect the original data
                // because strtok() used in parseData() replaces the commas with \0
                strcpy(tempChars, receivedChars);
                logSerial.print("Read: ");
                logSerial.println(tempChars);
                flash.readFile(tempChars);
            }
            else if (msg.startsWith("endConfig"))
            {
                configBool = false;
                Serial.println("Config ended");
            }
            else
            {
                logSerial.println("Command not found: " + msg);
            }
            logSerial.println("|");
        }
    }
    logSerial.println("Tracker configuration complete");
}
//---------------------------------------------------------------------------

// Function to read and show tracker configuration
void readConfig()
{
    EEPROM.get(eeAddress, trackerConfig);
    showJSONData();
}
//---------------------------------------------------------------------------

// Function to resolve lora configuration
void resolveConfig()
{
    // Lora config
    // Defaultowe wartości po inicjalizacji
    // 433MHz, 21dBm, 5kbps

    // Set DataRate
    // 1kbps, 2kbps, 5kbps, 8kbps, 10kbps, 15kbps, 20kbps, 25kbps
    switch (trackerConfig.dataRate)
    {
    case 1:
        logSerial.println(F("setDataRate 1kbps"));
        if (!driver.setDataRate(RH_E32::DataRate1kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    case 2:
        logSerial.println(F("setDataRate 2kbps"));
        if (!driver.setDataRate(RH_E32::DataRate2kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    case 5:
        logSerial.println(F("setDataRate 5kbps"));
        if (!driver.setDataRate(RH_E32::DataRate5kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    case 8:
        logSerial.println(F("setDataRate 8kbps"));
        if (!driver.setDataRate(RH_E32::DataRate8kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    case 10:
        logSerial.println(F("setDataRate 10kbps"));
        if (!driver.setDataRate(RH_E32::DataRate10kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    case 15:
        logSerial.println(F("setDataRate 15kbps"));
        if (!driver.setDataRate(RH_E32::DataRate15kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    case 20:
        logSerial.println(F("setDataRate 20kbps"));
        if (!driver.setDataRate(RH_E32::DataRate20kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    case 25:
        logSerial.println(F("setDataRate 25kbps"));
        if (!driver.setDataRate(RH_E32::DataRate25kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    default:
        logSerial.println(F("setDataRate default 5kbps"));
        if (!driver.setDataRate(RH_E32::DataRate5kbps))
            logSerial.println(F("setDataRate failed"));
        break;
    }
    //---------------------------------------------------------------------------

    // Set Power
    //21dBm, 24dBm, 27dBm, 30dBm
    switch (trackerConfig.power)
    {
    case 21:
        logSerial.println(F("setPower 21dBm"));
        if (!driver.setPower(RH_E32::Power21dBm))
            logSerial.println(F("setPower failed"));
        break;
    case 24:
        logSerial.println(F("setPower 24dBm"));
        if (!driver.setPower(RH_E32::Power24dBm))
            logSerial.println(F("setPower failed"));
        break;
    case 27:
        logSerial.println(F("setPower 27dBm"));
        if (!driver.setPower(RH_E32::Power27dBm))
            logSerial.println(F("setPower failed"));
        break;
    case 30:
        logSerial.println(F("setPower 30dBm"));
        if (!driver.setPower(RH_E32::Power30dBm))
            logSerial.println(F("setPower failed"));
        break;
    default:
        logSerial.println(F("setPower default 21dBm"));
        if (!driver.setPower(RH_E32::Power21dBm))
            logSerial.println(F("setPower failed"));
        break;
    }
    //---------------------------------------------------------------------------

    // Set Frequency
    // 410 < uint16_t < 441
    if (trackerConfig.frequency < 410 || trackerConfig.frequency > 441)
    {
        logSerial.println(F("setFrequency no valid frequancy set to deafult 433"));
        if (!driver.setFrequency(433))
            logSerial.println(F("setFrequency failed"));
    }
    else
    {
        logSerial.print(F("setFrequency "));
        logSerial.println(trackerConfig.frequency);
        if (!driver.setFrequency(trackerConfig.frequency)) // 410 < uint16_t < 441
            logSerial.println(F("setFrequency failed "));
    }
}
//---------------------------------------------------------------------------

// Function to parse date from recived configuraton
// data are assign to trackerConfig
void parseData()
{ // split the data into its parts

    char *strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars, ",");                 // get the first part - the string
    trackerConfig.dataRate = (uint16_t)atoi(strtokIndx); //    strcpy(messageFromPC, strtokIndx);   // copy it to messageFromPC

    strtokIndx = strtok(NULL, ",");                       // this continues where the previous call left off
    trackerConfig.frequency = (uint16_t)atoi(strtokIndx); // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    trackerConfig.power = (uint16_t)atoi(strtokIndx); // convert this part to a float
}
//---------------------------------------------------------------------------

// Function to show configuration
void showJSONData()
{
    doc["dataRate"] = trackerConfig.dataRate;
    doc["frequency"] = trackerConfig.frequency;
    doc["power"] = trackerConfig.power;
    serializeJson(doc, logSerial);
    logSerial.println();
}
//---------------------------------------------------------------------------