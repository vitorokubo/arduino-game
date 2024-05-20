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
#include "image.h"
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
bool Start = false;

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

struct Object{
  Position position;
  String direction;
  int duration;
  bool finished;
  // struct Bullet* nextBullet;
};

// class Ammo {
//   public:
//       int numberOfBullets;
//       Bullet firstBullet; 
//       Ammo(int nobullets) : numberOfBullets(nobullets), firstBullet(nullptr) {
//           for (int i = 0; i < nobullets; i++) {
//               insertBullet(&firstBullet->Nex);
//           }
//       }

//   Bullet* createBullet() {
//       Bullet* newBullet = (Bullet*)malloc(sizeof(Bullet));
//       newBullet->nextBullet = nullptr;
//       return newBullet;
//   }

//   void insertBullet(Bullet** firstBulletNode) {
//       Bullet* newBullet = createBullet();
//       if (*firstBulletNode == nullptr) {
//           *firstBulletNode = newBullet;
//           return;
//       }
//       Bullet* temp = *firstBulletNode;
//       while (temp->nextBullet != nullptr) {
//           temp = temp->nextBullet;
//       }
//       temp->nextBullet = newBullet;
//   }
// };


const int numberOfBullets = 5;
Object bullets[numberOfBullets];
const int numberOfAsteroids = 3;
Object asteroids[numberOfAsteroids];
const int numberOfColisions = 5;
Object colisions[numberOfColisions];
int points =0;
bool GameOver = false;

void initAsteroids(){
  for(int i =0; i<numberOfAsteroids;i++){
    asteroids[i].finished = true;
    asteroids[i].duration = 0;
  }
}

void initBullets(){
  for(int i = 0; i < numberOfBullets; i++){
  bullets[i].finished = true;
  bullets[i].duration = 0;
  }
}

void initColisions(){
  for(int i =0; i<numberOfColisions;i++){
    colisions[i].finished = true;
    colisions[i].duration = 0;
  }
}

class SpaceShip{
  public:
    Position atualPosition;
    String lastDirection;
    int movespeed;
    int bulletSpeed;
    Object bullets[5];
    SpaceShip(Position startposition = {120, 120},String startDirection = "up", int mspeed = 10, int bspeed = 10) 
    {
      atualPosition = startposition;
      lastDirection = startDirection;
      movespeed = mspeed;
      bspeed = bspeed;
    }

};
float angulo;
#define imageHeight 12
#define imageWidth 12
uint16_t shipResult[imageHeight * imageWidth];

uint16_t displayInt;
int imageNewHeight = 12;
int imageNewWidth = 12;
int firstRenderShip = true;

void setup() {
  initBullets();
  initAsteroids();  
  initColisions();
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

  /*Analog Joystick setup*/
  pinMode(x_pin,INPUT);
  pinMode(y_pin,INPUT);

  Serial.begin(9600);
  LR_neutral = analogRead(x_pin);
  UD_neutral = analogRead(y_pin);
  
  lcd.drawImageF(0,0,12,12,ship_up);
  lcd.drawImageF(12,0,12,12,ship_left);
  lcd.drawImageF(24,0,12,12,ship_right);
  lcd.drawImageF(36,0,12,12,ship_down);
  lcd.drawImageF(48,0,12,12,ship_left_up);
  lcd.drawImageF(60,0,12,12,ship_left_down);
  lcd.drawImageF(72,0,12,12,ship_right_up);
  lcd.drawImageF(84,0,12,12,ship_right_down);
  lcd.drawImageF(180,0,9,9,shot);
  lcd.drawImageF(105,0,20,20,meteor);
  lcd.drawImageF(125,0,20,20,explosion);


  lcd.drawImageF(80,80,72,53,title);
}

void loop() {
  if(!Start){
    lcd.setCursor(50, 150);
    lcd.print("             ");
    delay(100);
    lcd.setCursor(50, 150);
    lcd.print("HOLD TO START");
    delay(400);
    int shotButton = digitalRead(botaoAzul);
    if(shotButton == LOW) {
      Start = true;
      lcd.clearScreen();
    } 
  } else{ 
    int LR = analogRead(x_pin);
    int UD = analogRead(y_pin);
    String direction = readAnalogDirection(LR,UD);
    if(direction != "") lastDirection = direction;
    if(firstRenderShip){
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_up);
      firstRenderShip = false;
    }
    moveShip(direction);
    /*Handling Position*/  
    int shotButton = digitalRead(botaoAzul);
    if(shotButton == LOW) {
      createShot();
    }
    moveBullets();
    printBullets();
    spawnAsteroids();
    moveAsteroids();
    printAsteroids(); 
    detectColision();
    printColisions();

    float newAngulo = joystickAngle(LR,UD);
    // if(newAngulo != angulo){
    //   angulo = newAngulo;
    //   myImage = flipimage(12,12, angulo);
    //   lcd.drawImageF(100,100,imageNewWidth,imageNewHeight,myImage);
    // }
    // Serial.println(LR);
    // Serial.println(UD);
    // Serial.println(angulo);
    //  delay(60);
    
    if (GameOver) {
      lcd.clearScreen();
      lcd.setCursor(50, 50);
      lcd.println("GAME OVER");
      lcd.setCursor(50, 100);
      lcd.print("POINTS : ");
      lcd.print(points);
      while (GameOver) {
        lcd.setCursor(50, 150);
        lcd.print("HOLD START");
        lcd.setCursor(50, 170);
        lcd.print("TO PLAY AGAIN");
        delay(100); // Mudei o delay para 500 ms para uma piscada mais perceptível
        lcd.setCursor(50, 150);
        lcd.print("           ");
        lcd.setCursor(50, 170);
        lcd.print("             ");
        delay(400);
        int shotButton = digitalRead(botaoAzul); // Leitura do botão dentro do loop
        if (shotButton == LOW) {
            Serial.print("GAMEOVA");
            GameOver = false;
            Start = false;
            points = 0;
            firstRenderShip = true;
            atualPosition.x = 120;
            atualPosition.y = 120;
            lcd.clearScreen();
            lcd.drawImageF(80,80,72,53,title);
            initAsteroids();
            initColisions();
            initBullets();
        }
      }
    }
  }
}

// void rotateShipDown(uint16_t* rotatedShip) {
//   for (int i = 0; i < imageHeight; i++) {
//     for (int j = 0; j < imageWidth; j++) {
//       Serial.print(pgm_read_byte(&ship[(imageHeight - 1 - i) * imageWidth + j]));
//     }
//   }
// }

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
    for(int i =0; i<12; i++){
      for(int j=0; j<12; j++){
        lcd.drawPixel(atualPosition.x + i, atualPosition.y + j, BLACK);
      }
    }
    atualPosition.x = newPosition.x;
    atualPosition.y = newPosition.y;
    if (lastDirection == "up") {
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_up);
    } else if (lastDirection == "left-up") {
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_left_up);
    } else if (lastDirection == "left") {
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_left);
    } else if (lastDirection == "left-down") {
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_left_down);
    } else if (lastDirection == "down") {
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_down);
    } else if (lastDirection == "right-up") {
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_right_up);
    } else if (lastDirection == "right") {
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_right);
    } else if (lastDirection == "right-down") {
      lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_right_down);
    }
  }
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

void spawnAsteroids(){
  int randomValue = random(100);  // Gera um número aleatório entre 0 e 99
  if (randomValue < 60) {
    for(int i=0; i<numberOfAsteroids; i++){
      if(asteroids[i].finished == true){
        int randomValueForDirection = random(0,120)/20;
        String randomDirection;
        if (randomValueForDirection == 0) {
          randomDirection = "right";
        } else if (randomValueForDirection == 1) {
          randomDirection = "rigth-up";
        } else if (randomValueForDirection == 2) {
          randomDirection = "rigth-down";
        } else if (randomValueForDirection == 3) {
          randomDirection = "left";
        } else if (randomValueForDirection == 4) {
          randomDirection = "left-down";
          } else if (randomValueForDirection == 5) {
          randomDirection = "left-up";
        } else {
          randomDirection = "unknown"; 
        }
        Position spawnPosition;
        spawnPosition.x = random(0,240);
        spawnPosition.y = random(0,240);
        //escrever funcao que previne nascer no mesmo lugar que a nave
        Position newPosition = processMove(spawnPosition,randomDirection, 6);
        asteroids[i].direction = randomDirection;
        asteroids[i].duration = 20;
        asteroids[i].position.x = newPosition.x;
        asteroids[i].position.y = newPosition.y;
        asteroids[i].finished = false;
        return;
      }
    }
  }else {
  return;
  }
}

void moveAsteroids(){
 for(int i=0; i<numberOfAsteroids; i++){
    if(asteroids[i].finished == false){
      Position newPosition = processMove(asteroids[i].position, asteroids[i].direction, 6);
      for(int w =0; w<20; w++){
        for(int h=0; h<20; h++){
          lcd.drawPixel(asteroids[i].position.x+ w, asteroids[i].position.y + h, BLACK);
        }
      }
      asteroids[i].position.x = newPosition.x;
      asteroids[i].position.y = newPosition.y;
      asteroids[i].duration -= 1;
      if(asteroids[i].duration <= 0) asteroids[i].finished = true;
    }
  }
}

void printAsteroids(){
  for(int i=0; i<numberOfAsteroids; i++){
    if(asteroids[i].finished == false){
      lcd.drawImageF(asteroids[i].position.x,asteroids[i].position.y,20,20,meteor);
    }
  }
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
      for(int w=0; w<9; w++){
        for(int h=0; h<9; h++){
          lcd.drawPixel(bullets[i].position.x + w, bullets[i].position.y + h, BLACK);
        }
      }
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
      lcd.drawImageF(bullets[i].position.x,bullets[i].position.y,9,9,shot);
    }
  }
}

void detectColision(){
  for(int i=0; i<numberOfAsteroids; i++){
    if(asteroids[i].finished == false){
      if (!(asteroids[i].position.x + 9 < atualPosition.x ||
        asteroids[i].position.x > atualPosition.x + 12 ||
        asteroids[i].position.y + 9 < atualPosition.y ||
        asteroids[i].position.y > atualPosition.y + 12)){
        GameOver = true;
        for(int w=0; w<12; w++){
          for(int h=0; h<12; h++){
            lcd.drawPixel(asteroids[i].position.x + w, asteroids[i].position.y + h, BLACK);
          }
        }
        for(int w=0; w<20; w++){
          for(int h=0; h<20; h++){
            lcd.drawPixel(atualPosition.x + w, atualPosition.y + h, BLACK);
          }
        }
        for(int k=0; k<numberOfColisions; k++){
          if(colisions[k].finished == true){
            colisions[k].duration = 5;
            colisions[k].position.x = asteroids[i].position.x;
            colisions[k].position.y = asteroids[i].position.y;
            colisions[k].finished = false;
            break;
          }
        }
        for(int k=0; k<numberOfColisions; k++){
          if(colisions[k].finished == true){
            colisions[k].duration = 5;
            colisions[k].position.x = atualPosition.x;
            colisions[k].position.y = atualPosition.y;
            colisions[k].finished = false;
            break;
          }
        }
      }


      for(int j=0; j<numberOfBullets; j++){
        if(bullets[j].finished == false){
          int impactRangeX = 12;
          int impactRangeY = 9;
          if (!(bullets[j].position.x + 9 < asteroids[i].position.x ||
                bullets[j].position.x > asteroids[i].position.x + 20 ||
                bullets[j].position.y + 9 < asteroids[i].position.y ||
                bullets[j].position.y > asteroids[i].position.y + 20))  
              {
              bullets[j].finished = true;
              bullets[j].duration = 0;
              asteroids[i].finished = true;
              asteroids[i].duration = 0;
              for(int w=0; w<9; w++){
                for(int h=0; h<9; h++){
                  lcd.drawPixel(bullets[j].position.x + w, bullets[j].position.y + h, BLACK);
                }
              }
              for(int w=0; w<20; w++){
                for(int h=0; h<20; h++){
                  lcd.drawPixel(asteroids[i].position.x + w, asteroids[i].position.y + h, BLACK);
                }
              }
              for(int k=0; k<numberOfColisions; k++){
                if(colisions[k].finished == true){
                  colisions[k].duration = 5;
                  colisions[k].position.x = asteroids[i].position.x;
                  colisions[k].position.y = asteroids[i].position.y;
                  colisions[k].finished = false;
                  break;
                }
              }
              points += 100;
              continue;
            }
          }
        }
      }
    }
  }


void printColisions(){
  for(int i=0; i<numberOfColisions; i++){
    lcd.setCursor(colisions[i].position.x,colisions[i].position.y);
    if(colisions[i].finished == false && colisions[i].duration > 0 ){
      lcd.drawImageF(colisions[i].position.x,colisions[i].position.y,20,20,explosion);
      colisions[i].duration--;
      continue;
    }
    if(colisions[i].duration <= 0){
      for(int w=0; w<20; w++){
        for(int h=0; h<20; h++){
          lcd.drawPixel(colisions[i].position.x + w, colisions[i].position.y + h, BLACK);
        }
      }
      colisions[i].finished = true;
    }
  }
}

// uint16_t* flipimage(int width, int height, float angle) {
//   double theta = angle;
//   double cosTheta = cos(theta);
//   double sinTheta = sin(theta);
//   double tangentForShears = tan(theta/2);

//   imageNewHeight = round(fabs(width * sinTheta) + fabs(height * cosTheta));
//   imageNewWidth = round(fabs(height * cosTheta) + fabs(width * sinTheta));
//   uint16_t* newImage = (uint16_t*)malloc(imageNewWidth * imageNewHeight * sizeof(uint16_t));

//   for(int i=0; i< imageNewWidth*imageNewHeight; i++){
//     newImage[i] = 0x0000;
//   }
//   // Aqui você pode fazer o processamento da imagem
//   for(int i=0; i<height*width-1; i++ ){
//     uint16_t shipPart = ship[i];
//     int x = i%width;
//     int y = i/width-x;
//     //first shear
//     int new_x = round(x-y*tangentForShears);
//     int new_y = y;
//     //second shear
//     new_y = round((new_x*sinTheta)+new_y);
//     //third shear
//     new_x = round((new_x-(new_y*tangentForShears)));
    
//     newImage[new_x*new_y]=shipPart;
//   }

  // Serial.print("Width: ");
  // Serial.println(imageNewWidth);
  // Serial.print("Height: ");
  // Serial.println(imageNewHeight);
  // Retornar o ponteiro para o novo array
//   return newImage;
// }

void analogtracker(){
  Serial.print("X-axis:  ");
  Serial.print(analogRead(x_pin));
  Serial.print("\n");
  Serial.print("Y-axis:  ");
  Serial.print(analogRead(y_pin));
  Serial.print("\n");
}
