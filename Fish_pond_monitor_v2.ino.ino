#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DFRobot_PH.h"
#include <EEPROM.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Sensor Pins
#define PH_SENSOR_PIN A1
#define DO_SENSOR_PIN A2
#define AMMONIA_SENSOR_PIN A3

// LED Pins
#define PH_LED 6
#define DO_LED 7
#define AMMONIA_LED 8

// Constants for DO sensor
#define VREF 5000    // Reference voltage in mV
#define ADC_RES 1024 // ADC Resolution
#define READ_TEMP 25 // Default water temperature in °C

// Single-point calibration values
#define CAL1_V 131   // Calibration voltage in mV
#define CAL1_T 25    // Calibration temperature in °C

// Resistor and sensor constants for Ammonia sensor
#define RL 47        // RL value in KΩ
#define m -0.263     // Slope from sensor calibration
#define b 0.42       // Intercept from sensor calibration
#define Ro 20        // Ro value from calibration

// Lookup table for DO sensor
const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};

// pH Sensor Variables
float voltage, phValue, temperature = READ_TEMP;
DFRobot_PH ph;

void setup() {
    Serial.begin(115200);
    ph.begin();  // Initialize pH sensor
    lcd.begin(20, 4);  // Initialize LCD with 20 columns and 4 rows
    lcd.backlight();

    pinMode(PH_LED, OUTPUT);
    pinMode(DO_LED, OUTPUT);
    pinMode(AMMONIA_LED, OUTPUT);

    // **Display Startup Message**
    lcd.setCursor(2, 0);
    lcd.print("CHAKRISTARK");
    lcd.setCursor(5, 1);
    lcd.print("PRESENTS");
    lcd.setCursor(1, 2);
    lcd.print("FISH PONDS MONITOR");
    lcd.setCursor(5, 3);
    lcd.print("VERSION 2.0");
    delay(5000); // Show message for 5 seconds
    lcd.clear();
}

void loop() {
    static unsigned long timepoint = millis();
    if (millis() - timepoint > 1000U) {  // Read values every second
        timepoint = millis();

        // **pH Sensor Reading with Blinking LED**
        digitalWrite(PH_LED, HIGH);
        voltage = analogRead(PH_SENSOR_PIN) / 1024.0 * 5000;  // Convert to mV
        phValue = ph.readPH(voltage, temperature);  // pH calculation with temperature compensation
        ph.calibration(voltage, temperature);  // Calibration
        digitalWrite(PH_LED, LOW);
        delay(200);

        // **DO Sensor Reading with Blinking LED**
        digitalWrite(DO_LED, HIGH);
        uint16_t ADC_Raw = analogRead(DO_SENSOR_PIN);
        uint16_t ADC_Voltage = (uint32_t)VREF * ADC_Raw / ADC_RES;
        float doValue = readDO(ADC_Voltage, temperature);
        digitalWrite(DO_LED, LOW);
        delay(200);

        // **Ammonia Sensor Reading with Blinking LED**
        digitalWrite(AMMONIA_LED, HIGH);
        float VRL = analogRead(AMMONIA_SENSOR_PIN) * (5.0 / 1023.0); // Convert to voltage
        float sensorResistance = ((5.0 * RL) / VRL) - RL; // Compute resistance
        float ratio = sensorResistance / Ro;
        float ammoniaValue = pow(10, ((log10(ratio) - b) / m)); // Compute NH3 ppm
        digitalWrite(AMMONIA_LED, LOW);
        delay(200);

        // **Display on LCD**
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("pH: ");
        lcd.print(phValue, 2);

        lcd.setCursor(0, 1);
        lcd.print("DO: ");
        lcd.print(doValue, 2);
        lcd.print(" mg/L");

        lcd.setCursor(0, 2);
        lcd.print("NH3: ");
        lcd.print(ammoniaValue, 2);
        lcd.print(" ppm");

        lcd.setCursor(0, 3);
        lcd.print("Temp: ");
        lcd.print(temperature, 1);
        lcd.print(" C");

        Serial.print("pH: ");
        Serial.print(phValue, 2);
        Serial.print("\tDO: ");
        Serial.print(doValue, 2);
        Serial.print(" mg/L\tNH3: ");
        Serial.print(ammoniaValue, 2);
        Serial.print(" ppm\tTemp: ");
        Serial.print(temperature, 1);
        Serial.println(" C");
    }
}

// **DO Sensor Calculation**
int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c) {
    uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
    return (voltage_mv * DO_Table[temperature_c] / V_saturation);
}