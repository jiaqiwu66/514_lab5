#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

const char* ssid = "UW MPSK";
const char* password = "4kwYpX)n#r"; // Replace with your network password
#define DATABASE_URL "https://jwu-514-lab5-default-rtdb.firebaseio.com" // Replace with your database URL
#define API_KEY "AIzaSyBzxQACR5RsEemz_8Xy8KYP4emIRiLfJRM" // Replace with your API key
#define STAGE_INTERVAL 12000 // 12 seconds each stage
#define MAX_WIFI_RETRIES 5 // Maximum number of WiFi connection retries

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// HC-SR04 Pins
const int trigPin = 2;
const int echoPin = 1;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Function prototypes
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance, int uploadInterval);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  unsigned long startTime = millis();
  unsigned long currentTime = millis();
  long TOTAL_TIME = 86400000; // 24 hour to milisecond
  long outRangeTimestamp = 0L;
  bool outRangeFlag = false;
  float prevDistance = measureDistance();
  int CONTINUE_OUT_RANGE_TIME = 10000;// distance > 50cm for more than 10s can trigger sleep mode
  int SLEEP_TIME = 30000;//sleep mode last for 30s
  float OUT_RANGE_DIS = 50.0f;//distance > 50cm is out of range and cannot be measured
  float VALID_DELTA = 5.0f;//move distance > 5cm can trigger for sending data
  Serial.println("Connect to the WiFi ... ");
  connectToWiFi();
  delay(1000);
  while (currentTime - startTime < TOTAL_TIME) {
    float currentDistance = measureDistance();
    
    if (outRangeFlag && currentTime - outRangeTimestamp >= CONTINUE_OUT_RANGE_TIME) {
      // sleep 
      Serial.println("Start to sleep");
      WiFi.disconnect();
      esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000); // in microseconds
      esp_deep_sleep_start();
      outRangeFlag = false;
      outRangeTimestamp = millis();
      connectToWiFi();
      delay(1000);
    }

    if (currentDistance > OUT_RANGE_DIS) {
      if (!outRangeFlag) {
        // first time out of range
        // record the out of range start time stamp
        outRangeTimestamp = currentTime;
        outRangeFlag = true;
      }
    } else {
      outRangeFlag = false;
      outRangeTimestamp = currentTime;

      float delta = abs(currentDistance - prevDistance);
      if (delta > VALID_DELTA) {
        Serial.println("Upload the data to Firebase ... ");
        initFirebase();
        sendDataToFirebase(currentDistance, 1000);
      }
    }
    prevDistance = currentDistance;
    currentTime = millis();
    delay(1000);
  }
}

void loop(){
  // This is not used
}

float measureDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi()
{
  // Print the device's MAC address.
  Serial.println(WiFi.macAddress());
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wifiCnt++;
    if (wifiCnt > MAX_WIFI_RETRIES){
      Serial.println("WiFi connection failed");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase()
{
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

void sendDataToFirebase(float distance, int uploadInterval){
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > uploadInterval || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.pushFloat(&fbdo, "test/distance", distance)){
      Serial.println("PASSED");
      Serial.print("PATH: ");
      Serial.println(fbdo.dataPath());
      Serial.print("TYPE: " );
      Serial.println(fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.print("REASON: ");
      Serial.println(fbdo.errorReason());
    }
    count++;
  }
}