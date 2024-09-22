#include <Arduino.h>
#include <Wire.h>
#include <TCA9548A.h>
#include <BH1750.h>
#include <DS3231.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

TCA9548A I2CMux;
BH1750 lightMeter;
DS3231 myRTC;
const int LEDMatrixPin = 6;
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, LEDMatrixPin, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG, NEO_GRB + NEO_KHZ800);

bool century = false;
bool h12Flag;
bool pmFlag;
int TheHour;
int TheMin;
int brightnessLevel;
float lux;
const uint16_t defaultColor = matrix.Color(255, 165, 40);
const uint16_t dayColor = matrix.Color(255, 250, 220);
const uint16_t midColor = matrix.Color(255, 170, 100);
const uint16_t nightColor = matrix.Color(255, 165, 40);

// Define the number of points in the mapping
const int numPoints = 5;

// Define arrays for input and output values
int surrLux[numPoints] = {0, 100, 200, 260, 10000};
int lampBright[numPoints] = {30, 15, 5, 0, 0};

// Custom non-linear mapping function
int customMap(int inputValue) {
  // Ensure input is within the defined range
  inputValue = constrain(inputValue, surrLux[0], surrLux[numPoints - 1]);

  // Search for the closest input value in the array
  int index;
  for (index = 0; index < numPoints - 1; ++index) {
    if (inputValue < surrLux[index + 1]) {
      break;
    }
  }
  int x0 = surrLux[index];
  int x1 = surrLux[index + 1];
  int y0 = lampBright[index];
  int y1 = lampBright[index + 1];

  // Linear interpolation formula
  return y0 + (inputValue - x0) * (y1 - y0) / (x1 - x0);
}

//TouchSensor Code
const int buttonPin = 5;
bool isLightOn = false;
bool lightHeld = false;
bool afterPress = false;

unsigned long touchCheckTime = 0;
unsigned long lightCheckTime = 0;

void setup() {
  Wire.begin();
  Serial.begin(9600);

  I2CMux.begin(Wire);
  I2CMux.closeAll();

  // Enable channel 0 for BH1750 sensor
  I2CMux.openChannel(0);
  lightMeter.begin();

  // Enable channel 1 for DS3231 RTC
  I2CMux.openChannel(1);

  matrix.begin();
  matrix.setBrightness(10);
  matrix.fillScreen(defaultColor);

  // Set the DS3231 time (you can set this to your desired initial time)
  myRTC.setYear(23);  // Set the year as 23 (assuming it's 2023)
  myRTC.setMonth(10);
  myRTC.setDate(27);
  myRTC.setHour(18);
  myRTC.setMinute(33);
  myRTC.setSecond(0);

  //TouchSensor Code
  pinMode(buttonPin, INPUT_PULLUP);
}

void checkLightSensor() {
  // Read and print data from the BH1750 sensor
  I2CMux.openChannel(0);
  lux = lightMeter.readLightLevel();

  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
}

void buttonCall() {
  int buttonState = digitalRead(buttonPin);  // 0 - 255
  Serial.print("ButtonState = ");
  Serial.println(buttonState);

  if (buttonState == 0 && !isLightOn && !afterPress) {  // Button is pressed and LEDMatrix OFF, Will turn ON LEDMatrix
    isLightOn = true;
    afterPress = true;
    Ledmatrix();
  } else if (buttonState == 0 && isLightOn && !afterPress) {  // Button is pressed and LEDMatrix is ON, will turn off LEDMatrix
    isLightOn = false;
    matrix.setBrightness(0);
    matrix.fillScreen(defaultColor);
    matrix.show();
    afterPress = true;
  } else if (buttonState == 1) {
    afterPress = false;
  }
}

void loop() {
  // Check the touch sensor every second
  if (millis() - touchCheckTime >= 100) {
    touchCheckTime = millis();
    buttonCall();
  }

  // Check the light sensor every five seconds
  if (millis() - lightCheckTime >= 1000) {
    lightCheckTime = millis();
    checkLightSensor();
  }

  // Read and print data from the DS3231 RTC
  I2CMux.openChannel(1);
  TheHour = myRTC.getHour(h12Flag, pmFlag);
  TheMin = myRTC.getMinute();

  Serial.print("Date: ");
  Serial.print(myRTC.getYear(), DEC);
  Serial.print('/');
  Serial.print(myRTC.getMonth(century), DEC);
  Serial.print('/');
  Serial.print(myRTC.getDate(), DEC);
  Serial.print(" Day: ");
  Serial.print(myRTC.getDoW(), DEC);
  Serial.print(" Time: ");
  Serial.print(TheHour, DEC);
  Serial.print(':');
  Serial.print(TheMin, DEC);
  Serial.print(':');
  Serial.print(myRTC.getSecond(), DEC);

  if (h12Flag) {
    if (pmFlag) {
      Serial.print(" PM");
    } else {
      Serial.print(" AM");
    }
  } else {
    Serial.print(" 24h");
  }
  Serial.println();

  buttonCall();
  if (isLightOn) {
    Ledmatrix();
  }

  I2CMux.closeAll();  // Close all channels
}

void Ledmatrix() {
  // Adjust LED matrix brightness based on lux value
  brightnessLevel = customMap(lux);
  matrix.setBrightness(brightnessLevel);
  Serial.print("Brightness Level: ");
  Serial.println(brightnessLevel);

  // Adjust LED matrix color based on time of day
  if (TheHour >= 6 && TheHour < 12) {  // Daytime range (adjust as needed)
    matrix.fillScreen(dayColor);
  } else if (TheHour >= 3 && TheHour < 8) {
    matrix.fillScreen(nightColor);
  } else {
    matrix.fillScreen(midColor);
  }

  matrix.show();
}