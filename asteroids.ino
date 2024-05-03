/*
 ST7789 240x240 IPS (without CS pin) connections (only 6 wires required):

 #01 GND -> GND
 #02 VCC -> VCC (3.3V only!)
 #03 SCL -> D13/SCK
 #04 SDA -> D11/MOSI
 #05 RES -> D8 or any digital
 #06 DC  -> D7 or any digital
 #07 BLK -> NC
*/
#include <math.h>
#include "ship.h"

#define TFT_DC    7
#define TFT_RST   8 
#define SCR_WD   240
#define SCR_HT   240   // 320 - to allow access to full 240x320 frame buffer
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>
Arduino_ST7789 lcd = Arduino_ST7789(TFT_DC, TFT_RST);
uint16_t bgCol    = RGBto565(160,160,160);

/*Analog Joystick config*/
#include <Wire.h>
#define x_pin A1
#define y_pin A2
int LR_neutral;
int UD_neutral;
int deadzone = 100;
int movespeed = 10;
int bulletspeed = 10;

struct Position{
    int x;
    int y;
};

/*Buttons config*/
int botaoAzul = 4;
int botaoVermelho = 2;
int duration = 8;

Position atualPosition;
String lastDirection = "up";

struct Bullet{
  Position position;
  String direction;
  int duration;
  bool finished;
  struct Bullet* nextBullet;
};
class Ammo {
  public:
      int numberOfBullets;
      Bullet firstBullet; 
      Ammo(int nobullets) : numberOfBullets(nobullets), firstBullet(nullptr) {
          for (int i = 0; i < nobullets; i++) {
              insertBullet(&firstBullet->Nex);
          }
      }

  Bullet* createBullet() {
      Bullet* newBullet = (Bullet*)malloc(sizeof(Bullet));
      newBullet->nextBullet = nullptr;
      return newBullet;
  }

  void insertBullet(Bullet** firstBulletNode) {
      Bullet* newBullet = createBullet();
      if (*firstBulletNode == nullptr) {
          *firstBulletNode = newBullet;
          return;
      }
      Bullet* temp = *firstBulletNode;
      while (temp->nextBullet != nullptr) {
          temp = temp->nextBullet;
      }
      temp->nextBullet = newBullet;
  }
};


const int numberOfBullets = 5;
Bullet bullets[numberOfBullets];

void initBullets(){
  for(int i = 0; i < numberOfBullets; i++){
  bullets[i].finished = true;
  bullets[i].duration = 0;
  }
}

class SpaceShip{
  public:
    Position atualPosition;
    String lastDirection;
    int movespeed;
    int bulletSpeed;
    Bullet bullets[5];
    SpaceShip(Position startposition = {120, 120},String startDirection = "up", int mspeed = 10, int bspeed = 10) 
    {
      atualPosition = startposition;
      lastDirection = startDirection;
      movespeed = mspeed;
      bspeed = bspeed;
    }

};


void setup() {
  initBullets();  
  /*Buttons setup*/
  pinMode(botaoAzul,INPUT_PULLUP);
  pinMode(botaoVermelho,INPUT_PULLUP);

  /*Display setup*/
  atualPosition.x = 120;
  atualPosition.y = 120;
  lcd.init(SCR_WD, SCR_HT);
  lcd.fillScreen(BLACK);
  lcd.setTextColor(WHITE,BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(atualPosition.x, atualPosition.y);
  lcd.println("^");

  /*Analog Joystick setup*/
  pinMode(x_pin,INPUT);
  pinMode(y_pin,INPUT);

  Serial.begin(9600);
  LR_neutral = analogRead(x_pin);
  UD_neutral = analogRead(y_pin);
  
  lcd.drawImageF(0,0,32,32,ship);
}

void loop() {
  int LR = analogRead(x_pin);
  int UD = analogRead(y_pin);
  String direction = readAnalogDirection(LR,UD);
  if(direction != "") lastDirection = direction;
  moveShip(direction);
  /*Handling Position*/  
  int shotButton = digitalRead(botaoAzul);
  if(shotButton == LOW) {
    createShot();
  }
  moveBullets();
  printBullets();
  
  float angulo = joystickAngle(LR,UD);
  Serial.println(LR);
  Serial.println(UD);
  Serial.println(angulo);
  delay(100);
}

float joystickAngle(int x, int y) {
  
  float deltaX = x - LR_neutral;
  float deltaY = y - UD_neutral;
  double rad = atan2 (deltaY, deltaX * -1); // In radians
  double deg = rad * (180/M_PI);

   if (deg < 0) {
        deg += 360; // Adiciona 360 para que o intervalo seja de 0 a 360
    }
  return deg;
}

void moveShip(String direction){
  Position newPosition = processMove(atualPosition, direction, movespeed);
  if(newPosition.x != atualPosition.x || newPosition.y != atualPosition.y){
    lcd.setCursor(atualPosition.x, atualPosition.y);
    lcd.println(" ");
    atualPosition.x = newPosition.x;
    atualPosition.y = newPosition.y;
    lcd.setCursor(atualPosition.x, atualPosition.y);
    String directionSymbol = changeShipOrientation();
    lcd.println(directionSymbol);
  }
}

String changeShipOrientation(){
  String directionSymbol;
    if (lastDirection == "up") {
      directionSymbol = "^";
    } else if (lastDirection == "left-up") {
      directionSymbol = "<";
    } else if (lastDirection == "left") {
      directionSymbol = "<";
    } else if (lastDirection == "left-down") {
      directionSymbol = "<";
    } else if (lastDirection == "down") {
      directionSymbol = "V";
    } else if (lastDirection == "right-up") {
      directionSymbol = ">";
    } else if (lastDirection == "right") {
      directionSymbol = ">";
    } else if (lastDirection == "right-down") {
      directionSymbol = ">";
    }
    return directionSymbol;
}

String readAnalogDirection(int LR, int UD){
  String result = "";
  
  if(LR > LR_neutral+deadzone){
    if(result != "") result += "-" ;
    result += "left";
  }
  if(LR < LR_neutral-deadzone){
    if(result != "") result += "-" ;
    result += "right";
  }
  if(UD > UD_neutral+deadzone){
    if(result != "") result += "-" ;
    result += "up";
  }
  if(UD < UD_neutral-deadzone){
    if(result != "") result += "-" ;
    result += "down";
  }
  if(LR == LR_neutral && UD == UD_neutral){
    if(result != "") result += "-" ;
    result = "";
  }
  return result;
}

Position processMove(Position atualPosition, String direction, int movespeed) {
  Position result = atualPosition; // Inicializa a posição de retorno com a posição atual
  if (direction.indexOf("left") != -1) {
    if (result.x <= 0) {
      result.x = 240;
    } else {
      result.x -= movespeed;
    }
  }
  if (direction.indexOf("right") != -1) {
    if (result.x >= 240) {
      result.x = 0;
    } else {
      result.x += movespeed;
    }
  }
  if (direction.indexOf("down") != -1) {
    if (result.y >= 240) {
      result.y = 0;
    } else {
      result.y += movespeed;
    }
  }
  if (direction.indexOf("up") != -1) {
    if (result.y <= 0) {
      result.y = 240;
    } else {
      result.y -= 3 + movespeed;
    }
  }
  return result;
}


void createShot(){
    for(int i=0; i<numberOfBullets; i++){
      if(bullets[i].finished == true){
          Position newPosition = processMove(atualPosition,lastDirection, bulletspeed);
          bullets[i].direction = lastDirection;
          bullets[i].duration = duration;

          bullets[i].position.x = newPosition.x;
          bullets[i].position.y = newPosition.y;
          bullets[i].finished = false;
          return;
      }
    }
}

void moveBullets(){
  for(int i=0; i<numberOfBullets; i++){
    if(bullets[i].finished == false){
      Position newPosition = processMove(bullets[i].position, bullets[i].direction, bulletspeed);
      lcd.setCursor(bullets[i].position.x, bullets[i].position.y);
      lcd.println(" ");
      bullets[i].position.x = newPosition.x;
      bullets[i].position.y = newPosition.y;
      bullets[i].duration -= 1;
      if(bullets[i].duration <= 0) bullets[i].finished = true;
    }
  }
}

void printBullets(){
  for(int i=0; i<numberOfBullets; i++){
    if(bullets[i].finished == false){
      lcd.setCursor(bullets[i].position.x,bullets[i].position.y);
      lcd.println("o");
    }
  }
}

void analogtracker(){
  Serial.print("X-axis:  ");
  Serial.print(analogRead(x_pin));
  Serial.print("\n");
  Serial.print("Y-axis:  ");
  Serial.print(analogRead(y_pin));
  Serial.print("\n");
}
