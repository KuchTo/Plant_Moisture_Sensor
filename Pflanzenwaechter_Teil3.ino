#include <driver/adc.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>


// Portedefinierung Led's
#define LED_Rot     18    // Rote LED 
#define LED_Gelb    14    // Gelbe LED
#define LED_Gruen   15    // Gruene LED

// LED PWM Einstellungen
#define PWMfreq 5000  // 5 Khz basisfrequenz für LED Anzeige
#define PWMledChannelA  0
#define PWMledChannelB  1
#define PWMledChannelC  2
#define PWMresolution  8 // 8 Bit Resolution für LED PWM

//Sonstige Definitionen
#define ADCAttenuation ADC_ATTEN_DB_11    //ADC_ATTEN_DB_11 = 0-3,6V  Dämpfung ADC (ADC Erweiterung
#define MoisureSens_Poll_Interval 300000   // Intervall zwischen zwei Bodenfeuchtemessungen in Millisekunden  -> alle 5 Minuten Datenpaket an Handy senden
#define MaxSensors 6                      // Maximale Anzahl an anschließbaren FeuchteSensoren
#define StartInit true
#define Sens_Calib true
#define Sens_NOTCalib false

// Blynk APP Definitionen
#define BLYNK_GREEN     "#23C48E" 
#define BLYNK_BLUE      "#04C0F8" 
#define BLYNK_YELLOW    "#ED9D00" 
#define BLYNK_RED       "#D3435C" 
#define BLYNK_BLACK     "#000000" 
#define BLYNK_PRINT Serial
#define BLYNK_NO_BUILTIN 
#define BLYNK_NO_FLOAT
//#define BLYNK_DEBUG

struct MoistureSensorCalibrationData
  {
    int Data[MaxSensors*2] = {1651,2840,1652,2840,1653,2840,1654,2840,1655,2840,1656,2840};  // Calibration Data für Feuchtigkeitssensor. Bitte Projekt Text beachten und Werte ensprechend anpassen
  //  int Data[MaxSensors*2] = {0,0,0,0,0,0,0,0,0,0,0,0,};  // Calibration Data für Feuchtigkeitssensor. Bitte Projekt Text beachten und Werte ensprechend anpassen
    String SensorName[MaxSensors] = {"Pflanze 1","Pflanze 2","Pflanze 3","Pflanze 4","Pflanze 5","Pflanze 6"}; // Sensorname, der auch in der APP als Überschrift angezeigt wird
    byte StatusBorderPercentValues[MaxSensors*2][2]= { {10,50},         // Zweidimensinonales Array für Prozentgrenzwerte (Ampel) jeweils Einzeln pro Feuchtesensor (1 -6) 
                                                       {10,50},
                                                       {10,50},
                                                       {10,50},
                                                       {10,50},
                                                       {10,50}};
  };

struct MoistureSensorData
  {
    int Percent[MaxSensors] = {0,0,0,0,0,0};       // Feuchtigkeitssensordaten in Prozent 
    byte Old_Percent[MaxSensors] = {0,0,0,0,0,0};   // Vorherige _ Feuchtigkeitssensordaten in Prozent (Zweck: DatenMenge einsparen.)
    bool DataValid [MaxSensors] = {false,false,false,false,false,false};
  };
    
 
//Global Variables

  char auth[] = "7d77abf9edac4c398b733822eee463ce"; // Hier lt. Anleitung Auth Token dere Blynk App eintragen (E-Mail).
//char auth[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // Hier lt. Anleitung Auth Token dere Blynk App eintragen (E-Mail).
// Deine WiFi Zugangsdaten.
char ssid[] = "WLANGW1339";
//char ssid[] = "Deine_WLAN_SSID";                 // Bitte an eigene WLAN SSID anpassen
char pass[] = "A09703471882406B#!";  // Set password to "" for open networks.
//char pass[] = "Dein _WLAN _Passwort!";           // Bitte an eigene WLAN Passwort anpassen
MoistureSensorCalibrationData MCalib;
MoistureSensorData MMeasure;
byte AttachedMoistureSensors; // Detected Active Moisture Sensors (Count) 
unsigned long Moisure_ServiceCall_Handler = 0;  // Delay Variable for Delay between Moisure Readings

void setup() {
  pinMode(LED_Rot,OUTPUT);
  pinMode(LED_Gelb,OUTPUT);
  pinMode(LED_Gruen,OUTPUT);
  Serial.begin(115200);   // initialize serial communication at 115200 bits per second:
  ledcSetup(PWMledChannelA, PWMfreq, PWMresolution);
  ledcSetup(PWMledChannelB, PWMfreq, PWMresolution);
  ledcSetup(PWMledChannelC, PWMfreq, PWMresolution);
  ledcAttachPin(LED_Rot, PWMledChannelA);   // attach the channel to the GPIO to be controlled
  ledcAttachPin(LED_Gelb, PWMledChannelB);
  ledcAttachPin(LED_Gruen, PWMledChannelC);
  SetLedConfig(255,255,255);
  Serial.println(F("Systemkonfiguration:"));
  AttachedMoistureSensors = DetectMoistureSensors();
  Serial.print(AttachedMoistureSensors);
  Serial.println(F(" Bodenfeuchtigkeitsensor(en)"));
  Serial.print(F("Verbindung zu WLAN"));
  delay(500);
  Blynk.begin(auth, ssid, pass);  // Initalize WiFi-Connection over Blync Library
  Serial.println(F(" erfolgreich."));
  SetLedConfig(0,0,0);
  Init_Blynk_APP();
  Run_MoistureSensors(StartInit);
  for (int i = AttachedMoistureSensors;i < 6;i++) { Update_Blynk_APP(i,Sens_Calib); };
}
  
byte DetectMoistureSensors ()
  {
  #define MinSensorValue 100  
  byte Detected = 0;
  for (int i = 0;i < MaxSensors;i++)
    {
    int MSensorRawValue = ReadMoistureSensor_Raw_Val(i);
    if ( MSensorRawValue > MinSensorValue) { Detected++; } else {break;}
    }
  if (Detected < 1)
    { 
      Serial.println(F("Keine Bodenfeuchtigkeitssesoren erkannt. System angehalten."));
      esp_deep_sleep_start();
      while(1) {}
    }   
  return Detected;
  }

bool SetLedConfig(byte Red,byte yellow,byte green)
{
ledcWrite(PWMledChannelA, Red); // Rote LED 
ledcWrite(PWMledChannelB, yellow); // Gelbe LED
ledcWrite(PWMledChannelC, green); // Gruene LED
return true;
}

int ReadMoistureSensor_Raw_Val(byte Sensor)
  {
   int ReturnValue,i;
   long sum = 0;
   #define NUM_READS 6   
   adc1_config_width(ADC_WIDTH_BIT_12);   //Range 0-4095    
   switch (Sensor)
    {
    case 0:
      {
      adc1_config_channel_atten(ADC1_CHANNEL_0,ADCAttenuation);
      for (i = 0; i < NUM_READS; i++){  // Averaging algorithm 
      sum += adc1_get_raw( ADC1_CHANNEL_0 ); //Read analog 
      } 
      ReturnValue = sum / NUM_READS;
      break;   
      }
    case 1:
      {
      adc1_config_channel_atten(ADC1_CHANNEL_3,ADCAttenuation);
      for (i = 0; i < NUM_READS; i++){  // Averaging algorithm 
      sum += adc1_get_raw( ADC1_CHANNEL_3 ); //Read analog
      }
      ReturnValue = sum / NUM_READS;
      break;  
      }
    case 2:
      {     
      adc1_config_channel_atten(ADC1_CHANNEL_6,ADCAttenuation);
      for (i = 0; i < NUM_READS; i++){  // Averaging algorithm 
      sum += adc1_get_raw( ADC1_CHANNEL_6 ); //Read analog 
      }
      ReturnValue = sum / NUM_READS;
      break;  
      }
    case 3:
      { 
      adc1_config_channel_atten(ADC1_CHANNEL_7,ADCAttenuation);
      for (i = 0; i < NUM_READS; i++){  // Averaging algorithm 
      sum += adc1_get_raw( ADC1_CHANNEL_7 ); //Read analog      
      }
      ReturnValue = sum / NUM_READS;
      break;  
      }
    case 4:
      {
      adc1_config_channel_atten(ADC1_CHANNEL_4,ADCAttenuation);
      for (i = 0; i < NUM_READS; i++){  // Averaging algorithm 
      sum += adc1_get_raw( ADC1_CHANNEL_4 ); //Read analog
      }
      ReturnValue = sum / NUM_READS;
      break;  
      }
    default:
      {
      adc1_config_channel_atten(ADC1_CHANNEL_5,ADCAttenuation);
      for (i = 0; i < NUM_READS; i++){  // Averaging algorithm 
      sum += adc1_get_raw( ADC1_CHANNEL_5 ); //Read analog 
      }
      ReturnValue = sum / NUM_READS;     
      break;  
      }
    }
  
  return ReturnValue;  
  }

void Init_Blynk_APP()
  {
    Blynk.setProperty(V1,"label",MCalib.SensorName[0]);
    Blynk.setProperty(V2,"label",MCalib.SensorName[1]); 
    Blynk.setProperty(V3,"label",MCalib.SensorName[2]); 
    Blynk.setProperty(V4,"label",MCalib.SensorName[3]); 
    Blynk.setProperty(V5,"label",MCalib.SensorName[4]); 
    Blynk.setProperty(V6,"label",MCalib.SensorName[5]); 
  }

void Update_Local_Display()
{
  byte red1 = 0;
  byte yellow1 = 0;
  byte green1 = 0;    
  for (byte i = 0;i < AttachedMoistureSensors;i++)
    {
    if (MMeasure.DataValid[i])
        {        
        if ( MMeasure.Percent[i] > MCalib.StatusBorderPercentValues[i][1])
            {
            green1++; 
            } else if ( MMeasure.Percent[i] > MCalib.StatusBorderPercentValues[i][0])
            {
            yellow1++;  
            } else
            {
            red1++; 
            }
        }
    }  
  if (red1 > 0)
    { SetLedConfig(255,0,0); }
  else if (yellow1 > 0)
    { SetLedConfig(0,255,0); }
  else if (green1 > 0)
    { SetLedConfig(0,0,100); } 
  else 
    { SetLedConfig(100,100,100); }    
}

void Update_Blynk_APP(byte Sensor,bool Calibrated)
  {
    switch (Sensor)
    {
    case 0:
      {
      if ((MMeasure.DataValid[0]) & (Calibrated))
        {
        if ( MMeasure.Percent[0] > MCalib.StatusBorderPercentValues[0][1])
            {
            Blynk.setProperty(V1,"color",BLYNK_GREEN);  
            } else if ( MMeasure.Percent[0] > MCalib.StatusBorderPercentValues[0][0])
            {
            Blynk.setProperty(V1,"color",BLYNK_YELLOW);  
            } else
            {
            Blynk.setProperty(V1,"color",BLYNK_RED);  
            }
        delay(100);    
        Blynk.virtualWrite(V1,MMeasure.Percent[0]);            
        } else
        {
        if (Calibrated)
          {
          Blynk.setProperty(V1,"label","Deaktiviert");
          delay(100);         
          Blynk.virtualWrite(V1,0);
          delay(100);  
          Blynk.setProperty(V1,"color",BLYNK_BLACK);
          }
        else
          { 
          Blynk.virtualWrite(V1,0);
          delay(100);    
          Blynk.setProperty(V1,"color",BLYNK_BLUE);  
          }        
        }        
      break;   
      }
    case 1:
      {
      if ((MMeasure.DataValid[1]) & (Calibrated))
        {
        if ( MMeasure.Percent[1] > MCalib.StatusBorderPercentValues[1][1])
            {
            Blynk.setProperty(V2,"color",BLYNK_GREEN);  
            } else if ( MMeasure.Percent[1] > MCalib.StatusBorderPercentValues[1][0])
            {
            Blynk.setProperty(V2,"color",BLYNK_YELLOW);  
            } else
            {
            Blynk.setProperty(V2,"color",BLYNK_RED);  
            }
        delay(100);    
        Blynk.virtualWrite(V2,MMeasure.Percent[1]);            
        } else
        {          
        if (Calibrated)
          {         
          Blynk.setProperty(V2,"label","Deaktiviert");
          delay(100); 
          Blynk.virtualWrite(V2,0);  
          delay(100);
          Blynk.setProperty(V2,"color",BLYNK_BLACK);
          }
        else
          {
          Blynk.virtualWrite(V2,0);
          delay(100);
          Blynk.setProperty(V3,"color",BLYNK_BLUE);  
          }  
        }        
      break;  
      }
    case 2:
      {     
      if ((MMeasure.DataValid[2]) & (Calibrated))
        {
        if ( MMeasure.Percent[2] > MCalib.StatusBorderPercentValues[2][1])
            {
            Blynk.setProperty(V3,"color",BLYNK_GREEN);  
            } else if ( MMeasure.Percent[2] > MCalib.StatusBorderPercentValues[2][0])
            {
            Blynk.setProperty(V3,"color",BLYNK_YELLOW);  
            } else
            {
            Blynk.setProperty(V3,"color",BLYNK_RED);  
            }
        delay(100);    
        Blynk.virtualWrite(V3,MMeasure.Percent[2]);            
        } else
        {          
        if (Calibrated)
          {         
          Blynk.setProperty(V3,"label","Deaktiviert");
          delay(100); 
          Blynk.virtualWrite(V3,0);
          delay(100);  
          Blynk.setProperty(V3,"color",BLYNK_BLACK);
          }
        else
          {   
          Blynk.virtualWrite(V3,0);
          delay(100);
          Blynk.setProperty(V3,"color",BLYNK_BLUE);  
          }  
        }        
      break;   
      }
    case 3:
      { 
      if ((MMeasure.DataValid[3]) & (Calibrated))
        {
        if ( MMeasure.Percent[3] > MCalib.StatusBorderPercentValues[3][1])
            {
            Blynk.setProperty(V4,"color",BLYNK_GREEN);  
            } else if ( MMeasure.Percent[3] > MCalib.StatusBorderPercentValues[3][0])
            {
            Blynk.setProperty(V4,"color",BLYNK_YELLOW);  
            } else
            {
            Blynk.setProperty(V4,"color",BLYNK_RED);  
            }
        delay(100);    
        Blynk.virtualWrite(V4,MMeasure.Percent[3]);            
        } else
        {          
        if (Calibrated)
          {
          Blynk.setProperty(V4,"label","Deaktiviert");
          delay(100);           
          Blynk.virtualWrite(V4,0);
          delay(100);
          Blynk.setProperty(V4,"color",BLYNK_BLACK);
          }
        else
          {
          Blynk.virtualWrite(V4,0);  
          delay(100);
          Blynk.setProperty(V4,"color",BLYNK_BLUE);  
          }
        }        
      break; 
      }
    case 4:
      {
      if ((MMeasure.DataValid[4]) & (Calibrated))
        {
        if ( MMeasure.Percent[4] > MCalib.StatusBorderPercentValues[4][1])
            {
            Blynk.setProperty(V5,"color",BLYNK_GREEN);  
            } else if ( MMeasure.Percent[4] > MCalib.StatusBorderPercentValues[4][0])
            {
            Blynk.setProperty(V5,"color",BLYNK_YELLOW);  
            } else
            {
            Blynk.setProperty(V5,"color",BLYNK_RED);  
            }
        delay(100);    
        Blynk.virtualWrite(V5,MMeasure.Percent[4]);            
        } else
        {          
        if (Calibrated)
          {
          Blynk.setProperty(V5,"label","Deaktiviert");
          delay(100);       
          Blynk.virtualWrite(V5,0);
          delay(100);
          Blynk.setProperty(V5,"color",BLYNK_BLACK);
          }
        else
          {
          Blynk.virtualWrite(V5,0);
          delay(100);
          Blynk.setProperty(V5,"color",BLYNK_BLUE);  
          }
        }        
      break; 
      }
    default:
      {
      if ((MMeasure.DataValid[5]) & (Calibrated))
        {
        if ( MMeasure.Percent[5] > MCalib.StatusBorderPercentValues[5][1])
            {
            Blynk.setProperty(V6,"color",BLYNK_GREEN);  
            } else if ( MMeasure.Percent[5] > MCalib.StatusBorderPercentValues[5][0])
            {
            Blynk.setProperty(V6,"color",BLYNK_YELLOW);  
            } else
            {
            Blynk.setProperty(V6,"color",BLYNK_RED);  
            }
        delay(100);    
        Blynk.virtualWrite(V6,MMeasure.Percent[5]);            
        } else
        {          
        if (Calibrated)
          {
          Blynk.setProperty(V6,"label","Deaktiviert");
          delay(100);          
          Blynk.virtualWrite(V6,0);
          delay(100);  
          Blynk.setProperty(V6,"color",BLYNK_BLACK);
          }
        else
          {
          Blynk.virtualWrite(V6,0);
          delay(100);
          Blynk.setProperty(V6,"color",BLYNK_BLUE);  
          }
        }        
      break;  
      }
    } // End Switch
  }

void Get_Moisture_DatainPercent()
 {
 byte CalibDataOffset = 0; 
 for (byte i = 0;i < AttachedMoistureSensors;i++)
  {
  CalibDataOffset =  i *2;  
  int RawMoistureValue = ReadMoistureSensor_Raw_Val(i);
  if ((MCalib.Data[CalibDataOffset] == 0) || (MCalib.Data[CalibDataOffset+1] == 0)) // MinADC Value maxADC ADC Value
    {
    MMeasure.Percent[i] = RawMoistureValue;  
    MMeasure.DataValid[i] = false; 
    } else
    {
    RawMoistureValue= MCalib.Data[CalibDataOffset+1] - RawMoistureValue;
    RawMoistureValue=MCalib.Data[CalibDataOffset] + RawMoistureValue;
    MMeasure.Percent[i] = map(RawMoistureValue,MCalib.Data[CalibDataOffset],MCalib.Data[CalibDataOffset+1], 0, 100);
    if ((MMeasure.Percent[i] > 100 ) | (MMeasure.Percent[i] < 0 )) 
      { 
      MMeasure.Percent[i] = RawMoistureValue;  
      MMeasure.DataValid[i] = false; 
      } else  {  MMeasure.DataValid[i] = true; }
    }
  }
 return ;
 }



void Run_MoistureSensors (bool Init)   // HauptFunktion zum Betrieb der Bodenfeuchtesensoren
{
byte MinSensValue = 100;
if ((millis() - Moisure_ServiceCall_Handler >= MoisureSens_Poll_Interval) | (Init))
  {
   Moisure_ServiceCall_Handler = millis();
   Get_Moisture_DatainPercent(); 
   for (int i = 0;i < AttachedMoistureSensors;i++)
      {    
        if (MMeasure.DataValid[i])
          {
                      
          if (MMeasure.Percent[i] != MMeasure.Old_Percent[i])
            {
            
            MMeasure.Old_Percent[i] = MMeasure.Percent[i];
            if (MMeasure.Percent[i] < MinSensValue ) { MinSensValue = MMeasure.Percent[i]; };
            Serial.print(F("Feuchtigkeitswert Sensor "));
            Serial.print(i);
            Serial.print(F(" in Prozent :"));
            Serial.print(MMeasure.Percent[i]);    
            Serial.println(F(" %")); 
            Update_Blynk_APP(i,Sens_Calib);     // Aktualisiere Handywerte
            }
          } else
          {
           Update_Blynk_APP(i,Sens_NOTCalib);     // Aktualisiere Handywerte 
           Serial.print(F("Sensor "));
           Serial.print(i);
           Serial.print(F(" nicht kalibiert. Bitte kalibrieren. Rohdatenwert:"));
           Serial.println(MMeasure.Percent[i]);        
          }
      }
   Update_Local_Display();           // Aktualisiere lokales Pflanzenwächter Display (Led)     
  }
}
  
 

// Main Loop
void loop() 
{
 Run_MoistureSensors(false);
 Blynk.run();   // Execute Blync Basic- Functions 
}
