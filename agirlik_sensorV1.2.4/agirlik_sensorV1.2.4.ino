#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

//pins:
const int HX711_dout = 2; //mcu > HX711 dout pin
const int HX711_sck = 3; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

//EEPROM calibration and tare adress
const int calVal_eepromAdress = 0;
const int tareOffsetVal_eepromAdress = 4;
//counting
int productCount = 0;
int knownProductWeight;
int maxProductCount = 10;  
float tolerance = 0.01;  
int displayedNumber = 0;
unsigned long t = 0;
float newCalibrationValue;
float known_mass;
const long massInterval = 500;

//flags
int dar_flag = 0;
int kal_flag = 0;
int tar_flag = 0;
int klb_flag = 0;
int mas_flag = 0;
int bdk_flag = 0;
int ccl_flag = 0;
int yes_flag = 0;
int bos_flag = 0;


String dfd ="";
void callback();

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial2.print("page page0"); 
  Serial2.write(0xff); Serial2.write(0xff); Serial2.write(0xff);


  Serial.println();
  Serial.println("Starting...");
  LoadCell.begin();
  float calibrationValue; 
  //calibrationValue = -106.17;

#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512);
#endif

  EEPROM.get(calVal_eepromAdress, calibrationValue); 
  Serial.println(calibrationValue);
  long tare_offset = 0;
  EEPROM.get(tareOffsetVal_eepromAdress, tare_offset);
  LoadCell.setTareOffset(tare_offset);
  boolean _tare = false; 

  unsigned long stabilizingtime = 2000; 
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("\nStartup is complete");
   
  }
}

void loop() {
 static boolean newDataReady = 0;
 const int serialPrintInterval = 1; 
 unsigned long currentMillis = millis();


  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();  
      if (i < 1) {
       // Serial.print("Load_cell output val: ");
        //Serial.println(0.00);  
        
        sendNextion(0.00);
     
      } else {
       // Serial.print("Load_cell output val: ");
        //Serial.println(i); 
        
        sendNextion(i);
       
        
      }
      newDataReady = 0; 
      t = millis();  
    }
  }

    
  if(dar_flag == 1){

  refreshOffsetValueAndSaveToEEprom();
  dar_flag = 0;
  }
  if(kal_flag == 1){
    
    calibrateScale();
  kal_flag = 0;
  }
  if(tar_flag == 1){

  LoadCell.update();
  LoadCell.tareNoDelay();
  tar_flag = 0;
  }
  if(klb_flag == 1){

  calibrationpart();
  klb_flag = 0;
  }
  if(mas_flag == 1){

  getvalue(); 
  mas_flag = 0;
  }
  if (yes_flag == 1) {

  calibrationpart3();
  yes_flag = 0;
}

  if(bdk_flag == 1){

  bardakset();
  bdk_flag = 0;
  }
  if(ccl_flag == 1){

  colaset();
  ccl_flag = 0;
  }
  if(bos_flag == 1){
  bosset();
  bos_flag = 0; 
  }

callback();
  
}

void callback()  {
unsigned long currentMillis = millis();
  while(Serial2.available()){
  dfd += char(Serial2.read());
  if(dfd.length()>3 && dfd.substring(0,3)!="C:C") dfd="";
  else{
    if(dfd.substring((dfd.length()-1),dfd.length()) == "?")
    {
      String command = dfd.substring(3,6);
      String value = dfd.substring(6,dfd.length()-1);

      if(command == "DAR")
      {
       Serial.println(command);
      dar_flag = 1; 
      }  
      if(command == "KAL")
      {
      Serial.println(command);
      kal_flag = 1;
      }
      if(command == "TAR")
      {
       Serial.println(command);
      tar_flag = 1;
      }   
      if(command == "KLB")
      {
       Serial.println(command);
       klb_flag = 1;
      }    
      if(command == "MAS")
      {
       Serial.println(command);
       mas_flag = 1;
      }    
      if(command == "YES")
      {
       Serial.println(command);
       yes_flag = 1;
      }   
      if(command == "BDK")
      {
       Serial.println(command);
       bdk_flag = 1;
      }   
      if(command == "CCL")
      {
       Serial.println(command);
       ccl_flag = 1;
      } 
      if(command == "BOS")
      {
        Serial.println(command);
        bos_flag = 1;
      }  
      dfd="";
    
  
  
 }
}
}
}

//Tare
void refreshOffsetValueAndSaveToEEprom() {
  long _offset = 0;
  Serial.println("Calculating tare offset value...");
  LoadCell.tare();
  _offset = LoadCell.getTareOffset(); 
  EEPROM.put(tareOffsetVal_eepromAdress, _offset); 
#if defined(ESP8266) || defined(ESP32)
  EEPROM.commit();
#endif
  LoadCell.setTareOffset(_offset); 
  Serial.print("New tare offset value:");
  Serial.print(_offset);
  Serial.print(", saved to EEprom adr:");
  Serial.println(tareOffsetVal_eepromAdress);
}


//Calibration start
void calibrateScale() {
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  unsigned long stabilizingtime = 2000; 
  boolean _tare = true; 
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(1.0); 
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");
}



//Calibration Value
void calibrationpart(){
  
  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");
}

//Calibration Value part 2
void calibrationpart2(float known_mass){
  boolean _resume = false;
    LoadCell.update();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
       
      }
    
  LoadCell.refreshDataSet(); 
  newCalibrationValue = LoadCell.getNewCalibration(known_mass); 

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
}
    
  
  
//Calibration to EEPROM
void calibrationpart3(){

#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);

  Serial.println("End calibration");
  Serial.println("***");
}


      


//Send to Nextion values
void sendNextion(int i){    
    Serial2.print("t0.txt=");
    Serial2.write(0x22);
    Serial2.print(i);
    Serial2.write(0x22);
    Serial2.write(0xff); 
    Serial2.write(0xff);
    Serial2.write(0xff);

   for (int count = 0; count <= maxProductCount; count++) {
    float lowerBound = knownProductWeight * (count - tolerance);
    float upperBound = knownProductWeight * (count + tolerance);

    if (i > lowerBound && i < upperBound) {
        productCount = count;
      
        break;
    }
}


if (productCount == 0) {
    productCount = 0;
}
    Serial2.print("t1.txt=");
    Serial2.write(0x22);
    Serial2.print(productCount);
    Serial2.write(0x22);
    Serial2.write(0xff); 
    Serial2.write(0xff);
    Serial2.write(0xff);
}

//Get value from Nextion
void getvalue() {
  String response = "";
  String value = "";
  unsigned long startMillis = millis();
  unsigned long currentMillis = millis();

 
  Serial2.print("get t22.txt");
  Serial2.write(0xFF);
  Serial2.write(0xFF);
  Serial2.write(0xFF);

  
  while (currentMillis - startMillis < massInterval) {
    while (Serial2.available()) {
      char c = Serial2.read();
      response += c;
    }
    currentMillis = millis();  
  }

 
  int pIndex = response.indexOf('p');
  if (pIndex != -1) {
    int endIndex = response.indexOf(' ', pIndex);
    if (endIndex == -1) {
      endIndex = response.length();
    }
    value = response.substring(pIndex + 1, endIndex);

    // Değeri sayısal bir değere dönüştür
    float known_mass = value.toFloat();
    calibrationpart2(known_mass);
  }
}





void bardakset(){
  knownProductWeight = 3;
}
void colaset(){
  knownProductWeight = 343;
}
void bosset(){
  knownProductWeight = 0;
}