#include "Adafruit_ThinkInk.h"
#include "I2Cdev.h"
#include <MPU6050.h>
#include <Wire.h>
#include <math.h>

#define EPD_DC 9
#define EPD_CS 10
#define EPD_BUSY 7 // can set to -1 to not use a pin (will wait a fixed delay)
#define SRAM_CS 6
#define EPD_RESET 8  // can set to -1 and share with microcontroller Reset!
#define EPD_SPI &SPI // primary SPI

#define ALPHA 0
#define BETA 1
#define SAMPLE_SIZE 3

ThinkInk_213_Tricolor_RW display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY,
                                 EPD_SPI);

float COEFFICIENTS_LOW_0_HZ[2][3] = {
    { 1, -1.979133761292768, 0.979521463540373 },
    { 0.000086384997973502, 0.000172769995947004, 0.000086384997973502 }
};

float COEFFICIENTS_LOW_5_HZ[2][3] = {
    { 1, -1.80898117793047, 0.827224480562408 },
    {0.095465967120306, -0.172688631608676, 0.095465967120306}
};

float COEFFICIENTS_HIGH_1_HZ[2][3] = {
    { 1, -1.905384612118461, 0.910092542787947 },
    { 0.953986986993339, -1.907503180919730, 0.953986986993339 }
};

MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;
// float data[100][3];

float datax[SAMPLE_SIZE];
float datay[SAMPLE_SIZE];
float dataz[SAMPLE_SIZE];

float gravx[SAMPLE_SIZE];
float gravy[SAMPLE_SIZE];
float gravz[SAMPLE_SIZE];

float userx[SAMPLE_SIZE];
float usery[SAMPLE_SIZE];
float userz[SAMPLE_SIZE];

float low[SAMPLE_SIZE];
float high[SAMPLE_SIZE];
float input[SAMPLE_SIZE];
bool blink_state;
unsigned long blink_timer;
int total_steps;

void setup() {
  Serial.begin(57600);

  blink_state = true;
  blink_timer = millis();
  total_steps = 0;

  datax[0] = 0;
  datax[1] = 0;
  datax[2] = 0;

  datay[0] = 0;
  datay[1] = 0;
  datay[2] = 0;

  dataz[0] = 0;
  dataz[1] = 0;
  dataz[1] = 0;
    
  gravx[0] = 0;
  gravx[1] = 0;
  gravx[2] = 0;

  gravy[0] = 0;
  gravy[1] = 0;
  gravy[2] = 0;

  gravz[0] = 0;
  gravz[1] = 0;
  gravz[2] = 0;

  input[0] = 0;
  input[1] = 0;
  input[2] = 0;

  low[0] = 0;
  low[1] = 0;
  low[2] = 0;

  high[0] = 0;
  high[1] = 0;
  high[2] = 0;

  Wire.begin();

  // initialize device
  Serial.println("Initializing I2C devices...");
  mpu.initialize();

  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

  display.begin(THINKINK_TRICOLOR);
  Serial.println("Banner demo");
  
  display.clearBuffer();
  display.setTextSize(3);
  display.setCursor((display.width() - 144) / 2, (display.height() - 24) / 2);
  display.setTextColor(EPD_BLACK);
  display.print("Step");
  display.setTextColor(EPD_RED);
  display.print("Counter");
  display.display();

}

void filter(float *data, int size, float coef[2][3], float *output) {
  
  output[2] = coef[ALPHA][0] * (
      data[2] * coef[BETA][0] +
      data[1] * coef[BETA][1] +
      data[0] * coef[BETA][2] -
      output[1] * coef[ALPHA][1] -
      output[0] * coef[ALPHA][2]
  );
}

int measure_steps(float data) {
  float THRESHOLD = 0.01;
  float sqr = 0;
  int steps = 0;
  static float prevsqr;
  static unsigned long t;

  sqr = data * data;

  if (sqr >= THRESHOLD and prevsqr < THRESHOLD and (millis() - t) >= 200) {
    steps = 1;
    t = millis();
  }  

  prevsqr = sqr;

  return steps;
}

void loop() {

  // measure
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // rotate data
  datax[0] = datax[1];
  datax[1] = datax[2];
  datax[2] = ax / 16384.0;

  datay[0] = datay[1];
  datay[1] = datay[2];
  datay[2] = ay / 16384.0;

  dataz[0] = dataz[1];
  dataz[1] = dataz[2];
  dataz[2] = az / 16384.0;
    
  gravx[0] = gravx[1];
  gravx[1] = gravx[2];

  gravy[0] = gravy[1];
  gravy[1] = gravy[2];

  gravz[0] = gravz[1];
  gravz[1] = gravz[2];

  input[0] = input[1];
  input[1] = input[2];

  low[0] = low[1];
  low[1] = low[2];

  high[0] = high[1];
  high[1] = high[2];

  // get gravity accel
  filter(datax, SAMPLE_SIZE, COEFFICIENTS_LOW_0_HZ, gravx);
  filter(datay, SAMPLE_SIZE, COEFFICIENTS_LOW_0_HZ, gravy);
  filter(dataz, SAMPLE_SIZE, COEFFICIENTS_LOW_0_HZ, gravz);

  // get user accel
  userx[2] = datax[2] - gravx[2];
  usery[2] = datay[2] - gravy[2];
  userz[2] = dataz[2] - gravz[2];
  
  input[2] = (gravx[2] * userx[2]) + (gravy[2] * usery[2]) + (gravz[2] * userz[2]);

  filter(input, SAMPLE_SIZE, COEFFICIENTS_LOW_5_HZ, low);
  filter(low, SAMPLE_SIZE, COEFFICIENTS_HIGH_1_HZ, high);

  total_steps += measure_steps(high[2]);

  // update every 5 minutes
  if (millis() - blink_timer >= 300000) {
    blink_state = !blink_state;
    blink_timer = millis();
    Serial.println(total_steps);
    display.clearBuffer();
    display.setTextSize(3);
    display.setCursor((display.width() - 144) / 2, (display.height() - 24) / 2);
    display.setTextColor(EPD_BLACK);
    display.print(total_steps);
    display.display();
  }

  delay(20);
}
