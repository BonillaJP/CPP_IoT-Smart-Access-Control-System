#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <WiFi.h>
#include <WebServer.h>
#include <ESP2SOTA.h>
#include <BlynkSimpleEsp32.h>
#include <Keypad.h>

const char* ssid = "";          
const char* password = ""; 

WebServer server(80);

unsigned long previousMillis = 0;
const long interval = 30000;

const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = 
{
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {12,14,27,26};
byte colPins[COLS] = {25,33,32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String inputPassword = "";

String passwords[5] = {"12345","23456","34567","45678","56789"};

int blueLEDs[5] = {4,16,17,5,18};  
int redLED = 19;                     
int greenLED = 21;                   
int bypassButton = 22;
int relayPin = 23;                    

unsigned long unlockDuration = 3000; 


void checkPassword();
void blinkFeedback(bool success);
void unlockDoor();
void updateBlueLEDs();
void connectWiFi();

void setup() 
{
  Serial.begin(115200);

  keypad.setDebounceTime(50); 

  pinMode(redLED, OUTPUT);
  digitalWrite(redLED,HIGH);  
  pinMode(greenLED, OUTPUT);
  digitalWrite(greenLED,LOW); 
  for(int i=0;i<5;i++)
  {
    pinMode(blueLEDs[i],OUTPUT);
  }

  pinMode(bypassButton, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT); 
  digitalWrite(relayPin, HIGH);

  Serial.println("Starting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Blynk.config(BLYNK_AUTH_TOKEN);

  ESP2SOTA.begin(&server);
  server.begin();

  Serial.println("Setup complete. System is active regardless of WiFi.");
}

void loop() {
  unsigned long currentMillis = millis();

  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
    server.handleClient();
  }
  else if (currentMillis - previousMillis >= interval) {
    Serial.println("WiFi lost or not connected. Retrying...");
    WiFi.disconnect(); 
    WiFi.reconnect();
    previousMillis = currentMillis;
  }

  char key = keypad.getKey();
  if(key)
  {
    if(key >= '0' && key <= '9')
    {
      if(inputPassword.length() < 5)
      {
        inputPassword += key;
        updateBlueLEDs();
      }
      if(inputPassword.length() == 5) 
      {
        checkPassword();
      }
    }
    else if(key == '*')
    {
      if(inputPassword.length() > 0)
      {
        inputPassword.remove(inputPassword.length()-1);
        updateBlueLEDs();
      }
    }
    else if(key == '#')
    {
      inputPassword = "";
      updateBlueLEDs();
    }
  }

  if(digitalRead(bypassButton) == LOW)
  {
    unlockDoor();
    if (Blynk.connected()) {
        Blynk.logEvent("bypass","Door unlocked via bypass button");
    }
    delay(500); 
  }
}

void updateBlueLEDs()
{
  for(int i=0;i<5;i++)
  {
    digitalWrite(blueLEDs[i], i < inputPassword.length() ? HIGH : LOW);
  }
}

void checkPassword(){
  bool correct = false;
  int correctIndex = -1;

  for(int i=0;i<5;i++)
  {
    if(inputPassword == passwords[i])
    {
      correct = true;
      correctIndex = i+1;
      break;
    }
  }

  if(correct)
  {
    blinkFeedback(true);
    if (Blynk.connected()) {
       Blynk.logEvent("correct_pin", "Password " + String(correctIndex) + " used");
    }
    unlockDoor();
  }
  else
  {
    blinkFeedback(false);
    if (Blynk.connected()) {
       Blynk.logEvent("wrong_pin", "Incorrect PIN entered: " + inputPassword);
    }
  }

  inputPassword = "";
  updateBlueLEDs();
}

void blinkFeedback(bool success){
  int times = success ? 1 : 2;
  for(int t=0;t<times;t++)
  {
    for(int i=0;i<5;i++)
    {
      digitalWrite(blueLEDs[i],HIGH);
    }
    delay(200);
    for(int i=0;i<5;i++)
    {
      digitalWrite(blueLEDs[i],LOW);
    }
    delay(200);
  }
}

void unlockDoor(){
  digitalWrite(relayPin,LOW);
  digitalWrite(redLED,LOW);
  digitalWrite(greenLED,HIGH);
  delay(unlockDuration);
  digitalWrite(relayPin,HIGH);
  digitalWrite(redLED,HIGH);
  digitalWrite(greenLED,LOW);
}