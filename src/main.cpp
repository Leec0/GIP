#include <Arduino.h>      //library voor Arduino code te laten werken
#include <Servo.h>        //library voor de Servo te laten werken
#include <avr/pgmspace.h> //library voor grote variable op te slaan in het Flash geheugen
#include "test.hpp"       //eigen library die verwijst naar de image variable

const byte StepX = 2;     //Variabelen die bepalen wat op welke pin is ingesteld
const byte DirX = 5;
const byte StepY = 3;
const byte DirY = 6;
const byte ServoPin = 11; //Z end Stop CNC Shield
const byte PinEndXm = 13; //SpnDir pin CNC Shield niet motorkant
const byte PinEndXp = 9; //X end stop pin CNC Shield motorkant
const byte PinEndYm = 12; //SpnEn pin CNC Shield bovenkant
const byte PinEndYp = 10; //Y end stop pin CNC Shield onderkant
const byte PinStart = 16; //HOLD pin CNC Shield

Servo myservo;  //instellingen voor Servo
byte pos = 80; //positie variabele voor Servo

long posX = 0;  //positie variabele voor de Spuitkop
long posY = 0;

long TotStepsX = 0;   //Variabele voor het totaal aantal stappen te meten
long TotStepsYp = 0;
long TotStepsYm = 0;
long TotStepsY = 0;

byte Done = 0;  //Variabele voor programma logica
byte pause = 0;
long Skip = -1;

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

void setup()    //Alle pinnen juist instellen
{
  pinMode(StepX,OUTPUT);  //instellen pinnen voor stappenmotors
  pinMode(DirX,OUTPUT);
  pinMode(StepY,OUTPUT);
  pinMode(DirY,OUTPUT);
  pinMode(PinEndXm,INPUT_PULLUP);   //instellen pinnen voor knoppen
  pinMode(PinEndXp,INPUT_PULLUP);
  pinMode(PinEndYm,INPUT_PULLUP);
  pinMode(PinEndYp,INPUT_PULLUP);
  pinMode(PinStart, INPUT_PULLUP);

  Serial.begin(9600);   //Het starten van de Seriële monitor om het programma te kunnen volgen op de PC

  myservo.attach(ServoPin);   //instellen pin voor Servo motor

  myservo.write(80);

  Done = 0;   //Variabele Resetten naar 0
  Skip = -1;
  pause = 0;
}

void spuitkop()   //Programma dat de spuitkop 1 keer laat spuiten
{
  for (pos = 80; pos >= 50; pos -= 1) {      // gaat van 80 graden naar 50 graden
    myservo.write(pos);                      // geeft opdracht aan servo om naar positie ‘pos’ te gaan 
    delay(15);                               // wacht 15ms 
  }
  delay(50);
    for (pos = 50; pos <= 80; pos += 1) {   // gaat van 50 graden naar 80 graden
    myservo.write(pos);                     // geeft opdracht aan servo om naar positie ‘pos’ te gaan
    delay(15);                              // wacht 15ms
  }
}

void Step(boolean dir, byte DirPin, byte StepPin, long Steps) //Programma om een stappenmotor x aantal stappen te doen draaien
{ //Gebruik: Step([HIGH=Clockwise, LOW=counter clockwise], [DirX,Diry], [StepX,StepY], Aantal Steps)
  digitalWrite(DirPin,dir);         //Zet rotatie richting voor gevraagde motor
  for (int i = 0; i < Steps; i++)   //Zet aantal stappen
  {
    digitalWrite(StepPin,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepPin,LOW);
    delayMicroseconds(50);
  }
}

void TotalStepsY()  //Programma voor het totaal aantal stappen te meten in de verticale richting
{
  TotStepsY = 0;    //Reset het totaal aantal stappen
  digitalWrite(DirY,LOW);
  do                          //Beweeg naar onder tot end switch
  {
    digitalWrite(StepY,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepY,LOW);
    delayMicroseconds(50);
  } while(digitalRead(PinEndYp) == 0);
  delay(3);
  digitalWrite(DirY,HIGH);
  do                          //Beweeg naar boven tot end switch en telt bij elke stap
  {
    digitalWrite(StepY,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepY,LOW);
    delayMicroseconds(50);
    TotStepsY++;
  } while(digitalRead(PinEndYm) == 0);
}

void TotalSteps() //Programma om het totaal aantal stappen te meten
{
  digitalWrite(DirY,HIGH);
  do                          //Beweeg naar boven tot end switch
  {
    digitalWrite(StepY,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepY,LOW);
    delayMicroseconds(50);
  } while(digitalRead(PinEndYm) == 0);
  TotStepsX = 0;                  //Reset het Totaal aantal steps
  digitalWrite(DirX,LOW);
  do                              //Motor gaat naar niet motorkant bewegen tot end switch
  {
    digitalWrite(StepX,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepX,LOW);
    delayMicroseconds(50);
  } while(digitalRead(PinEndXp) == 0);
  TotalStepsY();                  //Het programma voor de verticale stappen te meten wordt gestart
  TotStepsYm = TotStepsY;         //Het totaal aantal stappen in de verticale richting wordt opgeslagen
  digitalWrite(DirX,HIGH);
  do                              //Motor gaat naar de ander kant bewegen tot end switch en telt bij elke stap
  {
    digitalWrite(StepX,HIGH);
    delayMicroseconds(50);
    digitalWrite(StepX,LOW);
    delayMicroseconds(50);
    TotStepsX++;
  } while(digitalRead(PinEndXm) == 0);
  TotalStepsY();                //Het programma voor de verticale stappen te meten wordt gestart
  TotStepsYp = TotStepsY;       //Het totaal aantal stappen in de verticale richting wordt opgeslagen
  Serial.println("Stappen tellen klaar.");
}

void Tekenen()  //Programma dat de printer een bepaalde tekening gaat laten tekenen
{
  if (pause == 0) //Controleer of het programma niet gepauzeerd is
  {
    if (digitalRead(PinStart) == 1) //Kijk of de pauzeknop wordt ingedrukt
    {
      while (digitalRead(PinStart) == 1) {}   //Wacht tot de pauzeknop terug is uitgedrukt
      pause = 1;                              //Pauzeer het programma
      delay(25);
    }
    if (Skip == -1 and posX < ResX-1)
    {
      for (long i = 0; i >= ResY; i++)
      {
        uint8_t Pixel = pgm_read_byte(&image[posX*ResX+i]);
        if (Pixel == 1)
        {
          Skip = i;
        }
        else if(Skip <= -1)
        {
          Skip = -2;
        }
      }
    }
    uint8_t Pixel = pgm_read_byte(&image[posX*ResX+posY]);  //slaag de waarde van 1 Pixel van de foto op in aparte variabele
    if (Pixel == 1) //Controleer of die waarde overeenkomt met een 1
    {
      spuitkop();   //Als die waarde 1 is Spuit op die positie
    }
    Serial.println(posY);
    Serial.println(posX);
    Serial.println(((posX*ResX+posY)/(ResX*ResY-1))*100);
    if (posX >= ResX-1 and posY >= ResY-1)   //Controleer of de spuitkop de laatste positie heeft bereikt
    {
      digitalWrite(DirY,HIGH);
      do                          //Beweeg naar boven tot end switch
      {
        digitalWrite(StepY,HIGH);
        delayMicroseconds(50);
        digitalWrite(StepY,LOW);
        delayMicroseconds(50);
      } while(digitalRead(PinEndYm) == 0);
      digitalWrite(DirX,HIGH);
      do                              //Motor gaat naar de motor kant bewegen tot end switch
      {
        digitalWrite(StepX,HIGH);
        delayMicroseconds(50);
        digitalWrite(StepX,LOW);
        delayMicroseconds(50);
      } while(digitalRead(PinEndXm) == 0);
      Done = 2; //Eindig het tekenprogramma
    }
    else if (posY >= ResY-1)  //Controleer of de spuitkop op het einde is in de verticale positie
    {
      digitalWrite(DirY,HIGH);
      do                          //Beweeg naar boven tot end switch
      {
        digitalWrite(StepY,HIGH);
        delayMicroseconds(50);
        digitalWrite(StepY,LOW);
        delayMicroseconds(50);
      } while(digitalRead(PinEndYm) == 0);
      Step(LOW,DirX,StepX,(TotStepsX/ResX)); //Beweeg de spuitkop horizontaal 1 positie
      Step(LOW,DirY,StepY,500);
      posY = 0;
      posX++; //Maak de variabele 1 waarde hoger
      Skip = -1;
    }
    else if (posY > Skip and Skip >= 0)
    {
       digitalWrite(DirY,HIGH);
      do                          //Beweeg naar boven tot end switch
      {
        digitalWrite(StepY,HIGH);
        delayMicroseconds(50);
        digitalWrite(StepY,LOW);
        delayMicroseconds(50);
      } while(digitalRead(PinEndYm) == 0);
      Step(LOW,DirX,StepX,(TotStepsX/ResX)); //Beweeg de spuitkop horizontaal 1 positie
      Step(LOW,DirY,StepY,500);
      posY = 0;
      posX++; //Maak de variabele 1 waarde hoger
      Skip = -1;
    }
    else if(posY < ResY-1)  //Controleer of de spuitkop nog niet op het einde is in verticale positie
    {
      Step(LOW,DirY,StepY,(TotStepsY/ResY));  //Beweeg de spuitkop verticaal 1 positie
      posY++; //Maak de Var 1 waarde hoger
    } 
  }
  else if (pause == 1 and digitalRead(PinStart) == 1)  //Als het programma gepauzeerd is controleer of de pauzeknop terug wordt ingedrukt
  {
    while (digitalRead(PinStart) == 1) {}   //Wacht tot de pauzeknop terug is uitgedrukt
    pause = 0;                              //ga verder met het programma
    delay(25);
  }
}

void loop()
{
  if (Done == 0)  //Controleer of het kalibreren  al is gebeurd
  {
    Serial.println("Wachten voor Kalibreren");
    while (digitalRead(PinStart) == 1) {} //Wacht tot de start knop wordt ingedrukt
    Serial.println("Kalibreren");
    TotalSteps(); //Start het totaal aantal stappen programma
    if (TotStepsYm < TotStepsYp) //Kijkt aan welke kant het aantal stappen in verticale richting het kleinst is
    {
      TotStepsY = TotStepsYm - 5000;
    } else if (TotStepsYp <= TotStepsYm)
    {
     TotStepsY = TotStepsYp - 5000;
    }
    TotStepsX = TotStepsX - 200;
    posX = 0; //Zet de positie op de start positie
    posY = 0;
    Serial.println(TotStepsX);
    Serial.println(TotStepsYp);
    Serial.println(TotStepsYm);
    Serial.println(TotStepsY);
    Done = 1;    //Variabele om te zeggen dat het totale aantal stappen gemeten is
  }
  else if (Done != 0) //Controleer of de kalibratie is gebeurd
  {
    Serial.println("Wachten voor Tekenen");
    while (digitalRead(PinStart) == 1) {} //Wacht tot de startknop is ingedrukt en uitgedrukt
    while (digitalRead(PinStart) == 0) {}
    Serial.println("Starten Tekenen");
    Step(LOW,DirX,StepX,100);
    Step(LOW,DirY,StepY,500);
    do  //Herhaal het tekenprogramma totdat het klaar is
    {
      Tekenen();  //Start het tekenprogramma
    } while (Done != 2);
    Serial.println("Tekening klaar");
  }
}