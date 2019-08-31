//////////////////////////////////////////////////////////////////////////
//  A Simple Game of Snake
//  written by Tyler Edwards for the badge created in Hackerbox 0020,
//  but should work on any ESP32 and Adafruit ILI9341 screen
//  Tyler on GitHub: https://github.com/HailTheBDFL
//  Hackerboxes: http://www.hackerboxes.com/
//  To begin the game, press the select/start/fire/A button on HB badge (default pin 15)
//========================================================================
// SNAKE with M5STACK : 2018.01.14 Transplant by macsbug
// Controller: LEFT=Buttons A,     RIGHT=B,     START/RETRY=C
// Controller: UP  =Buttons A + C, DOWN =B + C
// Github    : https://macsbug.wordpress.com/2018/01/14/esp32-snake-with-m5stack/
//========================================================================
// SNAKE with Odroid-GO : 2019.08.26 Transplant by micutil
// Controller: Up/Down/Left/Right = cross key     START/RETRY=C
// Controller: Inisial speed = Up/Down of cross key
// You need install chimera library for odroid-go.
// https://github.com/tobozo/ESP32-Chimera-Core
//========================================================================

#include <M5Stack.h>
#include "M5StackUpdater.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

float gameSpeed = 3;       //Higher numbers are faster
float plusSpeed = 0;
int threshold = 40;        //threshold for touch
boolean doStart = false;     //will not start without say-so
unsigned long offsetT = 0; //time delay for touch
unsigned long offsetM = 0; //time delay for main loop
float gs;
int headX = 1;             //coordinates for head
int headY = 1;
int beenHeadX[470];        //coordinates to clear later
int beenHeadY[470];
int changeX = 0;           //the direction of the snake
int changeY = 1;
boolean lastMoveH = false; //to keep from going back on oneself
int score = 1;
int foodX;                 //coordinates of food
int foodY;
boolean eaten = true;      //if true a new food will be made
int loopCount = 0;         //number of times the loop has run
int clearPoint = 0;        //when the loopCount is reset
boolean clearScore = false;
//========================================================================
void changeGameSpeed(float a, float b) {
  gameSpeed+=a;if(gameSpeed<1.0) gameSpeed=1.0;
  plusSpeed+=b;
  float gss=gameSpeed+plusSpeed;
  gs = 1000 / (gameSpeed+plusSpeed);     //calculated gameSpeed in milliseconds
  printSpeed();
}
//========================================================================
#ifdef ARDUINO_ODROID_ESP32
void drawInitalInfo() {
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 130);
  M5.Lcd.print("with A,B,Start or Selet");
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(20, 160);
  M5.Lcd.print("Change initial speed:");
  M5.Lcd.setCursor(100, 178);
  M5.Lcd.print("Push Up/Down key");
  M5.Lcd.setTextColor(BLACK);//戻す
}
#endif
//========================================================================
void setup() {
  gs = 1000 / gameSpeed;     //calculated gameSpeed in milliseconds
  memset(beenHeadX, 0, 470); //initiate beenHead with a bunch of zeros
  memset(beenHeadY, 0, 470);
  
  M5.begin();
  Wire.begin();
  if(digitalRead(BUTTON_A_PIN) == 0) {updateFromFS(SD);ESP.restart();}

  M5.Lcd.setBrightness(200);    // BRIGHTNESS = MAX 255
  M5.Lcd.fillScreen(WHITE);     // CLEAR SCREEN Original is BLACK
  M5.Lcd.fillRect(3, 21, 316, 226, BLUE); //deletes the old game
  
  //M5.Lcd.setRotation(0);
  //M5.Lcd.fillScreen(ILI9341_BLACK);
  
  M5.Lcd.setTextColor(0x5E85);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setCursor(70, 90);
  M5.Lcd.print(">START<");
  
  M5.Lcd.setTextColor(BLACK); //Score keeper
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(5, 3);
  M5.Lcd.print("Length: ");
  printScore();

  M5.Lcd.setCursor(165, 3);
  M5.Lcd.print("Speed: ");
  changeGameSpeed(0,0);
  
  #ifdef ARDUINO_ODROID_ESP32
  drawInitalInfo();
  #endif
  
  randomSeed(analogRead(26));        //make every game unique
}
//========================================================================
void loop() {
  if(doStart) {
    //Control
    #ifndef ARDUINO_ODROID_ESP32
    if(M5.BtnA.isPressed() && M5.BtnC.isPressed()){doUp  ();}
    if(M5.BtnB.isPressed() && M5.BtnC.isPressed()){doDown();}
    if(M5.BtnA.isPressed()){doLeft  ();}
    if(M5.BtnB.isPressed()){doRight ();}
    #else
    int jxy=M5.JOY_X.isAxisPressed();
    if(jxy==1) {doRight();} else if(jxy==2) {doLeft();}
    jxy=M5.JOY_Y.isAxisPressed();
    if(jxy==1) {doDown();} else if(jxy==2) {doUp();}
    #endif
  } else {
    //Start
    #ifndef ARDUINO_ODROID_ESP32
    if(M5.BtnC.wasPressed()){select();}
    #else
    if(M5.BtnStart.isPressed()||
       M5.BtnSelect.isPressed()||
       M5.BtnA.isPressed()||
       M5.BtnB.isPressed()
    ) {
      doSelect();
    } else {
      int jxy=M5.JOY_Y.wasAxisPressed();
      if(jxy==1) {changeGameSpeed(-1,0); printSpeed();} 
      else if(jxy==2) {changeGameSpeed(1,0); printSpeed();}
    }
    #endif
  }
  
  if (clearScore and doStart) { //resets score from last game, won't clear
    score = 1;                //until new game starts so you can show off
    printScore();             //your own score
    plusSpeed=0;changeGameSpeed(0,0);
    clearScore = false;
  }
  
  if (millis() - offsetM > gs and doStart) {
    beenHeadX[loopCount] = headX;  //adds current head coordinates to be
    beenHeadY[loopCount] = headY;  //covered later   
    headX = headX + (changeX);  //head moved
    headY = headY + (changeY);    
    if (headX - foodX == 0 and headY - foodY == 0) { //food
      score += 1;
      printScore();
      if(score%10==0) {
        changeGameSpeed(0,2);
      }
      eaten = true;
    }
    loopCount += 1; //loopCount used for addressing, mostly    
    if (loopCount > 467) {            //if loopCount exceeds size of
      clearPoint = loopCount - score; //beenHead arrays, reset to zero
      loopCount = 0;
    }   
    drawDot(headX, headY);           //head is drawn   
    if (loopCount - score >= 0) {    //if array has not been reset
      eraseDot(beenHeadX[loopCount - score], beenHeadY[loopCount - score]);
    }  //covers end of tail
    else {
      eraseDot(beenHeadX[clearPoint], beenHeadY[clearPoint]);
      clearPoint += 1;
    }
    if (eaten) {     //randomly create a new piece of food if last was eaten
      foodX = random(2, 26);
      foodY = random(2, 18);
      eaten = false;
    }
    drawDotRed(foodX, foodY); //draw the food
    if (headX > 26 or headX < 1 or headY < 1 or headY > 18) { //Boudaries
      endGame();
    }
    if (loopCount - score < 0) {         //check to see if head is on tail
      for (int j = 0; j < loopCount; j++) {
        if (headX == beenHeadX[j] and headY == beenHeadY[j]) {
          endGame();
        }
      }
      for (int k = clearPoint; k < 467; k++) {
        if (headX == beenHeadX[k] and headY == beenHeadY[k]) {
          endGame();
        }
      }
    }
    else {
      for (int i = loopCount - (score - 1); i < loopCount; i++) {
        if (headX == beenHeadX[i] and headY == beenHeadY[i]) {
          endGame();
        }
      }
    }   
    offsetM = millis(); //reset game loop timer
  }
  M5.update();
  delay(1);
}
//========================================================================
void endGame() {
  M5.Lcd.fillRect(3, 21, 316, 226, BLUE); //deletes the old game
  eaten = true;                           //new food will be created 
  M5.Lcd.setCursor(80, 90);               //Retry message
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.print("RETRY?");
  M5.Lcd.setTextColor(BLACK); //sets back to scoreboard settings
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(5, 3);
  M5.Lcd.print("Length: ");
  headX = 1;                 //reset snake
  headY = 1;
  changeX = 0;
  changeY = 1;
  lastMoveH = false;
  memset(beenHeadX, 0, 470); //clear the beenHead arrays
  memset(beenHeadY, 0, 470); //probably not necessary
  loopCount = 0;
  clearScore = true;
  doStart = false;             //stops game

  #ifdef ARDUINO_ODROID_ESP32
  drawInitalInfo();
  #endif
}
//========================================================================
void drawDot(int x, int y) {
  M5.Lcd.fillRect(12*(x-1)+5, 12*(y-1)+23, 10, 10, WHITE);
}
//========================================================================
void drawDotRed(int x, int y) {
  M5.Lcd.fillRect(12*(x-1)+5, 12*(y-1)+23, 10, 10, RED);
}
//========================================================================
void eraseDot(int x, int y) {
  M5.Lcd.fillRect(12*(x-1)+5, 12*(y-1)+23, 10, 10, BLUE);//Original is BLUE
}
//========================================================================
void printScore() {
  M5.Lcd.fillRect(88, 3, 50, 16, WHITE);//clears old score
  M5.Lcd.setCursor(88, 3);
  M5.Lcd.print(score);  //prints current score
}
//========================================================================
void printSpeed() {
  M5.Lcd.fillRect(248, 3, 50, 16, WHITE);//clears old score
  M5.Lcd.setCursor(248, 3);
  M5.Lcd.print((int)(gameSpeed+plusSpeed));  //prints current score
}
//========================================================================
void doUp() {
  //lastMoveH makes sure you can't go back on yourself
  if (millis() - offsetT > gs and lastMoveH) {
    changeX = 0; changeY = -1;  //changes the direction of the snake
    offsetT = millis();
    lastMoveH = false;
  }
}
//========================================================================
void doDown() {
  if (millis() - offsetT > gs and lastMoveH) {
    changeX = 0; changeY = 1;
    offsetT = millis();
    lastMoveH = false;
  }
}
//========================================================================
void doLeft() {
  if (millis() - offsetT > gs and !lastMoveH) {
    changeX = -1; changeY = 0;
    offsetT = millis();
    lastMoveH = true;
  }
}
//========================================================================
void doRight() {
  if (millis() - offsetT > gs and !lastMoveH) {
    changeX = 1; changeY = 0;
    offsetT = millis();
    lastMoveH = true;
  }
}
//========================================================================
void doSelect() {
  if (millis() - offsetT > gs and !doStart) {
    M5.Lcd.fillRect(10, 50, 300, 200, BLUE); //Erase start message
    //M5.Lcd.fillRect(80, 90, 126, 24, BLUE); //Erase start message
    doStart = true;                         //allows loop to start
    offsetT = millis();
  }
}
//========================================================================
