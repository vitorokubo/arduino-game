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

// Inclusão de bibliotecas respectivamente: math.h (funções matematicas), biblioteca para gerar as imagens do jogo (nave, asteroides, explosoes, etc)
//adafruit.h é a biblioteca para o controle grafico e Arduino_ST7789_Fast.h para o controle da lcd se fazendo necessário a adafruit
#include <math.h>
#include "image.h"
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>
#define TFT_DC    7
#define TFT_RST   8 
#define SCR_WD   240
#define SCR_HT   240   // 320 - to allow access to full 240x320 frame buffer

//inicalizando o controle da tela lcd
Arduino_ST7789 lcd = Arduino_ST7789(TFT_DC, TFT_RST);
uint16_t bgCol    = RGBto565(160,160,160);

/*Analog Joystick config*/
//biblioteca wire.h para se comunicar com dispostivos I2C
#include <Wire.h>
#define x_pin A1
#define y_pin A2

//setando variaveis de tamanho e velocidade de objetos
int LR_neutral;
int UD_neutral;
int deadzone = 100;
int movespeed = 10;
int bulletspeed = 10;
bool Start = false;

//struct para a posição
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

//struct de tipos de dados para o controle de objetos
struct Object{
  Position position;
  String direction;
  int duration;
  bool finished;
};

class Game{
  public:
  bool Start = false;
  int firstRenderShip = true;
  int score = 0;
  bool GameOver = false;
};
//Funções a baixo resumo 
//criando o tipo de dado para o movimento a partir do Position
Position processMove(Position atualPosition, String direction, int movespeed) {
  Position result = atualPosition; // Inicializa a posição de retorno com a posição atual
  // verifica se não está indo na posição de mesmo nome, se caso verdadeiro, faz parar, do contrario adiciona deslocamento naquela direção
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
//Assim como o nome diz, movimento os objetos na tela, pegando o objeto em si, e o redesenhando na tela somando a posição cartesiana ao vetor unitário de posição.
void moveObjects(Object objects[], int numberOfObjects, int objectWidth, int objectHeight, int move) {
  for (int i = 0; i < numberOfObjects; i++) {
    if (!objects[i].finished) {
      Position newPosition = processMove(objects[i].position, objects[i].direction, move);
      for (int x = 0; x < objectWidth; x++) {
        for (int y = 0; y < objectHeight; y++) {
            lcd.drawPixel(objects[i].position.x + x, objects[i].position.y + y, BLACK);
        }
      }
      objects[i].position.x = newPosition.x;
      objects[i].position.y = newPosition.y;
      objects[i].duration -= 1;
      if (objects[i].duration <= 0) objects[i].finished = true;
    }
  }
}
//"desenha" os objetos na tela conforme a posção cartesiana
void printObjects(Object objects[], int numObjects, int objectWidth, int objectHeight, const uint16_t* image) {
  for (int i = 0; i < numObjects; i++) {
    if (!objects[i].finished) {
      lcd.drawImageF(objects[i].position.x, objects[i].position.y, objectWidth, objectHeight, image);
    }
  }
}
void initObjects(Object objects[], int numObjects) {
  for (int i = 0; i < numObjects; i++) {
      objects[i].finished = true;
      objects[i].duration = 0;
  } 
}
//classe para o objeto "nave"
class SpaceShip{
  public:
    int Width = 12;
    int Height = 12;
    Position atualPosition;
    String lastDirection;
    int movespeed;
    int bulletSpeed;
    Object bullets[5];
    //monta a estrutura da nave
    SpaceShip(Position startposition = {120, 120},String startDirection = "up", int mspeed = 10, int bspeed = 10) 
    {
      atualPosition = startposition;
      lastDirection = startDirection;
      movespeed = mspeed;
      bspeed = bspeed;
    }
    //metodo para movimentar a nave pela tela
    void MoveShipPosition(String direction){
      Position newPosition = processMove(atualPosition, direction, movespeed);
      if(newPosition.x != atualPosition.x || newPosition.y != atualPosition.y){
        for(int i =0; i<Width; i++){
          for(int j=0; j<Height; j++){
            lcd.drawPixel(atualPosition.x + i, atualPosition.y + j, BLACK);
          }
        }
        atualPosition.x = newPosition.x;
        atualPosition.y = newPosition.y;
        //condições para desenhar na tela conforme a posição da nave
        if (lastDirection == "up") {
          lcd.drawImageF(atualPosition.x, atualPosition.y,Width,Height,ship_up);
        } else if (lastDirection == "left-up") {
          lcd.drawImageF(atualPosition.x, atualPosition.y,Width,Height,ship_left_up);
        } else if (lastDirection == "left") {
          lcd.drawImageF(atualPosition.x, atualPosition.y,Width,Height,ship_left);
        } else if (lastDirection == "left-down") {
          lcd.drawImageF(atualPosition.x, atualPosition.y,Width,Height,ship_left_down);
        } else if (lastDirection == "down") {
          lcd.drawImageF(atualPosition.x, atualPosition.y,Width,Height,ship_down);
        } else if (lastDirection == "right-up") {
          lcd.drawImageF(atualPosition.x, atualPosition.y,Width,Height,ship_right_up);
        } else if (lastDirection == "right") {
          lcd.drawImageF(atualPosition.x, atualPosition.y,Width,Height,ship_right);
        } else if (lastDirection == "right-down") {
          lcd.drawImageF(atualPosition.x, atualPosition.y,Width,Height,ship_right_down);
        }
      }
    }
    // void DrawShip(xposition, yposition){
    //       lcd.drawImageF(atualPosition.x, atualPosition.y,12,12,ship_up);
    // }
};
//setando a coleçao asteroid
class AsteroidsCollection{
  public:
  int Width = 20;
  int Height = 20;
  int movespeed = 6;
  int numberOfAsteroids = 3;
  Object asteroids[3];
  //iniciando o metodo de inicio para os asteroides aparecerem na tela
  AsteroidsCollection(int mspeed = 6) 
  {
    movespeed = mspeed;
    initAsteroids();
  }
  initAsteroids(){
    initObjects(asteroids, numberOfAsteroids);
  }
  //metodo par iniciar os ateroides de aparecerem na tela randomicamente
  void spawnAsteroids(){
    int randomValue = random(100);  // Gera um número aleatório entre 0 e 99
    if (randomValue < 60) {
      //iteração para gerar os asteroides aleatoriamente numa posição e movimento aleatorios
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
    moveObjects(asteroids,numberOfAsteroids,Width,Height, movespeed);
  }
   printAsteroids(){
    printObjects(asteroids, numberOfAsteroids, Width, Height, meteor);
  }
};

//classe do objeto de tiros da nave
class BulletsCollection{
  public:
  int Width = 9;
  int Height = 9;
  int duration;
  int movespeed;
  int numberOfBullets = 5;
  Object bullets[5];
  BulletsCollection(int durate=8, int mspeed = 10){
    duration = durate;
    movespeed = mspeed;
    initBullets();
  }
  initBullets(){
    initObjects(bullets, numberOfBullets);
  }
  printBullets(){
    printObjects(bullets, numberOfBullets, Width, Height, shot);
  }
  //metodo para criar a bala conforme a posição/direção da nave
  void createShot(SpaceShip ship){
    for(int i=0; i<numberOfBullets; i++){
      if(bullets[i].finished == true){
          Position newPosition = processMove(ship.atualPosition,ship.lastDirection, movespeed);
          bullets[i].direction = ship.lastDirection;
          bullets[i].duration = duration;
          bullets[i].position.x = newPosition.x;
          bullets[i].position.y = newPosition.y;
          bullets[i].finished = false;
          return;
      }
    }
  }
  void moveBullets(){
    moveObjects(bullets,numberOfBullets,Width,Height, movespeed );
  }
};
//classe para colisão 
class ColisionCollection{
  public:
  int Width = 20;
  int Height = 20;
  int duration = 5;
  int numberOfColisions = 6;
  Object colisions[6];
  ColisionCollection(int time = 5){
    duration = time;
    initColisions();
  }
  initColisions(){
    initObjects(colisions, numberOfColisions);
  }
  //metodo para detectar as colisões conforme a posição e tamanho dos objetos
  void detectColision(Game& game, AsteroidsCollection& asteroids, BulletsCollection& bullets, SpaceShip& spaceShip){
    for(int i=0; i<asteroids.numberOfAsteroids; i++){
      if(asteroids.asteroids[i].finished == false){
        if (!(asteroids.asteroids[i].position.x + asteroids.Width < spaceShip.atualPosition.x ||
          asteroids.asteroids[i].position.x > spaceShip.atualPosition.x + spaceShip.Width ||
          asteroids.asteroids[i].position.y + asteroids.Height < spaceShip.atualPosition.y ||
          asteroids.asteroids[i].position.y > spaceShip.atualPosition.y + spaceShip.Height)){
          game.GameOver = true;
          for(int w=0; w<spaceShip.Width; w++){
            for(int h=0; h<spaceShip.Height; h++){
              lcd.drawPixel(asteroids.asteroids[i].position.x + w, asteroids.asteroids[i].position.y + h, BLACK);
            }
          }
          for(int w=0; w<asteroids.Width; w++){
            for(int h=0; h<asteroids.Height; h++){
              lcd.drawPixel(atualPosition.x + w, atualPosition.y + h, BLACK);
            }
          }
          for(int k=0; k<numberOfColisions; k++){
            if(colisions[k].finished == true){
              colisions[k].duration = 5;
              colisions[k].position.x = asteroids.asteroids[i].position.x;
              colisions[k].position.y = asteroids.asteroids[i].position.y;
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
        //iterando a posição da bala com a posição do asteroide para criar a colisão
        for(int j=0; j<bullets.numberOfBullets; j++){
          if(bullets.bullets[j].finished == false){
            int impactRangeX = 12;
            int impactRangeY = 9;
            if (!(bullets.bullets[j].position.x + bullets.Width < asteroids.asteroids[i].position.x ||
                  bullets.bullets[j].position.x > asteroids.asteroids[i].position.x + asteroids.Width ||
                  bullets.bullets[j].position.y + bullets.Height < asteroids.asteroids[i].position.y ||
                  bullets.bullets[j].position.y > asteroids.asteroids[i].position.y + asteroids.Height))  
                {
                bullets.bullets[j].finished = true;
                bullets.bullets[j].duration = 0;
                asteroids.asteroids[i].finished = true;
                asteroids.asteroids[i].duration = 0;
                for(int w=0; w<9; w++){
                  for(int h=0; h<9; h++){
                    lcd.drawPixel(bullets.bullets[j].position.x + w, bullets.bullets[j].position.y + h, BLACK);
                  }
                }
                for(int w=0; w<20; w++){
                  for(int h=0; h<20; h++){
                    lcd.drawPixel(asteroids.asteroids[i].position.x + w, asteroids.asteroids[i].position.y + h, BLACK);
                  }
                }
                for(int k=0; k<numberOfColisions; k++){
                  if(colisions[k].finished == true){
                    colisions[k].duration = 5;
                    colisions[k].position.x = asteroids.asteroids[i].position.x;
                    colisions[k].position.y = asteroids.asteroids[i].position.y;
                    colisions[k].finished = false;
                    break;
                  }
                }
                game.score += 100;
                continue;
              }
            }
          }
        }
      }
  }
  //função para criar o efeito de explosão ao colidir e sumir com o objeto da tela
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
};

SpaceShip ship;
BulletsCollection bullets;
AsteroidsCollection asteroids;
ColisionCollection colisions;
Game game;
//inicio do programa
void setup() {
  /*Buttons setup*/
  pinMode(botaoAzul,INPUT_PULLUP);
  pinMode(botaoVermelho,INPUT_PULLUP);

  /*Display setup*/
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
  //desenha os objetos todos
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

//loop de verificação a cada 
void loop() {
  // se o jogo nao tiver começado inicia a tela inicial para apertar o botao de start
  if(!game.Start){
    lcd.setCursor(50, 150);
    lcd.print("             ");
    delay(100);
    lcd.setCursor(50, 150);
    lcd.print("HOLD TO START");
    delay(400);
    int shotButton = digitalRead(botaoAzul);
    if(shotButton == LOW) {
      game.Start = true;
      lcd.clearScreen();
    } 
    // se ja tiver começado inicializa todos os objetos e metodos para executar o jogo
  } else{ 
    int LR = analogRead(x_pin);
    int UD = analogRead(y_pin);
    String direction = readAnalogDirection(LR,UD);
    if(direction != "") ship.lastDirection = direction;
    if(game.firstRenderShip){
      lcd.drawImageF(ship.atualPosition.x, ship.atualPosition.y, ship.Width,ship.Height, ship_up);
      game.firstRenderShip = false;
    }
    ship.MoveShipPosition(direction);
    /*Handling Position*/  
    int shotButton = digitalRead(botaoAzul);
    if(shotButton == LOW) {
      bullets.createShot(ship);
    }
    bullets.moveBullets();
    bullets.printBullets();
      asteroids.spawnAsteroids();
      asteroids.moveAsteroids();
      asteroids.printAsteroids(); 
      colisions.detectColision(game,asteroids, bullets, ship);
      colisions.printColisions();
      if (game.GameOver) {
        lcd.clearScreen();
        lcd.setCursor(50, 50);
        lcd.println("GAME OVER");
        lcd.setCursor(50, 100);
        lcd.print("SCORE : ");
        lcd.print(game.score);
        while (game.GameOver) {
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
              game.GameOver = false;
              game.Start = false;
              game.score = 0;
              game.firstRenderShip = true;
              ship.atualPosition.x = 120;
              ship.atualPosition.y = 120;
              lcd.clearScreen();
              lcd.drawImageF(80,80,72,53,title);
              asteroids.initAsteroids();
              colisions.initColisions();
              bullets.initBullets();
          }
        }
      }
  } 
}
//pega a direção do angulo do joystick
float joystickAngle(int x, int y) {
  float deltaX = x - LR_neutral;
  float deltaY = y - UD_neutral;
  double rad = atan2 (deltaY, deltaX * -1); // In radians
  double deg = rad * (180/M_PI);
  if(deg < 0) {
    deg += 360; // Adiciona 360 para que o intervalo seja de 0 a 360
  }
  return deg;
}

//pega a direção e joga para o result para fazer a nave se moverimage.png
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

void analogtracker(){
  Serial.print("X-axis:  ");
  Serial.print(analogRead(x_pin));
  Serial.print("\n");
  Serial.print("Y-axis:  ");
  Serial.print(analogRead(y_pin));
  Serial.print("\n");
}
