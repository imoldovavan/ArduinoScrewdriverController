#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <avr/eeprom.h>

#include <AccelStepper.h>
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// change this to fit the number of steps per revolution
// for your motor
const int stepsPerRevolution = 200;

// initialize the stepper library on pins 2, 3, 10, 11:
//Stepper myStepper(stepsPerRevolution, 2, 3, 10, 11);
AccelStepper myStepper(AccelStepper::FULL4WIRE, 2, 3, 10, 11);

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
int percent = 0;
long timer = 0;
int checkvalue = 0;
int checktime = 0;
long reset = 0;
int stepperdelay = 0;  // this is miliseconds
int stepperdelayold = 0;
int idelay, idelaymax; 
int selection = 0;
int bandselected = 0;
signed int motorlocation = 0;
unsigned long myTime = 0;
unsigned long selectTime = 0;


#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
#define idelayreset   1400
#define stepperdelayreset 234

#define memCurrentPosition 22
#define memCurrentBand 24

enum {
  band10=0,
  band17,
  band20,
  band40,
}; //do not go over 10


EEMEM unsigned long pos10m, pos17m, pos20m, pos40m, defaultBand, defaultPosition;    //a variable stored in EEPROM
long bar1, bar2;            //regular variables in SRAM

int read_LCD_buttons(){               // read the buttons
  adc_key_in = analogRead(0);       // read the value from the sensor

  if (adc_key_in > 1000) return btnNONE;

  // For V1.1 use this threshold
  if (adc_key_in < 50)   return btnRIGHT; 
  if (adc_key_in < 200)  return btnUP;
  if (adc_key_in < 350)  return btnDOWN;
  if (adc_key_in < 550)  return btnLEFT;
  if (adc_key_in < 800)  return btnSELECT; 

  return btnNONE;                // when all others fail, return this.
}

void displayselection(int select){

  switch (select)
  {
  case band10:
    lcd.setCursor(0,1);
    lcd.print("10m");
    if (bandselected == select)
    {
      lcd.print("* ");
    } else {
      lcd.print("  ");
    }
    delay(300);
    break;

  case band17:
    lcd.setCursor(0,1);
    lcd.print("17m");
    
    if (bandselected == select)
    {
      lcd.print("* ");
    } else {
      lcd.print("  ");
    }
    
    delay(300);
    break;

  case band20:
    lcd.setCursor(0,1);
    lcd.print("20m");
    if (bandselected == select)
    {
      lcd.print("* ");
    } else {
      lcd.print("  ");
    }
    
    delay(300);
    break;

  case band40:
    lcd.setCursor(0,1);
    lcd.print("40m");
    if (bandselected == select)
    {
      lcd.print("* ");
    } else {
      lcd.print("  ");
    }
    delay(300);
    break;
  
  default:
    break;
  }
}

// int memBand(int band){
//   return band*2;
// }

void memwritebandposition (int band){
  switch (band)
  {
  case band10:
    eeprom_write_dword( (unsigned long*) &pos10m, myStepper.currentPosition() );
    break;
  
  case band17:
    eeprom_write_dword( (unsigned long*) &pos17m, myStepper.currentPosition() );
    break;

  case band20:
    eeprom_write_dword( (unsigned long*) &pos20m, myStepper.currentPosition() );
    break;

  case band40:
    eeprom_write_dword( (unsigned long*) &pos40m, myStepper.currentPosition() );
    break;

  default:
    break;
  }
}

signed int memreadbandposition (int band){
  switch (band)
  {
  case band10:
    return eeprom_read_dword( (unsigned long*) &pos10m);
    break;
  
  case band17:
    return eeprom_read_dword( (unsigned long*) &pos17m);
    break;

  case band20:
    return eeprom_read_dword( (unsigned long*) &pos20m);
    break;

  case band40:
    return eeprom_read_dword( (unsigned long*) &pos40m);
    break;

  default:
    return -10000;
    break;
  }
  return -10001;
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("BAND MoveTo  Now");

  // set stepper motor speed:
  myStepper.setMaxSpeed(800);
  myStepper.setAcceleration(400.0);
  selection = eeprom_read_dword( &defaultBand );
  bandselected = selection;
  myStepper.setCurrentPosition(eeprom_read_dword( (unsigned long*) &defaultPosition ));
  displayselection(selection);
  selectTime = 2000;

}

void loop() {
  lcd.setCursor(5,1);
  lcd.print(memreadbandposition(selection));
  lcd.print("  ");

  lcd.setCursor(11,1);
  lcd.print(myStepper.currentPosition());
  lcd.print("   ");
  
  // read the buttons
  lcd_key = read_LCD_buttons();

  // depending on which button was pushed, we perform an action
  switch (lcd_key){

    case btnRIGHT:{             //  push button "RIGHT" 
      selection++;
      if (selection >= 4) selection = 0;
      
      displayselection(selection);
      

      break;
    }
    case btnLEFT:{              //  push button "LEFT"
      selection--;
      if (selection < 0) selection = 3;
      displayselection(selection);
      

      break;
    }   
    case btnUP:{                //  push button "UP"
      
      myStepper.move(10);

      break;
    }
    case btnDOWN:{            //  push button "DOWN"

      myStepper.move(-10);

      break;
    }
    case btnSELECT:{           //  push button "SEL"

      selectTime = millis() - myTime;
      
      break;
    }
    case btnNONE:{
      if (!myStepper.isRunning()) myStepper.disableOutputs();
      if(selectTime < 500) {
        bandselected = selection;
        //EEPROMWriteInt(memCurrentBand, bandselected);
        eeprom_write_dword( &defaultBand, bandselected );
        eeprom_write_dword( &defaultPosition, memreadbandposition(selection));
        lcd.setCursor(3,1);
        lcd.print("* ");
        myStepper.moveTo(memreadbandposition(selection));
        
      }

      if(selectTime > 500 && selectTime < 2000 ){
        // EEPROMWriteInt(0, bandselected);
        //EEPROMWriteInt(memBand(selection), myStepper.currentPosition());
        memwritebandposition(selection);
        // lcd.setCursor(4,1);
        // lcd.print("w"); 
        // lcd.print(bandselected);
      }
      
      myTime = millis();
      selectTime = 2000;
      //lcd.setCursor(5,1);
      
      break;
    }
  }
  myStepper.run();
}