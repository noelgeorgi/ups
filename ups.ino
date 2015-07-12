// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3

#include "EmonLib.h"             // Include Emon Library
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>

#define currentPin 3
#define inputVoltagePin 2
#define outputVoltagePin 1
#define batteryVoltagePin 0
#define stepUpPinYellow 8
#define stepUpPinRed 9
#define stepDownPinBrown 10
#define backupOnPin 11
#define chargerPin 6

#define inputTransformerConstant 192.06
#define outputTransformerConstant 195.46
#define hallSensorConstant 75.187 // 1/{13.3mV/A)/1000
#define batteryConstant 5.476 // 12.2/(2.2*1023) scaled by 1000=5.42. actual value=5.476
#define phaseShiftConstant 1.7
#define voltCalcCrossings 20
#define voltCalcTimeout 50
#define avgReadings 12
#define coldStartTime 3

#define READVCC_CALIBRATION_CONST 1131750L
#define loadMaxVoltage 12.7
#define lowCutoffVoltage 10.5
#define lowCutoffResetVoltage 11.8
#define batteryBulkVoltage 12.4
#define batteryAbsorbtionVoltage 14.10
#define batteryFloatVoltage 13.2

float avgValue;
float batteryVoltage=0;
float ipVoltage=0;
float opVoltage=0;
float curr=0;

int coldStartVar=0;
int pwmValue=0;

boolean lcdRefresh=true;
boolean percentDisplay=false;
boolean cutOffStatus=false;
boolean warmStartStatus=false;
boolean batteryBulkStatus=true;
boolean batteryAbsorbtionStatus=true;

EnergyMonitor inputVoltage;//,outputVoltage;             // Create an instance
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup()
{  
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  Timer1.initialize(1000000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt(changeStatus); // attach the service routine here
  pinMode(stepUpPinYellow,OUTPUT);
  pinMode(stepUpPinRed,OUTPUT);
  pinMode(stepDownPinBrown,OUTPUT);
  pinMode(backupOnPin,OUTPUT);
  pinMode(chargerPin,OUTPUT);
  
  inputVoltage.voltage(inputVoltagePin, inputTransformerConstant, phaseShiftConstant);  // Voltage: input pin, calibration, phase_shift
  inputVoltage.current(currentPin,hallSensorConstant);
  //outputVoltage.voltage(outputVoltagePin,outputTransformerConstant,phaseShiftConstant);
  
  digitalWrite(stepUpPinYellow,LOW);
  digitalWrite(stepUpPinRed,LOW);
  digitalWrite(stepDownPinBrown,LOW);
  digitalWrite(backupOnPin,LOW);
  analogWrite(chargerPin,0); // pnp
}
void loop()
{
  inputVoltage.calcVI(voltCalcCrossings,voltCalcTimeout);         // Calculate all. No.of half wavelengths (crossings), time-out
  ipVoltage   = inputVoltage.Vrms;    //extract Vrms into Variable
  curr = inputVoltage.Irms;
  //opVoltage   = outputVoltage.Vrms;
  //outputVoltage.calcVI(20,100);
  readBatteryVoltage();
  batteryVoltage = (((readVcc()*avgValue*batteryConstant))/1000000); // Vcc*adc_value*batteryConstant
if(coldStartVar==coldStartTime)
  {
  if(lcdRefresh==true)
  {
    lcdDisplay();
  }
  
  if(ipVoltage>250||ipVoltage<135)
  {
    digitalWrite(stepUpPinYellow,LOW);
    digitalWrite(stepUpPinRed,LOW);
    digitalWrite(stepDownPinBrown,LOW);
    analogWrite(chargerPin,0);
    delay(10);
    if(cutOffStatus==false)
    {
    digitalWrite(backupOnPin,HIGH);
    }
    percentDisplay=true;
    warmStartStatus=true;
    batteryBulkStatus=true;
    batteryAbsorbtionStatus=true;
  }
  else
  {
    if(warmStartStatus==true)
    {
     digitalWrite(stepUpPinYellow,LOW);
     digitalWrite(stepUpPinRed,LOW);
     digitalWrite(stepDownPinBrown,LOW);
     warmStartStatus=false;
     delay(3000);
    digitalWrite(backupOnPin,LOW);
    }
    if(ipVoltage>180&&ipVoltage<210)
  {
    digitalWrite(stepUpPinRed,HIGH);
    digitalWrite(stepUpPinYellow,LOW);
    digitalWrite(stepDownPinBrown,LOW);
  }
  if(ipVoltage>135&&ipVoltage<150)
  {
    digitalWrite(stepUpPinYellow,HIGH);
    digitalWrite(stepUpPinRed,LOW);
    digitalWrite(stepDownPinBrown,LOW);
  }
  if(ipVoltage>220&&ipVoltage<250)
  {
    digitalWrite(stepDownPinBrown,HIGH);
    digitalWrite(stepUpPinYellow,LOW);
    digitalWrite(stepUpPinRed,LOW);
  } 
    if(batteryVoltage<batteryBulkVoltage&&batteryBulkStatus==true)
    {
      analogWrite(chargerPin,255); //bulk charge
      lcd.setCursor(14,0);
      lcd.print("B");
    }
    if(batteryVoltage>=batteryBulkVoltage&&batteryVoltage<=batteryAbsorbtionVoltage)
    {
      batteryBulkStatus=false;
      if(batteryAbsorbtionStatus==true)
      {
      pwmValue=((1033.75-(65.625*batteryVoltage)));
      analogWrite(chargerPin,pwmValue); //eqn of the form y=mx+c; m=(220-115)/(14-12.4), 220=m*12.4+c
      lcd.setCursor(14,0);
      lcd.print("A");
      }
    }
    if(batteryVoltage>batteryAbsorbtionVoltage)
    {
      batteryAbsorbtionStatus=false;
      analogWrite(chargerPin,0);
    }
    if(batteryVoltage<batteryFloatVoltage&&batteryBulkStatus==false&&batteryAbsorbtionStatus==false)
    {
      analogWrite(chargerPin,90);
      lcd.setCursor(14,0);
      lcd.print("F");
    }
    if(batteryVoltage>=batteryFloatVoltage&&batteryBulkStatus==false&&batteryAbsorbtionStatus==false)
    {
      analogWrite(chargerPin,0);
    }
    percentDisplay=false;
  }
  if(batteryVoltage<lowCutoffVoltage)
  {
    digitalWrite(backupOnPin,LOW);
    cutOffStatus=true;
  }
  if(batteryVoltage>lowCutoffResetVoltage)
  {
    cutOffStatus=false;
  }
  
}
}
void readBatteryVoltage()
{
  int sum = 0;
  int temp;
  int i;
  avgValue=0;
  
  for (i=0; i<avgReadings; i++) {            // loop through reading raw adc values AVG_NUM number of times  
    temp = analogRead(batteryVoltagePin);          // read the input pin  
    sum += temp;                        // store sum for averaging
    delayMicroseconds(50);              // pauses for 50 microseconds  
  }
  avgValue= (sum / avgReadings);                // divide sum by AVG_NUM to get average 
}

void lcdDisplay()
{
  lcd.clear();
  lcd.print("I/P=");
  lcd.print(int(ipVoltage));
  lcd.print("V ");
  //lcd.print("I=");
  //lcd.print(curr);
  //lcd.print("A");
  //lcd.print("O/P=");
  //lcd.print(int(opVoltage));
  //lcd.print("V");
  lcd.setCursor(0,1);
  lcd.print("Batt=");
  lcd.print(batteryVoltage);
  lcd.print("V ");
  if(percentDisplay==true)
  {
  lcd.print("%=");
  lcd.print((int)((batteryVoltage-lowCutoffVoltage)/(loadMaxVoltage-lowCutoffVoltage)*100));
  }
}

long readVcc()
{
 long result;
  
  //eliminates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/

  #if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  
  #elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB1286__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  ADCSRB &= ~_BV(MUX5);   // Without this the function always returns -1 on the ATmega2560 http://openenergymonitor.org/emon/node/2253#comment-11432
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
	
  #endif


  #if defined(__AVR__) 
  delay(2);                                        // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);                             // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = READVCC_CALIBRATION_CONST / result;  //1100mV*1024 ADC steps http://openenergymonitor.org/emon/node/1186
  return result;
 #elif defined(__arm__)
  return (3300);                                  //Arduino Due
 #else 
  return (3300);                                  //Guess that other un-supported architectures will be running a 3.3V!
 #endif 
}

void changeStatus()
{
  lcdRefresh=!lcdRefresh;
  if(coldStartVar!=coldStartTime)
  {
  coldStartVar++;
  }
}
