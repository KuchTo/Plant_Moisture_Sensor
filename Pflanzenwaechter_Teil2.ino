#include <driver/adc.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>


// Portedefinierung Led's
#define LED_Rot     18    // Rote LED 
#define LED_Gelb    14    // Gelbe LED
#define LED_Gruen   15    // Gruene LED

// LED PWM Einstellungen
#define PWMfreq 5000  // 5 Khz basisfrequenz
#define PWMledChannelA  0
#define PWMledChannelB  1
#define PWMledChannelC  2
#define PWMresolution  8 // 8 Bit Resolution

#define ADCAttenuation ADC_ATTEN_DB_11  //ADC_ATTEN_DB_11 = 0-3,6V  Dämpfung ADC
#define MoisureSens_Poll_Interval 60000  // Intervall zwischen zwei Bodenfeuchtemessungen in Millisekunden
#define MaxSensors 1

#define BLYNK_PRINT Serial
#define BLYNK_NO_BUILTIN 
#define BLYNK_NO_FLOAT
//#define BLYNK_DEBUG

struct MoistureSensorCalibrationData
  {
    int Data[MaxSensors*2] = {1650,2840};  // Calibration Data für Feuchtigkeitssensor. Bitte Projekt Text beachten und Werte ensprechend anpassen 
  };

struct MoistureSensorData
  {
    byte Percent[MaxSensors] = {0};       // Feuchtigkeitssensordaten in Prozent 
    byte Old_Percent[MaxSensors] = {0};   // Vorherige _ Feuchtigkeitssensordaten in Prozent (Zweck: DatenMenge einsparen.)
  };
    
 
//Global Variables

char auth[] = "7d77abf9edac4c398b733822eee463ce"; // Hier lt. Anleitung Auth Token dere Blynk App eintragen (E-Mail).

// Deine WiFi Zugangsdaten.
char ssid[] = "WLANGW1339";
char pass[] = "A09703471882406B#!";  // Set password to "" for open networks.

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
  Run_MoistureSensors(true);
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

    }
  
  return ReturnValue;  
  }

bool Get_Moisture_DatainPercent()
 {
 bool ReadisValid = true;
 for (int i = 0;i < AttachedMoistureSensors;i++)
  {
  if ((MCalib.Data[i] == 0) || (MCalib.Data[i+1] == 0)) // MinADC Value maxADC ADC Value
    { 
    ReadisValid = false;
    return ReadisValid; 
    }
  int RawMoistureValue= ReadMoistureSensor_Raw_Val(i);
  RawMoistureValue= MCalib.Data[i+1] - RawMoistureValue;
  RawMoistureValue=MCalib.Data[i] + RawMoistureValue;
  //Serial.println(MCalib.Data[i]);
  //Serial.println(MCalib.Data[i+1]);
  //Serial.println(RawMoistureValue);
  MMeasure.Percent[i] = map(RawMoistureValue,MCalib.Data[i],MCalib.Data[i+1], 0, 100);
  if (MMeasure.Percent[i] > 100 ) { ReadisValid = false; }
  }
 return ReadisValid;
 }

void Run_MoistureSensors (bool Init)   // HauptFunktion zum Betrieb der Bodenfeuchtesensoren
{
byte MinSensValue = 100;
if ((millis() - Moisure_ServiceCall_Handler >= MoisureSens_Poll_Interval) | (Init))
  {
   Moisure_ServiceCall_Handler = millis();
   if (Get_Moisture_DatainPercent())  // Gültige aktuelle Daten Empfangen
   {
      for (int i = 0;i < AttachedMoistureSensors;i++)
        { 
          if (MMeasure.Percent[i] != MMeasure.Old_Percent[i])
            {
              MMeasure.Old_Percent[i] = MMeasure.Percent[i];
              if (MMeasure.Percent[i] < MinSensValue ) { MinSensValue = MMeasure.Percent[i]; };
              Serial.print(F("Feuchtigkeitswert Sensor 1 in Prozent :"));
              Serial.print(MMeasure.Percent[0]);    
              Serial.println(F(" %")); 
              if (i == 0)  {Blynk.virtualWrite(V1,MMeasure.Percent[i]);  } // Aktualisiere Handywerte
            }           
        }
      if (MMeasure.Percent[0] > 50)
        { SetLedConfig(0,0,20); }
      else if (MMeasure.Percent[0] > 10)
        { 
          SetLedConfig(0,255,0); 
          }
      else 
        { SetLedConfig(255,0,0); } 
   } 
   else  // Keine gültigen Daten empfangen
   {
     Serial.print(F("Bodenfeuchtigkeitssensor nicht kalibiert.Bitte kalibieren. RohDaten des Sensors 1:"));
     Serial.println(ReadMoistureSensor_Raw_Val(0));
     SetLedConfig(255,255,255);
   } 
  }
}
  
 

// Main Loop
void loop() 
{
 Run_MoistureSensors(false);
 Blynk.run();   // Execute Blync Basic- Functions 
}
