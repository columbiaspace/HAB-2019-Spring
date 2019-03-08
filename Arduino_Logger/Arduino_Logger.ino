#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <MS5611.h>

//The better altitude sensor
MS5611 ms5611;


// DELETE THIS LATER PROBABLY J*NKASS PIECE OF SHIT
// Altitude Sensor
//Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
// LCD
Adafruit_LiquidCrystal lcd(0);

// Counter that needs to be outside of the loop
int target_count = 0;

// receiving from rocket?
int receive_flag = 0;

// time
tmElements_t tm;

double referencePressure;


void checkSettings()
{
  Serial.print("Oversampling: ");
  Serial.println(ms5611.getOversampling());
}

// Reduce clutter for LCD update
void printLCD(String curr_time, String alt, String pres){

  if (alt.length() == 2)
    alt = "  " + alt;
  if (alt.length() == 3)
    alt = " " + alt;
  
  lcd.setCursor(0,1);
  lcd.print("A:");
  lcd.setCursor(2,1);
  lcd.print(alt);
  lcd.setCursor(6,1);
  lcd.print("m");

  lcd.setCursor(9,0);
  lcd.print("P:");
  lcd.setCursor(10,1);
  lcd.print(pres);

  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.setCursor(2,0);
  lcd.print(curr_time);
}

String to2digits(int number) {
  String num = String(number);
  if (number >= 0 && number < 10) {
    num = '0' + num;
  }
  return num;
}

String getTime(tmElements_t tm) {
  String output = "";
  if (RTC.read(tm)) {
    String temp = "";
    temp = to2digits(tm.Hour);
    output = output + temp;
    temp = to2digits(tm.Minute);
    output = output + temp;
    temp = to2digits(tm.Second);
    output = output + temp;
  }
  return output;
}
 
void setup(void) {
  int errflag = 0;
  tmElements_t tm;
  Serial.begin(9600);
  while (!Serial) ;
  Serial.println("RX"); Serial.println("");

  /* Initialize lcd */
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  
  /* Initialize the sensor */
//  if(!bmp.begin()) {
//    /* There was a problem detecting the BMP085 ... check your connections */
//    errflag = 1;
//    Serial.print("BMP problem");
//    Serial.print("aErr");
//  }
    // Initialize MS5611 sensor
  Serial.println("Initialize MS5611 Sensor");

  if(!ms5611.begin())
  {
    errflag=1;
    Serial.println("Could not find a valid MS5611 sensor, check wiring!");
  }

  // Get reference pressure for relative altitude
  referencePressure = ms5611.readPressure();

  // Check settings
  checkSettings();

  

  /* Initialize SD card */
  Serial.print("SD");
  // see if the card is present and can be initialized:
  if (!SD.begin(10)) {
    errflag = 1;
    Serial.println("Fail");
  }
  Serial.println("Init");

  /* Initialize radio */
  //Serial.print("Radio");
  //rocketSerial.begin(1200);

  /* Print header line */
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  String headerString = "time, b_alt, temp, pres, r_alt";
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(headerString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(headerString);
    
  }
  // if the file isn't open, pop up an error:
  else {
    errflag = 1;
    Serial.println("f_err");
  }

  if(errflag) {
    pinMode(13, OUTPUT);
    while(1) {
      Serial.println("err");
      delay(100);
    }
  }

}
 
void loop(void) {
  // launch altitude in meters
  int TARGETALT = 50;

  // local altitude 
  float alt_l = 0;
  String alt_l_str = "";
  
  // rocket altitude
  int alt_r = 0;
  String alt_r_str = "-1";
  
  // local stats
  String temp = "";
  String pres = "";
  String rocket_input = "";

  // pressure sensor get event
  sensors_event_t event; 
//  bmp.getEvent(&event);

  // better sensor readings
  // Read raw values
  uint32_t rawTemp = ms5611.readRawTemperature();
  uint32_t rawPressure = ms5611.readRawPressure();

  // Read true temperature & Pressure
  double realTemperature = ms5611.readTemperature();
  long realPressure = ms5611.readPressure();

  // Calculate altitude 
  float absoluteAltitude = ms5611.getAltitude(realPressure);
  float relativeAltitude = ms5611.getAltitude(realPressure, referencePressure);

  
  /* Update local altitude */
  if (realPressure) {
    /* Display atmospheric pressue in hPa */
//    Serial.println("");
//    Serial.print("Pres:    ");
//    pres = String(event.pressure);
//    Serial.print(event.pressure);
//    Serial.println(" hPa");

    Serial.println("");
    Serial.print("Better Real pres:   ");
    Serial.print(realPressure);
    Serial.println("hPa");
     
    /* First we get the current temperature from the BMP085 */
//    float temperature;
//    temp = String(temperature);
//    Serial.print("Temp: ");
//    Serial.print(temperature);
//    Serial.println(" C");
    
    Serial.println("");
    
    Serial.print("Better Tmp: ");
    Serial.print(realTemperature);
    Serial.println(" C");
    
    
 
    /* Then convert the atmospheric pressure, SLP and temp to altitude    */
    /* Update this next line with the current SLP for better results      */
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    Serial.print("Alt:    "); 
//    alt_l = bmp.pressureToAltitude(seaLevelPressure,event.pressure,temperature);
    // Altitude as string
    Serial.print(relativeAltitude);
    Serial.println(" m");
    printLCD("[N/A]", String(relativeAltitude), String(realPressure));
  } else {
    Serial.println("Pres Err");
    printLCD("ERR", String(alt_l_str), String(realPressure));
  }
  
  String curr_time = getTime(tm);
  printLCD(curr_time, String(relativeAltitude), String(realPressure));
  String dataString = "time, "+curr_time+", altitude, " + String(absoluteAltitude,2);
  String betterData = " Abs Alt, " + String(absoluteAltitude,2) + ", Rel Alt, " + relativeAltitude + ", Temp," + realTemperature +
          ", Real Pres, " + realPressure;

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("d.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString + betterData);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
    Serial.println(betterData);
    
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("f_err");
  }
  
  delay(1000);
}
