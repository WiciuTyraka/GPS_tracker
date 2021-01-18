#include <EEPROM.h>
// #include <RH_E32.h>
#include <RadioTyraka.h>
#include "LoraStructs.h"
#include "Logger.h"
#include <ArduinoJson.h>

// define buttons pins
#define sw1 34
#define sw2 35

// define leds pins
#define led1 26 //error
#define led2 27 // setup done
#define led3 12 // lora init done
#define led4 14 // flesh write

// define lora pins
// Lora pins
#define loraRx 17
#define loraTx 16
#define m0 4
#define m1 0
#define aux 2

// lora configuration struct
struct LoraConfig
{
    uint16_t dataRate;
    uint16_t frequency;
    uint16_t power;
};

// Logger inicialization
Logger logger(&Serial);

// Lora inicialization
// RH_E32 driver(&Serial2, m0, m1, aux); //M0,M1,AUX
RadioTyraka driver(&Serial2, m0, m1, aux);

// TRACKER_PACKET incomingData; // Lora packet for tracker

LoraConfig loraConfig; //Lora configuration

const size_t capacity = JSON_OBJECT_SIZE(3);
DynamicJsonDocument doc(capacity);

int eeAddress = 0; //EEPROM address to start reading from

boolean configBool;

char arr[16];

long int temp_recv = 0; //licznik czasu ktory uplynal od ostatniej wiadomosci

void setup()
{
    // Hardware Serial comunication uart
    Serial.begin(9600);
    // Serial.begin(115200);
    while (!Serial)
        ; // Wait for serial port to be available
    logger.ok(F("setup"), F("Serial for uart comunication succeeded"));
    //---------------------------------------------------------------------------

    // Hardware Serial lora comunication uart
    // Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    Serial2.begin(9600);
    while (!Serial2)
        ;
    logger.ok(F("setup"), F("Serial for lora comunication succeeded"));
    //---------------------------------------------------------------------------

    //  Inicialize pinMode for leds
    pinMode(led1, OUTPUT);
    logger.info(F("setup"), "led1 - error set to pin " + String(led1, DEC));
    pinMode(led2, OUTPUT);
    logger.info(F("setup"), "led2 - setup done set to pin " + String(led2, DEC));
    pinMode(led3, OUTPUT);
    logger.info(F("setup"), "led3 - init success set to pin " + String(led3, DEC));
    pinMode(led4, OUTPUT);
    logger.info(F("setup"), "led4 - flesh write set to pin " + String(led4, DEC));
    //---------------------------------------------------------------------------

    //  Inicialize pinMode for buttons
    pinMode(sw1, INPUT);
    logger.info(F("setup"), "sw1 - right button set to pin " + String(sw1, DEC));
    pinMode(sw2, INPUT);
    logger.info(F("setup"), "sw2 - left button set to pin " + String(sw2, DEC));
    //---------------------------------------------------------------------------

    // Begin EEPROM comunication only for esp
    EEPROM.begin(512);
    logger.ok(F("setup"), F("EEPROM comunication succeeded"));
    //---------------------------------------------------------------------------

    // Lora inicialization
    logger.info(F("setup"), F("Wait for lora to be available"));
    logger.info(F("setup"), F("initing"));
    if (!driver.init())
    {
        logger.error(F("setup"), F("init failed"));
        digitalWrite(led1, HIGH);
        while (1)
            ;
    }
    logger.ok(F("setup"), F("init over"));
    digitalWrite(led3, HIGH);
    //---------------------------------------------------------------------------

    //  Read lora config struct from eeprom
    logger.info(F("setup"), F("Current configuration settings: "));
    readConfig();
    showJSONData();
    resolveConfig();
    //---------------------------------------------------------------------------

    logger.ok(("setup"), F("Setup done"));
    digitalWrite(led2, HIGH);
    //---------------------------------------------------------------------------
}
void loop()
{
    // lora config
    if (digitalRead(sw1) == LOW)
    {
        config();
    }
    // recive lora char
    // else
    // {
    //     if (driver.available()) //czy jest wiadomosc do odebrania?
    //     {

    //         //maksymalna dopuszczalna dlugosc wiadomosci
    //         //wynosi MAX_MESSAGE_LEN = MAX_PAYLOAD_LEN - HEADER_LEN [53 = 58 - 5]
    //         //naglowek dodawany jest automatycznie, oznacza to ze mamy do dyspozycji 55bajtów.
    //         uint8_t buf[RH_E32_MAX_MESSAGE_LEN]; //bufor na wiadomosc
    //         uint8_t len = sizeof(buf);           //ilość znaków

    //         if (driver.recv(buf, &len)) // odebrana wiadomośc zapisujemy w buff
    //         {

    //             //      Serial.print("odebrano wiadomosc : ");
    //             //      Serial.println((char*)buf);
    //             //      Serial.print("o rozmiarze : ");
    //             //      Serial.println(sizeof(buf));
    //             //
    //             //      Serial.print("czas od ostatniej wiadomosci: ");
    //             //      Serial.print(millis() - temp_recv);
    //             //      Serial.println(" [ms]");
    //             //      Serial.println();
    //             Serial.print(F("<"));
    //             Serial.print((char *)buf);
    //             Serial.print(F(","));
    //             Serial.print(millis() - temp_recv);
    //             Serial.print(F(","));
    //             Serial.print(sizeof(buf));
    //             Serial.println(F(">"));

    //             temp_recv = millis();
    //         }
    //         else //jezeli nic nie odebralismy "fail"
    //         {
    //             //                Serial.println(F("recv failed"));
    //             logger.error(F("loop"), F("recv failed"));
    //         }
    //     }
    // }
    // lora recive struct
    else
    {
        if (driver.available()) //czy jest wiadomosc do odebrania?
        {

            struct TRACKER_PACKET incomingData;
            uint8_t len = RH_E32_MAX_MESSAGE_LEN;

            /*Receive message, message stored in buf, len of message stored in len.
              as a result we got info.
              info content:
              Status   <- it's transmission success status,
                MSG_LEN  <- lenght of actual tranmited message + header,
                ID_TX    <- transmiter ID.
                FLAGS    <- user flags.
                MSG_TYPE <-  string or struct.
            */

            PacketInfo info = driver.receiveMessage(&incomingData, &len); //    <----------- recevie message

            Serial.print("Status: ");
            Serial.println(getStatusDescription(info.status));

            if (info.status == L_SUCCESS) // check if Status of transmition is successful.
            {
                print_message(&info, &incomingData);
            }
            else //fail if we receiv nothing - something goes wrong
            {
                Serial.println("recv failed");
            }
        }
    }
}

void print_message(PacketInfo *info, struct TRACKER_PACKET *msg)
{
    // Serial.println("Buffer:"); // Print content of buffer in hex
    // for (int i = 0; i < info.MSG_LEN; i++)
    // {
    //     char b[2];
    //     sprintf(b, "%02X", buf[i]);
    //     Serial.print(b);
    //     Serial.print(" ");
    // }
    Serial.print("odebrano wiadomosc numer: ");
    Serial.println(msg->counter); // Print acctual message
    Serial.print("czas od ostatniej wiadomosci: ");
    Serial.print(millis() - temp_recv);
    Serial.println(" [ms]");
    Serial.println();

    Serial.println("STRUCT:");
    Serial.print("Time UTC         : ");
    Serial.print(msg->gpsHour);
    Serial.print(F(":"));
    Serial.print(msg->gpsMinute);
    Serial.print(F(":"));
    Serial.println(msg->gpsSecond);

    Serial.print("GPS LAT         : ");
    Serial.println(msg->gpsLat);

    Serial.print("GPS LON         : ");
    Serial.println(msg->gpsLon);

    Serial.print("BMP PRESSURE         : ");
    Serial.println(msg->bmpPressure);

    Serial.print("BMP ALTITUDE         : ");
    Serial.println(msg->bmpAltitude);

    Serial.print("BMP TEMPERATURE         : ");
    Serial.println(msg->bmpTemperature);

    Serial.println("HEADER INFO");
    Serial.print("message type   : ");
    Serial.println(info->MSG_TYPE);
    Serial.println("0  - string");
    Serial.println("1  - struct");
    Serial.print("message lenght : ");
    Serial.println(info->MSG_LEN); //print length of message + header
    Serial.println("FLAGS");
    Serial.print("FLAG1         : ");
    Serial.println(info->FLAGS.FLAG1);
    Serial.print("FLAG2         : ");
    Serial.println(info->FLAGS.FLAG2);
    Serial.print("FLAG3         : ");
    Serial.println(info->FLAGS.FLAG3);
    Serial.print("FLAG4         : ");
    Serial.println(info->FLAGS.FLAG4);
    Serial.print("MISSION_STATE : ");
    Serial.println(getflightStateDescription(info->FLAGS.MISSION_STATE));

    Serial.println("-------------------------");
    Serial.println("-------------------------");
    Serial.println();

    temp_recv = millis();
}

// Functions to hendle uart configuration
void config()
{
    const byte numChars = 32;
    char receivedChars[numChars];
    char tempChars[numChars]; // temporary array for use when parsing
    char *strtokIndx;

    // logSerial.println(F("lora configuration"));
    // logSerial.println(F("Enter data in this style <dataRate,frequency,power>"));
    // logSerial.println(F("Example: <10,411,30>"));
    // logSerial.println(F("Default: <5,433,21>"));
    // logSerial.println(F("Current configuration settings: "));
    // readConfig();
    configBool = true;

    while (configBool)
    {
        if (Serial.available() > 0)
        {
            String msg = Serial.readString();
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

                // data are assign to trackerConfig
                strtokIndx = strtok(tempChars, ",");              // get the first part - the string
                loraConfig.dataRate = (uint16_t)atoi(strtokIndx); //    strcpy(messageFromPC, strtokIndx);   // copy it to messageFromPC

                strtokIndx = strtok(NULL, ",");                    // this continues where the previous call left off
                loraConfig.frequency = (uint16_t)atoi(strtokIndx); // convert this part to an integer

                strtokIndx = strtok(NULL, ",");
                loraConfig.power = (uint16_t)atoi(strtokIndx); // convert this part to a float // split the data into its parts

                EEPROM.put(eeAddress, loraConfig); // Saving data to flesh/eeproom
                EEPROM.commit();                   // only use for esp32

                showJSONData();

                resolveConfig();
            }
            else if (msg.startsWith("endConfig"))
            {
                configBool = false;
                Serial.println("Config ended");
            }
            else
            {
                Serial.println("Command not found: " + msg);
            }
            Serial.println("|");
        }
    }
    Serial.println("Tracker configuration complete");
}
//---------------------------------------------------------------------------

// Function to read and show tracker configuration
void readConfig()
{
    EEPROM.get(eeAddress, loraConfig);
    showJSONData();
}
//---------------------------------------------------------------------------

// Function to show configuration
void showJSONData()
{
    doc["dataRate"] = loraConfig.dataRate;
    doc["frequency"] = loraConfig.frequency;
    doc["power"] = loraConfig.power;
    serializeJson(doc, Serial);
    Serial.println();
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
    switch (loraConfig.dataRate)
    {
    case 1:
        Serial.println(F("setDataRate 1kbps"));
        if (!driver.setDataRate(RH_E32::DataRate1kbps))
            Serial.println(F("setDataRate failed"));
        break;
    case 2:
        Serial.println(F("setDataRate 2kbps"));
        if (!driver.setDataRate(RH_E32::DataRate2kbps))
            Serial.println(F("setDataRate failed"));
        break;
    case 5:
        Serial.println(F("setDataRate 5kbps"));
        if (!driver.setDataRate(RH_E32::DataRate5kbps))
            Serial.println(F("setDataRate failed"));
        break;
    case 8:
        Serial.println(F("setDataRate 8kbps"));
        if (!driver.setDataRate(RH_E32::DataRate8kbps))
            Serial.println(F("setDataRate failed"));
        break;
    case 10:
        Serial.println(F("setDataRate 10kbps"));
        if (!driver.setDataRate(RH_E32::DataRate10kbps))
            Serial.println(F("setDataRate failed"));
        break;
    case 15:
        Serial.println(F("setDataRate 15kbps"));
        if (!driver.setDataRate(RH_E32::DataRate15kbps))
            Serial.println(F("setDataRate failed"));
        break;
    case 20:
        Serial.println(F("setDataRate 20kbps"));
        if (!driver.setDataRate(RH_E32::DataRate20kbps))
            Serial.println(F("setDataRate failed"));
        break;
    case 25:
        Serial.println(F("setDataRate 25kbps"));
        if (!driver.setDataRate(RH_E32::DataRate25kbps))
            Serial.println(F("setDataRate failed"));
        break;
    default:
        Serial.println(F("setDataRate default 5kbps"));
        if (!driver.setDataRate(RH_E32::DataRate5kbps))
            Serial.println(F("setDataRate failed"));
        break;
    }
    // Set Power
    //21dBm, 24dBm, 27dBm, 30dBm
    switch (loraConfig.power)
    {
    case 21:
        Serial.println(F("setPower 21dBm"));
        if (!driver.setPower(RH_E32::Power21dBm))
            Serial.println(F("setPower failed"));
        break;
    case 24:
        Serial.println(F("setPower 24dBm"));
        if (!driver.setPower(RH_E32::Power24dBm))
            Serial.println(F("setPower failed"));
        break;
    case 27:
        Serial.println(F("setPower 27dBm"));
        if (!driver.setPower(RH_E32::Power27dBm))
            Serial.println(F("setPower failed"));
        break;
    case 30:
        Serial.println(F("setPower 30dBm"));
        if (!driver.setPower(RH_E32::Power30dBm))
            Serial.println(F("setPower failed"));
        break;
    default:
        Serial.println(F("setPower default 21dBm"));
        if (!driver.setPower(RH_E32::Power21dBm))
            Serial.println(F("setPower failed"));
        break;
    }
    // Set Frequency
    // 410 < uint16_t < 441
    if (loraConfig.frequency < 410 || loraConfig.frequency > 441)
    {
        Serial.println(F("setFrequency no valid frequancy set to deafult 433"));
        if (!driver.setFrequency(433))
            Serial.println(F("setFrequency failed"));
    }
    else
    {
        Serial.print(F("setFrequency "));
        Serial.println(loraConfig.frequency);
        if (!driver.setFrequency(loraConfig.frequency)) // 410 < uint16_t < 441
            Serial.println(F("setFrequency failed "));
    }
}
//---------------------------------------------------------------------------