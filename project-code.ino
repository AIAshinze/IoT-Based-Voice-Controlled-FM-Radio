
#include <ESP8266WiFi.h> //Library for the ESP8266 NodeMCU Board
#include <Wire.h> //Wire Libray needed for I2C communication
#include <PT2258.h> //PT2258 Library needed to communicate with the IC
#include <RDA5807M.h> //RDA5807M Library needed to communicate with the RDA module
#include <Adafruit_MQTT.h> // Adafruit MQTT needed fot MQTT
#include <Adafruit_MQTT_Client.h> //Adafruit MQTT is for the MQTT Client
const char* ssid = "CASTLE BLACK"; // SSID of router
const char* password = "Blackpegasus"; // Password of router
bool potStatus; // 1 when communication is established between the MCU and the IC
bool radioStatus; // 1 when communication is established between the MCU and the IC
int volume = 15; // default volume level at start off
#define Relay_Pin 13 // This pin is used to turn on and off the radio
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "littlegreenman" //AIO UserName
#define AIO_KEY "aio_rWry521iHxbtKrhVlGc1JQZ8L4z5"
//..............TMP_RADIO...............
#define FIX_BAND RADIO_BAND_FM ///<The band that will be tuned by this sketch is FM.
#define FIX_RADIO_VOLUME 6  ///<The Volume 
//...........TMP_RADIO..........
PT2258 digitalPot; // PT2258 object
RDA5807M radio; //RDA5807 Object
WiFiClient client; //WiFiClient Object
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_KEY); // Setup the MQTT client class by passing in the WiFi client and MQTT sever and login details.
Adafruit_MQTT_Subscribe Radio_Station = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME"/feeds/Radio_Station");//Methods used to subscribe to a Feed
Adafruit_MQTT_Subscribe Toggle_FM = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME"/feeds/Toggle_FM"); //Methods used to subscribe to feed
Adafruit_MQTT_Subscribe Volume = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME"/feeds/Volume"); //Methods used to subscribe to feed)
void MQTT_connect(); //Function Prototype for MQTT Connect

void setup() {
  Serial.begin(9600); //UART begin
  Serial.println(); // adds an extra line for spacing
  Serial.println(); // adds an extra line for spacing
  /******************* all the usual things required for the WiFi connection ********/
  Serial.print("connecting to");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
delay(500);
  Serial.print(".");
  }
  Serial.println(" ");
  Serial.println("WiFi connected");
  Serial.println("IP address");
  Serial.println(WiFi.localIP());
  /************* all the usual things required for WiFi connection *********/
  Wire.begin(); // begin the I2C starting sequence
  Wire.setClock(100000); // setting the I2C clock to 100KHz
  Serial.println("initiating Radio with Digital Pot...");
  delay(200);
  potStatus = digitalPot.init(); //boolean used for status checks
  radioStatus = radio.init(); //boolean used for status checks
  if (potStatus)
  {
 Serial.println("Found PT2258 Device");
  }
  else
  {
  Serial.println("Failed to initiate PT2258");
  }
  if (radioStatus)
  {
  Serial.println("Found RDA5807M Device");
  }
 else 
  {
  Serial.println("Failed to initiate RDA5807M");
  }
   mqtt.subscribe(&Radio_Station);//Setup MQTT subscription for Radio_Station feed
   mqtt.subscribe(&Toggle_FM);//Setup MQTT subscription for Toggle_FM
   mqtt.subscribe(&Volume);//Setup MQTT subscription for Volume feed
   pinMode(Relay_Pin, OUTPUT);
   digitalWrite(Relay_Pin, LOW);
   radio.setVolume(FIX_RADIO_VOLUME);//next we set the normalize radio volume
   radio.setMono(false);// we do not want the chip to give mono output 
   radio.setMute(false);// we do not want the chip to go mute at start 
}

void loop() {
 MQTT_connect();// call this function to connect to the MQTT Server
 Adafruit_MQTT_Subscribe *subscription;// we make a pointer to Adafruit_MQTT_Subscribe object.
 while ((subscription = mqtt.readSubscription(20000))) // We check to see if there is any new message from the server or not 
 {
  if (subscription == &Toggle_FM)
  {
  // is it a message from the Toggle_FM Feed
  Serial.print(F("GOt:"));
  Serial.println((char *)Toggle_FM.lastread); // print the feed data just for debugging
  if (String((char *)Toggle_FM.lastread) == String("on")) 
  // we compare the received data to known parameter in this case we are expecting that "on" is coming from the server
  {  
  // but before we do that we have to make it a string which makes the comparison super easy
  digitalWrite(Relay_Pin, HIGH); // if we get a "on" string from the server we are making the D7 pin HIGH 
  }
  if (String((char *)Toggle_FM.lastread) == String("off")) // again we are checking for the string off 
  {
  digitalWrite(Relay_Pin, LOW); // if we get an "off" string from the server we are making the D7 pin LOW
  }
 }
 if (subscription == &Radio_Station)
 {
  Serial.print(F("Got: "));
  Serial.println((char *)Radio_Station.lastread);
  if (String((char *)Radio_Station.lastread) == String("DBS Asaba")) // here we are checking for the string DBS Asaba FM
  {
  radio.setBandFrequency(FIX_BAND, 9790); // if the above condition is true we are setting the radio channel to 97.9MHz
  }
  if (String((char *)Radio_Station.lastread) == String("Hit FM")) // here we are checking for the string Hit FM 
  {
  radio.setBandFrequency(FIX_BAND, 9590); // if the above condition is true we are setting the radio channel to 95.9MHz 
  }
  // The above mentioned process is continued 
  if (String((char *)Radio_Station.lastread) == String("Sparkling FM"))
  {
  radio.setBandFrequency(FIX_BAND, 9230); // if above condition is true we are setting the radio channel to 92.3MHz
  }                                      
  if (String((char *)Radio_Station.lastread) == String("Correct FM"))
  {
  radio.setBandFrequency(FIX_BAND, 9730); // if the above condition is true we are setting the radio channel to 97.3MHz
  }
 }
 if (subscription == &Volume){
   // here we are checking for the string Volume and it is an integer value in a string format 
 // we must convert it back to an integer to change the volume with the help of the PT2258 IC 
 Serial.print(F("Got: "));
 Serial.println((char *)Volume.lastread);
 volume = atoi((char *)Volume.lastread); // We are using the atoi() method to convert a character pointer to an integer 
 volume = map(volume, 0, 100, 79, 0); //map(value,fromLow,toHigh, toLow, toHigh) // as the PT2258 IC only understands integer values in dB 
 // we are mapping the 0dB-79dB value to 0% - 100%
 digitalPot.setChannelVolume(volume, 0); //after all that we are setting the volume for the channel 0 of the PT2258 IC
 digitalPot.setChannelVolume(volume, 1); //after all that we are setting the volume for the channel 1 of the PT2258 IC
}
}
}
void MQTT_connect()
{
  int8_t ret; // 8 bit integer to store the retries 
  // Stop if already connected. 
  if (mqtt.connected()) {
  return;
  }
  Serial.print("Connecting to MQtt...");
  uint8_t retries = 3; 
  while ((ret = mqtt.connect()) !=0 )
  { //connect will return 0 for connected 
  Serial.println(mqtt.connectErrorString(ret));
  Serial.println("Retrying MQTT connection in 5 seconds...");
  mqtt.disconnect();
  delay(5000);
  //wait 5 seconds
  retries--;
  if (retries == 0) {
 // basically die and wait for WDT to reset me 
 while (1); 
  }
  }
  Serial.println("MQTT Connected!");
}
