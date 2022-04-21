/********************************************************* 
  Mark Bitz 
  Plant Soil Moisture Sensor Monitor
  Date:03/21/2022
  Modified:04/21/2022

  Description
    Monitors moisture for a plant and sends data up to the MQTT site and sends a text alert to a cell phone number

  HARDWARE:  DOIT ESP32 DEVKIT V1 - Or any other ESP32 will work as well.
  ESP32
  Capacitive Soil Moisture Sensor V2.0
  18650 5800mAh 3.7v regardable battery
  connector wire
/********************************************************/
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ESP_Mail_Client.h>
const char *ssid =  "YOUR SSID HERE";     // Enter your WiFi Name
const char *pass =  "YOUR WIFI PASSWORD HERE"; // Enter your WiFi Password
String hostname = "PLANT NAME HERE";
WiFiClient client;
#define LOCAL_MQTT_SERV "LOCAL MQTT SERVER"
#define MQTT_SERV "io.adafruit.com"
#define LOCAL_MQTT_PORT 1883
#define MQTT_PORT 1883
#define MQTT_NAME "YOUR USERNAME HERE" // Your Adafruit IO Username
#define MQTT_PASS "aio_pqzc26hujQiF9aI1KeLN0oEuAaQX" // Adafruit IO AIO key
#define DEEP_SLEEP_TIME 3600
const int moisturePin = A0;             // moisteure sensor pin
int moisturePercentage;              //moisture reading
int moistValue;

/* SMTP Information */
#define SMTP_HOST "smtp-mail.outlook.com"
#define SMTP_PORT 587
/* The email sign in credentials */
#define AUTHOR_EMAIL "YOUR EMAIL HERE"
#define AUTHOR_PASSWORD "EMAIL PASSWORD HERE"

//The SMTP Session object used for Email sending 
SMTPSession smtp;

// Callback function to get the Email sending status 
void smtpCallback(SMTP_Status status);
//sendmail text function to send email
void sendmail_text(char* namer, char* recipient, int moistValue);


//Set up the feed you're publishing to
//Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);
//Adafruit_MQTT_Publish SoilMoisture1 = Adafruit_MQTT_Publish(&mqtt,MQTT_NAME "/f/SoilMoisture1");  // SoilMoisture1 is the feed name where you will publish your data
//for a local MQTT SERVER
Adafruit_MQTT_Client mqtt(&client, LOCAL_MQTT_SERV, LOCAL_MQTT_PORT);
Adafruit_MQTT_Publish SoilMoisture1 = Adafruit_MQTT_Publish(&mqtt,"YOUR PLAN NAME HERE/moistValue");  // moistValue is the feed name where you will publish your data

void goToDeepSleep(){
  Serial.println("Going to Sleep...");
  // 43200000000 = 12 hours in microseconds
  esp_sleep_enable_timer_wakeup(43200000000);
  esp_deep_sleep_start();
}

void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.setHostname(hostname.c_str()); //define hostname
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");              // print ... till not connected
  }
  Serial.println("");
  Serial.println("WiFi connected");

  MQTT_connect();
  /*moisturePercentage = ( 100 - ( (analogRead(moisturePin) / 1023.00) * 100 ) ); */
  int samplesize = 5;
  for (int i=0; i<samplesize; i++){
    moistValue = analogRead(moisturePin);
    moistValue ++;
    delay(100);
  }
  moistValue = moistValue/samplesize;

  Serial.print("Soil Moisture is  = ");
  Serial.println(moistValue);
       if (! SoilMoisture1.publish(moistValue)) //This condition is used to publish the Variable (moisturePercentage) on adafruit IO. Change thevariable according to yours.
       {                     
         delay(5000);   
          }
  //Enable the debug via Serial port * none debug or 0 * basic debug or 1 
  //smtp.debug(1);
  
  //Set the callback function to get the sending results   
  smtp.callback(smtpCallback);
  sendmail_text("April","3125551212@tmomail.net",moistValue);
  sendmail_text("John","3157013261@vtext.com",moistValue);
  sendmail_text("Sue", "SueJohnson@gmail.com",moistValue);

  delay(1000);
  goToDeepSleep();
}


void sendmail_text(char* namer, char* recipient, int moistValue) {
    if (1) { Serial.println("in sendmail_text"); }

    //Declare the session config data 
    ESP_Mail_Session session;

    //Set the session config
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";
      
    //Declare the message class 
    SMTP_Message message;

    //Set the message headers
    message.sender.name = "NAME HERE";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "PLANT NAME Moisture ALERT";
    message.addRecipient(namer, recipient);
  
    //Send HTML message
    String htmlMsg = "<div style=\"color:#2f4468;\"><h1>ALERT-PLANT NAME needs Water</h1><p>Value:";
    htmlMsg += moistValue;
    htmlMsg += "</p></div>";
    message.html.content = htmlMsg.c_str();
    message.html.content = htmlMsg.c_str();
    message.text.charSet = "us-ascii";
    message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
     
    //Connect to server with the session config
    if (!smtp.connect(&session)) {
      return;
    }
     
    //Start sending Email and close the session
    if (!MailClient.sendMail(&smtp, &message)) {
      Serial.println("Error sending Email, " + smtp.errorReason());
    }
    htmlMsg = "";
    delay(5000); //delay 5 seconds
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}


void loop(){

}

void MQTT_connect() 
{
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) 
  {
    return;
  }
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) // connect will return 0 for connected
  { 
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) 
       {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
}
