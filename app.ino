#include "HX711.h"
#include <ESP32Servo.h>
#include <WiFi.h>

const char* ssid = "DIGI_ee374a";
const char* password = "6cb5a47d";

#define DT 2   // DOUT (data)
#define SCK 3  // clock
#define SERVO_PIN 5  // pinul conectat la semnalul servo

HX711 scale;
Servo servoMotor;
WiFiServer server(80);


float calibration_factor = -7050.0; 


IPAddress local_IP(192, 168, 1, 50);      
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);



void setup() {


  Serial.begin(115200);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  Serial.print("Conectare...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectat!");
  Serial.println(WiFi.localIP());

  server.begin();
  


  servoMotor.attach(SERVO_PIN);
  Serial.begin(9600);
  delay(500);

  Serial.println("Initializare...");

  scale.begin(DT, SCK);
  scale.set_scale();  
  scale.tare();       

  Serial.println("Tare complet!");
  Serial.println("Pune un obiect de greutate cunoscută (ex. 100g)");
  delay(5000); 
  
}



void fill() {
  long valoare_bruta = scale.read_average(10);
  Serial.print("Valoare brută: ");
  Serial.println(valoare_bruta);

  scale.set_scale(calibration_factor);

  float greutate = scale.get_units(5); // Media pe 5 citiri

  float greutate_noua = greutate + 5;

  unsigned long startTime = millis();
  const unsigned long timeout = 7000; // 7 secunde

  servoMotor.write(0); // deschis

  while (greutate < greutate_noua) {
    // Timeout?
    if (millis() - startTime > timeout) {
      Serial.println("[EROARE] Sistem blocat sau fără hrană!");
      break;
    }

    valoare_bruta = scale.read_average(10);
    scale.set_scale(calibration_factor);
    greutate = scale.get_units(5);
  }

  servoMotor.write(90); // inchis
}



bool mod_automat_activ = false;

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client conectat");

    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n' && request.endsWith("\r\n\r\n")) {
          break;
        }
      }
    }

    Serial.println("Cerere:");
    Serial.println(request);

    // Comenzi simple
    if (request.indexOf("GET /automat") >= 0) {
      mod_automat_activ = true;
      client.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nMod automat activat");
    } else if (request.indexOf("GET /stop") >= 0) {
      mod_automat_activ = false;
      client.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nMod automat oprit");
    } else if (request.indexOf("GET /fill") >= 0) {
      fill();
      client.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nFilling");
    } else if (request.indexOf("GET /read") >= 0) {
      long valoare_bruta = scale.read_average(10);

      scale.set_scale(calibration_factor);

      float g = scale.get_units(5); 
      
      client.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nGreutate: " + String(g) + " g");
    } else if (request.indexOf("GET /open") >= 0) {
      servoMotor.write(0);
      client.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOPENED");
    } else if (request.indexOf("GET /close") >= 0) {
      servoMotor.write(90);
      client.println("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nCLOSED");
    } else {
      client.println("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nComanda necunoscuta");
    }

    delay(10);
    client.stop();
    Serial.println("Client deconectat");
  }

  // mod automat
  if (mod_automat_activ) {
    Serial.println("Execut mod automat...");

    long valoare_bruta = scale.read_average(10);
    Serial.print("Valoare brută: ");
    Serial.println(valoare_bruta);

    scale.set_scale(calibration_factor);

    float greutate = scale.get_units(5); // Media pe 5 citiri
    Serial.print("Greutate estimată [g]: ");
    Serial.println(greutate, 2);

    if (greutate < 1) {
      servoMotor.write(0);  // deschis
    } else {
      servoMotor.write(90);   // închis
    }

  }

  delay(50);  
}


