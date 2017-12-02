#include <WioLTEforArduino.h>
#include <ADXL345.h>
#include <WioLTEClient.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <config.h>

#define ANALOG_A4 (WIOLTE_A4)

#define APN               "soracom.io"
#define USERNAME          "sora"
#define PASSWORD          "sora"

#define MQTT_SERVER_HOST  "beam.soracom.io"
#define MQTT_SERVER_PORT  (1883)

#define ID                "WioLTE_"
#define OUT_TOPIC         "E64CE564379A0CC8B000CA84CFDD61F2/json"
#define IN_TOPIC          "E64CE564379A0CC8B000CA84CFDD61F2/440103072674825/subscribe"

#define INTERVAL          (1000)
//#define SHOCK_INTERVAL    (100)
ADXL345 adxl;

WioLTE Wio;
WioLTEClient WioClient(&Wio);
PubSubClient MqttClient;

String cmd_string;
long randnum;
String client;
char client_id[32];

void callback(char* topic, byte* payload, unsigned int length) {
  cmd_string = "";
  SerialUSB.print("Subscribe:");
  for (int i = 0; i < length; i++) {
    cmd_string = String(cmd_string + (char)payload[i]);
    //SerialUSB.print((char)payload[i]);
  }
  SerialUSB.println("");
  SerialUSB.print("cmd_string:");
  SerialUSB.print(cmd_string);
  if (cmd_string == "mode:1") {
    // Set activity to insensitivity
    SerialUSB.println("");
    SerialUSB.println("set activity_thresh:50");
    adxl.setActivityThreshold(50); //default:75
    char data[100];
    sprintf(data, "{\"mode\":%lu}", 1);
    SerialUSB.print("Publish:");
    SerialUSB.print(data);
    SerialUSB.println("");
    MqttClient.publish(OUT_TOPIC, data);
  }
  if (cmd_string == "mode:0") {
    // Set activity to default
    SerialUSB.println("");
    SerialUSB.println("set activity_thresh:30");
    adxl.setActivityThreshold(30); //default:75
    char data[100];
    sprintf(data, "{\"mode\":%lu}", 0);
    SerialUSB.print("Publish:");
    SerialUSB.print(data);
    SerialUSB.println("");
    MqttClient.publish(OUT_TOPIC, data);
  }

}

void setup() {
  pinMode(ANALOG_A4, INPUT_ANALOG);
  randomSeed(analogRead(ANALOG_A4));
  delay(200);

  SerialUSB.println("");
  SerialUSB.println("--- START ---------------------------------------------------");

  SerialUSB.println("### I/O Initialize.");
  Wio.Init();
  adxl.powerOn(); // ADXL-345 Init
  //set activity/ inactivity thresholds (0-255)
  //adxl.setActivityThreshold(75); //62.5mg per increment
  adxl.setActivityThreshold(30); // custom value
  adxl.setInactivityThreshold(75); //62.5mg per increment
  //adxl.setTimeInactivity(10); // how many seconds of no activity is inactive?
  adxl.setTimeInactivity(255); // how many seconds of no activity is inactive?

  //look of activity movement on this axes - 1 == on; 0 == off
  adxl.setActivityX(1);
  adxl.setActivityY(1);
  adxl.setActivityZ(1);

  //look of inactivity movement on this axes - 1 == on; 0 == off
  adxl.setInactivityX(1);
  adxl.setInactivityY(1);
  adxl.setInactivityZ(1);

  //setting all interrupts to take place on int pin 1
  //I had issues with int pin 2, was unable to reset it
  adxl.setInterruptMapping( ADXL345_INT_ACTIVITY_BIT,     ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_INACTIVITY_BIT,   ADXL345_INT1_PIN );

  //register interrupt actions - 1 == on; 0 == off
  adxl.setInterrupt( ADXL345_INT_ACTIVITY_BIT,   1);
  adxl.setInterrupt( ADXL345_INT_INACTIVITY_BIT, 1);


  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyLTE(true);
  delay(5000);

  SerialUSB.println("### Turn on or reset.");

  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("### Connecting to \""APN"\".");
  delay(5000);
  if (!Wio.Activate(APN, USERNAME, PASSWORD)) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  conn_mqtt();
}

void conn_mqtt() {
  // Connecting MQTT broker
  SerialUSB.println("### Connecting to MQTT server \""MQTT_SERVER_HOST"\"");
  MqttClient.setServer(MQTT_SERVER_HOST, MQTT_SERVER_PORT);
  MqttClient.setCallback(callback);
  MqttClient.setClient(WioClient);
  randnum = random(100000);
  client = String(ID + (String)randnum);
  SerialUSB.print("client ID:");
  SerialUSB.print(client);
  SerialUSB.println("");
  client.toCharArray(client_id, 32);
  if (!MqttClient.connect(client_id)) {
    SerialUSB.println("### ERROR! ###");
    return;
  }
  MqttClient.subscribe(IN_TOPIC);
}

void loop() {
  int x;
  int y;
  int z;
  adxl.readXYZ(&x, &y, &z);
  SerialUSB.print(x);
  SerialUSB.print(' ');
  SerialUSB.print(y);
  SerialUSB.print(' ');
  SerialUSB.println(z);

  //getInterruptSource clears all triggered actions after returning value
  //so do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();

  //activity
  if (adxl.triggered(interrupts, ADXL345_ACTIVITY)) {
    SerialUSB.println("Motion detected.");
    //add code here to do when activity is sensed
    //conn_mqtt();
    char data[100];
    sprintf(data, "{\"motion\":%lu}", 1);
    SerialUSB.print("Publish:");
    SerialUSB.print(data);
    SerialUSB.println("");
    MqttClient.publish(OUT_TOPIC, data);
  }


  //inactivity
  if (adxl.triggered(interrupts, ADXL345_INACTIVITY)) {
    SerialUSB.println("No motion detected.");
    //add code here to do when inactivity is sensed
    //conn_mqtt();
    char data[100];
    sprintf(data, "{\"motion\":%lu}", 0);
    SerialUSB.print("Publish:");
    SerialUSB.print(data);
    SerialUSB.println("");
    MqttClient.publish(OUT_TOPIC, data);
  }

  //delay(INTERVAL);

err:
  unsigned long next = millis();
  while (millis() < next + INTERVAL)
  {
    MqttClient.loop();
  }

}
