//libraria folosita pentru butoane
#include <Button.h>
//libraria folosita pentru PWM
#include <PWM.h>
//libraria folosita pentru LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27);//, 2, 1, 0, 4, 5, 6, 7, 3,POSITIVE);  // stabileste adresa LCD-ului I2C
#include "Timer.h"                     //http://github.com/JChristensen/Timer
Timer t;

//pinii encoderului
#define encoder0PinA  2
#define encoder0PinB  4

//variabile folosite pentru citirea vitezei
volatile long encoder0Pos = 0;
long newposition;
long oldposition = 0;
unsigned long newtime;
unsigned long oldtime = 0;
long vel;

//alocare pini
int minmaxP = 3; //pin buton
int pot_minmax = A0; //pin potentiometru min max
int pot_freq = A1; //pin potentiometru frecventa
int control = 0;
int mes = 2;
int pinPWM = 9;
String pwm, freq,  mesaj = "";
String rpm = "00000";
String comanda;
String sens;

//alte variabile

boolean comut_inax;//variabila folosite pentru comutare buton minmax PWM
int val_pot_minmax = 0; //variabila care retine valoarea pot pentru min/max PWM
int val_pot_freq = 0; //variabila care retine valoarea pot frecventa

int val_pwm, val_pwm_o; //variabila de iesire PWM
int val_freq = 500, val_freq_o = 500; //variabila de iesire FREQ in Hz



Button minmax = Button(2, BUTTON_PULLUP_INTERNAL); //butonul folosit pentru comutare pwm min/max


void setup() {
  t.every(200, serial_S);

  pinMode(pinPWM, OUTPUT);
  pinMode(pot_minmax, INPUT);
  Serial.begin(115200);
  lcd.begin(20, 4); //initializeaza lcd
  InitTimersSafe(); //initializeaza timer inafara de timer 0

  bool success = SetPinFrequencySafe(pinPWM, val_freq);//seteaza noua frecventa

  //daca este ok semnalizeaza pe serial
  if (success) {
    Serial.println("FREQ OK");
  }
  lcd.setCursor ( 0, 0 );
  lcd.print("RPM= ");
  lcd.setCursor ( 0, 1 );
  lcd.print("SENS= ");
  lcd.setCursor ( 0, 2 );
  lcd.print("PWM= ");
  lcd.setCursor ( 0, 3 );
  lcd.print("FREQ= ");

  //setare pini encoder
  pinMode(encoder0PinA, INPUT);
  digitalWrite(encoder0PinA, HIGH);       // turn on pullup resistor
  pinMode(encoder0PinB, INPUT);
  digitalWrite(encoder0PinB, HIGH);       // turn on pullup resistor
  attachInterrupt(0, doEncoder, RISING);  // encoDER ON PIN 2


}

void loop() {
  String k, v;
  inax();


  ////////////////////////////////////////////////
  // respond to commands from serial.
  ////////////////////////////////////////////////
  //while (Serial.available()) {
  if (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n')
    {
      if (comanda.length() > 0) {
        parseCmd(comanda, &k, &v);
        if (k == "c")
        {
          control = v.toInt();
        }

        /*  if (k=="m")
         {
           mes=v.toInt();
         }*/
        if (k == "f")
        {
          val_freq = constrain(v.toInt(), 500, 3000);
        }
        if (k == "p")
        {
          val_pwm = constrain(v.toInt(), 0, 255);
        }
        /* Serial.print("parseCmd k=");
         Serial.println(k);
         Serial.print("parseParam v=");
         Serial.println(v);*/
        comanda = "";
      }
    }
    else
    {
      comanda += c;
    }
  }
  //}


  if (  control == 0) {
    val_pot_minmax = analogRead(pot_minmax);
    long t = 0;
    for (int x = 0; x < 100; x++) {
      val_pot_freq = analogRead(pot_freq);
      t = t + val_pot_freq;
    }
    val_pot_freq = t / 100;


    if (comut_inax == true) { //daca e minim
      val_pwm = map(  val_pot_minmax, 0, 1023, 127, 0);
      sens="orar";
    }
    else {//daca e maxim
      val_pwm = map( val_pot_minmax, 0, 1023, 127, 254);
      sens="antiorar";
    }

    val_freq = map(val_pot_freq, 0, 1023, 500, 3000);

  }
  else
  {
  }
  t.update();


  //daca apare o modificare in valoarea pwm
  if (val_pwm != val_pwm_o)
  {
    pwmWrite(pinPWM, val_pwm);
    float tmp = (float)val_pwm / (float)254 * 100;
    pwm = String(tmp);
    pwm = pwm + "%";
    val_pwm_o = val_pwm;
  }


  if (val_freq != val_freq_o)
  {
    bool success = SetPinFrequencySafe(pinPWM, val_freq);//seteaza noua frecventa

    //daca este ok semnalizeaza pe serial
    if (success) {
      pwmWrite(pinPWM, val_pwm);
      if ((val_freq >= 500) && (val_freq < 1000))
      { freq = "0" + String(val_freq);
      }
      else
      {
        freq = String(val_freq);
      }



    }
    val_freq_o = val_freq;
  }



}

//functia inax comuta intre PWM minim sau maxim in functie de apasarea butonului
void inax() {

  if (minmax.isPressed()) {
    comut_inax = false;
  } else {
    comut_inax = true;
  }
}
//sfarsit functie inax




int parseCmd(String cmdLine, String *key, String *value) {
  int equalsPos;
  equalsPos = cmdLine.indexOf('=');
  if (equalsPos == -1) {
    *key = cmdLine;
    *value = "";
  }
  else {
    *key = cmdLine.substring(0, equalsPos);
    *value = cmdLine.substring(equalsPos + 1);
  }
  return (equalsPos);
}

void serial_S() {
  //calculare viteza
  newposition = encoder0Pos;
  newtime = millis();
  vel = (newposition - oldposition) * 1000 / (newtime - oldtime);//calculeaza impulsuri pe secunda
  vel = (vel/1000)*60; //transforma in rotatii pe minut
  oldposition = newposition;
  oldtime = newtime;
  if (vel < 10)
  {
    rpm = "0000" + String(vel);

  }
  else if ((vel < 100) && (vel >= 10))
  { rpm = "000" + String(vel);
  }
  else if ((vel < 1000) && (vel >= 100))
  { rpm = "00" + String(vel);
  }
  else if ((vel < 10000) && (vel >= 1000))
  { rpm = "0" + String(vel);
  }

  else
  {
    rpm = String(vel);
  }
  mesaj = pwm + "|"; mesaj = mesaj + freq; mesaj = mesaj + "|"; mesaj = mesaj + rpm;
  //afisare pe serial si LCD

  Serial.println(mesaj);
  lcd.setCursor ( 4, 2 );
  lcd.print("    ");
  lcd.setCursor ( 4, 2 );
  lcd.print(pwm);

  lcd.setCursor ( 5, 3 );
  lcd.print("    ");
  lcd.setCursor ( 5, 3 );
  lcd.print(val_freq);

  lcd.setCursor ( 4, 0 );
  lcd.print("         ");
  lcd.setCursor ( 4, 0 );
  lcd.print(rpm);

  lcd.setCursor ( 6, 1 );
  lcd.print("    ");
  lcd.setCursor ( 6, 1 );
  lcd.print(sens);

}

void doEncoder()
{
  if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)) {
    encoder0Pos++;
  } else {
    encoder0Pos--;
  }
}
