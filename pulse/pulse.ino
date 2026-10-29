#include <Keyboard.h>


// =====================================================
// CONFIGURATION
// =====================================================

const bool DEBUG = true;


// MT8870
const byte PIN_Q1  = 2;
const byte PIN_Q2  = 3;
const byte PIN_Q3  = 4;
const byte PIN_Q4  = 5;
const byte PIN_STD = 6;


// LED
const byte LED_PIN = LED_BUILTIN;


// key hold time
const unsigned long KEY_TIME = 80;


// STD filtering
const unsigned long STD_FILTER = 20;



// =====================================================
// PROFILES
// =====================================================

const char* profileName[4] =
{
  "P1",
  "P2",
  "P3",
  "P4"
};


byte activeProfile = 0;



// =====================================================
// KEY ACTION
// =====================================================

struct KeyAction
{
  uint8_t key;
  bool special;
};



// =====================================================
// PROFILE P1 MAPPING
// DuckStation: D-Pad + Face buttons + L1/L2/R1/R2
// =====================================================

KeyAction profileP1[16] =
{
  {KEY_DOWN_ARROW,true},  //0 - D-Pad Down
  {'1',false},            //1 - L2
  {'a',false},            //2 - Triangle
  {'3',false},            //3 - R2
  {'q',false},            //4 - L1
  {'z',false},            //5 - Circle
  {'e',false},            //6 - R1
  {KEY_LEFT_ARROW,true},  //7 - D-Pad Left
  {KEY_UP_ARROW,true},    //8 - D-Pad Up
  {KEY_RIGHT_ARROW,true}, //9 - D-Pad Right
  {'x',false},            //* - Cross
  {'s',false},            //# - Square
  {0,false},   //A
  {0,false},   //B
  {0,false},   //C
  {0,false}    //D
};



// =====================================================
// PROFILE P2 MAPPING
// =====================================================

KeyAction profileP2[16] =
{
  {KEY_DOWN_ARROW,true},  //0 - D-Pad Down
  {KEY_LEFT_ARROW,true},  //1 - D-Pad Left
  {KEY_RIGHT_ARROW,true}, //2 - D-Pad Right
  {'3',false},            //3 - R2
  {'q',false},            //4 - L1
  {'z',false},            //5 - Circle
  {'e',false},            //6 - R1
  {'a',false},            //7 - Triangle
  {KEY_UP_ARROW,true},    //8 - D-Pad Up
  {'x',false},            //9 - Cross
  {'1',false},            //* - L2
  {'s',false},            //# - Square
  {0,false},
  {0,false},
  {0,false},
  {0,false}
};



// =====================================================
// PROFILE P3 MAPPING
// =====================================================

KeyAction profileP3[16] =
{
  {'0',false},
  {'1',false},
  {'2',false},
  {'3',false},
  {'4',false},
  {'5',false},
  {'6',false},
  {'7',false},
  {'8',false},
  {'9',false},
  {'*',false},
  {'#',false},
  {0,false},
  {0,false},
  {0,false},
  {0,false}
};



// =====================================================
// PROFILE P4 MAPPING
// =====================================================

KeyAction profileP4[16] =
{
  {'0',false},
  {'1',false},
  {'2',false},
  {'3',false},
  {'4',false},
  {'5',false},
  {'6',false},
  {'7',false},
  {'8',false},
  {'9',false},
  {'*',false},
  {'#',false},
  {0,false},
  {0,false},
  {0,false},
  {0,false}
};



KeyAction* profiles[4] =
{
  profileP1,
  profileP2,
  profileP3,
  profileP4
};



// =====================================================
// MT8870 TABLE
// Q4 Q3 Q2 Q1
// =====================================================

const byte mt8870Map[16] =
{
  15,
   1,
   2,
   3,
   4,
   5,
   6,
   7,
   8,
   9,
   0,
  10,
  11,
  12,
  13,
  14
};

// =====================================================
// VARIABLES
// =====================================================

bool keyPressed = false;

uint8_t currentKey = 0;
bool currentSpecial = false;

unsigned long keyStart = 0;


// STD detection
bool lastSTD = false;
unsigned long lastSTDChange = 0;



// =====================================================
// LED STATE MACHINE
// =====================================================

bool ledRunning = false;
bool ledState = false;

byte ledTarget = 0;
byte ledCounter = 0;

unsigned long ledTimer = 0;

const unsigned long LED_INTERVAL = 120;



void startProfileLED(byte profile)
{
  ledTarget = profile + 1;
  ledCounter = 0;

  ledState = false;
  ledRunning = true;

  ledTimer = millis();

  digitalWrite(LED_PIN, LOW);
}



void updateLED()
{
  if(!ledRunning)
    return;


  unsigned long now = millis();


  if(now - ledTimer >= LED_INTERVAL)
  {
    ledTimer = now;


    ledState = !ledState;

    digitalWrite(LED_PIN, ledState);


    if(!ledState)
    {
      ledCounter++;


      if(ledCounter >= ledTarget)
      {
        ledRunning = false;
        digitalWrite(LED_PIN, LOW);
      }
    }
  }
}



// =====================================================
// RELEASE CURRENT KEY (safe, always)
// =====================================================

void releaseCurrentKey()
{
  if(!keyPressed)
    return;

  if(currentSpecial)
    Keyboard.release(currentKey);
  else
    Keyboard.release((char)currentKey);

  keyPressed = false;
}



// =====================================================
// SETUP
// =====================================================

void setup()
{
  pinMode(PIN_Q1, INPUT);
  pinMode(PIN_Q2, INPUT);
  pinMode(PIN_Q3, INPUT);
  pinMode(PIN_Q4, INPUT);
  pinMode(PIN_STD, INPUT);

  pinMode(LED_PIN, OUTPUT);


  if(DEBUG)
  {
    Serial.begin(115200);

    unsigned long t = millis();
    while(!Serial && (millis() - t < 3000))
    {
    }

    Serial.println();
    Serial.println("MT8870 DTMF Keyboard");
    Serial.println("Active profile: P1");
  }


  Keyboard.begin();


  startProfileLED(0);
}




// =====================================================
// READ DTMF
// =====================================================

byte readDTMF()
{

  byte raw =
    (digitalRead(PIN_Q4) << 3) |
    (digitalRead(PIN_Q3) << 2) |
    (digitalRead(PIN_Q2) << 1) |
     digitalRead(PIN_Q1);


  return mt8870Map[raw];
}




// =====================================================
// SWITCH PROFILE
// =====================================================

void changeProfile(byte profile)
{

  if(profile > 3)
    return;

  // release key if held during switch
  releaseCurrentKey();

  activeProfile = profile;


  startProfileLED(profile);


  if(DEBUG)
  {
    Serial.print("Profile: ");
    Serial.println(profileName[profile]);
  }
}




// =====================================================
// SEND KEY
// =====================================================

void sendKey(KeyAction action)
{

  if(action.key == 0)
    return;

  // release previous key if still held
  releaseCurrentKey();


  currentKey = action.key;
  currentSpecial = action.special;


  if(action.special)
    Keyboard.press(action.key);

  else
    Keyboard.press((char)action.key);


  keyPressed = true;

  keyStart = millis();



  if(DEBUG)
  {
    Serial.print("Key: ");

    if(action.special)
      Serial.println("SPECIAL");

    else
      Serial.println((char)action.key);
  }

}



// =====================================================
// LOOP
// =====================================================

void loop()
{

  unsigned long now = millis();



  // ---------------------------------------------
  // LED
  // ---------------------------------------------

  updateLED();



  // ---------------------------------------------
  // KEY RELEASE
  // ---------------------------------------------

  if(keyPressed)
  {

    if(now - keyStart >= KEY_TIME)
    {
      releaseCurrentKey();
    }
  }




  // ---------------------------------------------
  // STD - new DTMF tone
  // ---------------------------------------------

  bool std = digitalRead(PIN_STD);



  if(std && !lastSTD)
  {

    if(now - lastSTDChange >= STD_FILTER)
    {

      byte dtmf = readDTMF();


      lastSTDChange = now;



      // A = P1
      if(dtmf == 12)
      {
        changeProfile(0);
      }


      // B = P2
      else if(dtmf == 13)
      {
        changeProfile(1);
      }


      // C = P3
      else if(dtmf == 14)
      {
        changeProfile(2);
      }


      // D = P4
      else if(dtmf == 15)
      {
        changeProfile(3);
      }


      // digits, *, #
      else
      {

        KeyAction action =
          profiles[activeProfile][dtmf];


        sendKey(action);

      }



      if(DEBUG)
      {
        Serial.print("DTMF=");
        Serial.println(dtmf);
      }

    }
  }



  lastSTD = std;

}
