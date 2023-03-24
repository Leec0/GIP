#include <Arduino.h>
#include <Servo.h>
#include "variable.hpp"

const byte StepX = 2;
const byte DirX = 5;
const byte StepY = 3;
const byte DirY = 6;
const byte ServoPin = 11; //Z end Stop CNC Shield
const byte PinEndXm = 13; //SpnDir pin CNC Shield niet motor kant
const byte PinEndXp = 9; //X end stop pin CNC Shield motor kant
const byte PinEndYm = 12; //SpnEn pin CNC Shield boven kant
const byte PinEndYp = 10; //Y end stop pin CNC Shield onder kant
const byte PinStart = 16; //HOLD pin CNC Shield

byte StateEndXm = 0;
byte StateEndXp = 0;
byte StateEndYm = 0;
byte StateEndYp = 0;

Servo myservo;  
byte pos = 180;
long posX = 0;
long posY = 0;
int nextBit = 0;

long TotStepsX = 0;
long TotStepsYp = 0;
long TotStepsYm = 0;
long TotStepsY = 0;
byte Done = 0;
byte pause = 0;

/*
Idee Foto naar matrix: programma LCD Image convertor gebruiken voor een matrix te krijgen
Positie van de Spuitbus bij houden met var die veranderd als de positie van de spuitkop veranderd
De x pos in een rij in de matrix y pos kolom
spuitbus gaat kolom per kolom af
kijken of op die positie het element 1 of 0 is
bij 1 spuiten en bij 0 verder bewegen naar de volgende positie.

Eventueel controleren of een hele kolom 0 is om dan in een keer naar de volgende kolom te gaan
of kijken of dat de rest van de kolom nul is om terug naar boven te gaan en naar de volgende kolom te gaan
Hierdoor gaat het printen van het ontwerp minder lang duren
*/

void setup()
{
  pinMode(StepX,OUTPUT);
  pinMode(DirX,OUTPUT);
  pinMode(StepY,OUTPUT);
  pinMode(DirY,OUTPUT);
  pinMode(PinEndXm,INPUT_PULLUP);
  pinMode(PinEndXp,INPUT_PULLUP);
  pinMode(PinEndYm,INPUT_PULLUP);
  pinMode(PinEndYp,INPUT_PULLUP);
  pinMode(PinStart, INPUT_PULLUP);

  Serial.begin(9600);

  myservo.attach(ServoPin);       // Servo pin 11 (Z End Stop CNC Shield)
}

void spuitkop() 
{
  for (pos = 80; pos >= 50; pos -= 1) {      // goes from 80 degrees to 50 degrees
    myservo.write(pos);                      // tell servo to go to position in variable 'pos'
    Serial.println(pos);
    delay(15);                               // waits 15ms for the servo to reach the position
  }
  delay(50);
    for (pos = 50; pos <= 80; pos += 1) {   // goes from 50 degrees to 80 degrees
    myservo.write(pos);                     // tell servo to go to position in variable 'pos'
    Serial.println(pos);
    delay(15);                              // waits 15ms for the servo to reach the position
  }
  delay(100);
}

void Step(boolean dir, byte DirPin, byte StepPin, long Steps) // Gebruik: Step([HIGH=Clockwis,LOW=counter clockwise],[DirX,Diry],[StepX,StepY],Aantal Steps)
{
  digitalWrite(DirPin,dir);         //Zet rotatie richting voor gevraagde motor

  for (int i = 0; i < Steps; i++)   //Zet aantal Steps
  {
    digitalWrite(StepPin,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepPin,LOW);
    delayMicroseconds(50);
  }
}

void TotalStepsY()
{
  TotStepsY = 0;
  digitalWrite(DirY,LOW);
  do                          //Beweeg naar onder tot end switch
  {
    digitalWrite(StepY,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepY,LOW);
    delayMicroseconds(50);
    StateEndYp = digitalRead(PinEndYp);
  } while(StateEndYp == 0);
  delay(3);
  digitalWrite(DirY,HIGH);
  do                          //Beweeg naar boven tot end switch en telt bij elke stap
  {
    digitalWrite(StepY,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepY,LOW);
    delayMicroseconds(50);
    TotStepsY++;
    StateEndYm = digitalRead(PinEndYm);
  } while(StateEndYm == 0);
}

void TotalSteps()
{
  digitalWrite(DirY,HIGH);
  do                          //Beweeg naar boven tot end switch
  {
    digitalWrite(StepY,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepY,LOW);
    delayMicroseconds(50);
    StateEndYm = digitalRead(PinEndYm);
  } while(StateEndYm == 0);
  TotStepsX = 0;                  //Reset het Totaal aantal steps
  digitalWrite(DirX,HIGH);
  do                              //Motor gaat naar niet motor kant bewegen tot end switch
  {
    digitalWrite(StepX,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepX,LOW);
    delayMicroseconds(50);
    StateEndXm = digitalRead(PinEndXm);
  } while(StateEndXm == 0);
  TotalStepsY();
  TotStepsYm = TotStepsY; 
  digitalWrite(DirX,LOW);
  do                              //Motor gaat naar de ander kant bewegen tot end switch en telt bij elke stap
  {
    digitalWrite(StepX,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepX,LOW);
    delayMicroseconds(50);
    TotStepsX++;
    StateEndXp = digitalRead(PinEndXp);
  } while(StateEndXp == 0);
  TotalStepsY();
  TotStepsYp = TotStepsY;
  Serial.println("Stappen tellen klaar.");
}

void Tekenen() {
  if (pause == 0)
  {
    if (digitalRead(PinStart) == 1)
    {
      pause = 1;
      while (digitalRead(PinStart) == 0) {}
    }
    if (image[(posX)*ResY+posY] == 1)
    {
      spuitkop();
    }
    for (int i = posX*ResY+posY+1; i < (posX+1)*ResY; i++)
    {
      if (image[i] == 0)
      {
        posY = ResY;
      }
    }
    if (posY >= ResY)
    {
      digitalWrite(DirY,HIGH);
      do                          //Beweeg naar boven tot end switch
      {
        digitalWrite(StepY,HIGH);
        delayMicroseconds(50);
        digitalWrite(StepY,LOW);
        delayMicroseconds(50);
        StateEndYm = digitalRead(PinEndYm);
      } while(StateEndYm == 0);
      Step(HIGH,DirX,StepX,(TotStepsX/ResX));
      posY = 0;
      posX = posX + 1;
    }
    else
    {
      Step(LOW,DirY,StepY,(TotStepsY/ResY));
      posY = posY + 1;
    }
  } else if (digitalRead(PinStart) == 1)
  {
    pause = 0;
    while (digitalRead(PinStart) == 0) {}
  }
}

void loop()
{
  if(Done != 1) //Programma om totaal aantal stappen te tellen als dat nog niet gebeurd is;
  {
    while (digitalRead(PinStart) == 1) {} //Wacht tot de start knop wordt ingedrukt
    TotalSteps();
    if (TotStepsYm < TotStepsYp) //Kijkt aan welke kant het aantal y stappen het kleinst is
    {
      TotStepsY = TotStepsYm - 5000;
    } else if (TotStepsYp <= TotStepsYm)
    {
      TotStepsY = TotStepsYp - 5000;
    }
    posX = 0;
    posY = 0;
    Serial.println(TotStepsX);
    Serial.println(TotStepsYp);
    Serial.println(TotStepsYm);
    Serial.println(TotStepsY);
    Done = 1;    //Varibale om te zeggen dat het totale aantal stappen gemeten is
  }
  while (digitalRead(PinStart) == 1) {}
  Tekenen();
}