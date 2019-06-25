#include <driver/adc.h>

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

#define ADCAttenuation ADC_ATTEN_DB_11  //ADC_ATTEN_DB_11 = 0-3,6V  Dämpung ADC Einstzellung
#define MaxSensors 1


struct MoistureSensorCalibrationData
  {
    int Data[MaxSensors*2] = {1650,2840};  // Calibration Data für Feuchtigkeitssensor. Bitte Projekt Text beachten und Werte ensprechend anpassen 
  };

struct MoistureSensorData
  {
    byte Percent[MaxSensors] = {0};  // Feuchtigkeitssensordaten in Prozent 
  };
    
 
//Global Variables
MoistureSensorCalibrationData MCalib;
MoistureSensorData MMeasure;
byte AttachedMoistureSensors; // Detected Active Moisture Sensors (Count) 

void setup() {
  // initialize serial communication at 9600 bits per second:
  pinMode(LED_Rot,OUTPUT);
  pinMode(LED_Gelb,OUTPUT);
  pinMode(LED_Gruen,OUTPUT);
  Serial.begin(115200);
  ledcSetup(PWMledChannelA, PWMfreq, PWMresolution);
  ledcSetup(PWMledChannelB, PWMfreq, PWMresolution);
  ledcSetup(PWMledChannelC, PWMfreq, PWMresolution);
  ledcAttachPin(LED_Rot, PWMledChannelA);   // attach the channel to the GPIO to be controlled
  ledcAttachPin(LED_Gelb, PWMledChannelB);
  ledcAttachPin(LED_Gruen, PWMledChannelC);
  SetLedConfig(20,20,20);
  AttachedMoistureSensors = DetectMoistureSensors();
  Serial.println(F("Systemkonfiguration:")); 
  Serial.print(AttachedMoistureSensors);
  Serial.println(F(" Bodenfeuchtigkeitsensor(en)"));
  
}


  
byte DetectMoistureSensors ()
  {
  #define MinSensorValue 100  
  byte Detected = 0;
  for (int i = 0;i < MaxSensors;i++)
    {
    int MSensorRawValue = ReadMoistureSensorVal(i);
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



int ReadMoistureSensorVal(byte Sensor)
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


bool GetMoistureData()
 {
 bool ReadisValid = true;
 for (int i = 0;i < AttachedMoistureSensors;i++)
  {
  if ((MCalib.Data[i] == 0) || (MCalib.Data[i+1] == 0)) // MinADC Value maxADC ADC Value
    { 
    ReadisValid = false;
    return ReadisValid; 
    }
  int RawMoistureValue= ReadMoistureSensorVal(i);
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
  
 

// Main Loop
void loop() 
{
 if (GetMoistureData())
 {
  Serial.print(F("Feuchtigkeitswert Sensor 1 in Prozent :"));
  Serial.print(MMeasure.Percent[0]);    
  Serial.println(F(" %"));
  if (MMeasure.Percent[0] > 50)
   { SetLedConfig(0,0,20); }
  else if (MMeasure.Percent[0] > 10)
   { SetLedConfig(0,255,0); }
  else 
   { 
    Serial.println(F(""));
    SetLedConfig(255,0,0); } 
 }
 else
 {
 Serial.print(F("Bodenfeuchtigkeitssensor nicht kalibiert.Bitte kalibieren. RohDaten des Sensors 1:"));
 Serial.println(ReadMoistureSensorVal(0));
 SetLedConfig(20,20,20);
 }
 
  delay(1000);        // delay between reads for stability 
}
