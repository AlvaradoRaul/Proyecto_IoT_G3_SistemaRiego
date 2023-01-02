//Librerias Utilizadas
#include <ESP8266WiFi.h>//Libreria ESP8266 de Conexion a red
#include <PubSubClient.h>//Libreria Publish/Subscribe con un servidor MQTT
#include <DHTesp.h>//Libreria del sensor DHT
DHTesp dht; //Variable dht
// PIN de los led rgb
int Rojo = 5;
int Verde = 4;
int Azul = 0;
// PIN de la minibomba de agua
float bomba = 2;
// PIN del sensor de humedad de suelo
#define SensorPin A0
float humedad_suelo = 0;//Variable Humedad_Suelo

//Datos de la conexion a internet
const char* ssid = "AndroidAP"; //Nombre del router
const char* password = "12345687"; // clave del router
const char* mqtt_server = "192.168.1.15"; // direccion Ip del Servidor MQTT

// Variables de conexion MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Variables globales
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];// Se almacenar los datos de los sensores para enviarselo al Node-Red
char dth22_temp[MSG_BUFFER_SIZE]; // DTH22 Temperatura
char dth22_hum[MSG_BUFFER_SIZE];// DTH22 Humedad
char hum[MSG_BUFFER_SIZE]; // Humedad de suelo


void setup() {
  // Inicializando los PINES
  //ENTRADA
  pinMode(SensorPin, INPUT);
  //SALIDA
  pinMode(Rojo,OUTPUT);
  pinMode(Verde,OUTPUT);
  pinMode(Azul,OUTPUT);
  pinMode(bomba, OUTPUT);
  //Pin del sensor DHT22
  dht.setup(16,DHTesp::DHT22);
  //Iniciamos monitor de Serial
  Serial.begin(115200);
  //Iniciamos la Funcion WIFI
  inicializando_wifi();
  //Iniciamos el cliente
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // Comprobamos la conexion
  if (!client.connected()) {
     reconnect();
  }  
  client.loop();
  // Se envia los datos cada 25000 milisegundos
  long tiempo = millis();
  if(tiempo-lastMsg > 25000)
  {
    lastMsg = tiempo;
  }
  // Lectura de sensores
  TempAndHumidity valor = dht.getTempAndHumidity(); // Valores del sensor DHT22
  humedad_suelo = analogRead(SensorPin);// Valor del sensor humedad de suelo
  ledRGB(valor.temperature); // funcion para manipular el led rgb
  //Se imprimen los datos en el monitor serial
  Serial.println(" Humedad del ambiente: " + String(valor.humidity, 1)+ "%");
  Serial.println(" Temperatura: " + String(valor.temperature, 2)+ "C°\n");
  Serial.println("Humedad del suelo: " + String(humedad_suelo));
  delay(5000);
  // Conversion de datos para publicarlos
  sprintf(dth22_temp, "%3.2f", valor.temperature);
  sprintf(dth22_hum, "%3.2f", valor.humidity);
  sprintf(hum, "%3.2f", humedad_suelo);
  // Publicando datos de los sensores en NODE-RED
  client.publish("esp8266/temperatura", dth22_temp);
  client.publish("esp8266/humedad", dth22_hum);
  client.publish("esp8266/humedadSuelo", hum);
  //Funcion de encendido de bomba de agua
  enciendeBomba(humedad_suelo);
}

// Conexion a WIFI
void inicializando_wifi() {
  delay(10);
  // Constancia de conexion
  Serial.println();
  Serial.print("Conectado a ");
  Serial.println(ssid);
  // Iniciamos la conexion a la red
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Constancia de Espera
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //Inicializamos el generador de números pseudo aleatorios
  //Función micros(): Devolverá el número de microsegundos desde que la placa empezó a ejecutar el programa
  randomSeed(micros());
  //Constancia de conexion realizada
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//Conexion MQTT
void reconnect() {
  while (!client.connected()) {
    // Mensaje de constancia de conexion
    Serial.print("Intentando conexión Mqtt...");
    // Se crea un cliente ID
    String clientId = "ESP8266";
    // Iniciamos la conexion al MQTT
      if (client.connect(clientId.c_str() ) ) {      
        // Mensaje de Conexion
        Serial.println("Conectado!");
      } 
      else {
        // Mensajes de Error de Conexion
        Serial.print("falló :( con error -> ");
        Serial.print(client.state());
        Serial.println(" Intentamos de nuevo en 5 segundos");
        delay(5000);
      }
  }
}

// Lectura de datos que llegan despues de suscribir
void callback(char* topic, byte* payload, unsigned int length) {
  //Mensaje de constancia de recepcion de envio
  Serial.print("Mensaje llegado[");
  Serial.print(topic);
  Serial.print("] ");
  //Se muestra los mensajes enviados
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

//Funcion de Encendido de Bomba de Agua
void enciendeBomba(double sensorHumedadSuelo) 
{
  // Condicion de Encendido: El valor supere 700
  if(sensorHumedadSuelo>700){
    // Se enciende la bomba de agua por 15 segundos
    digitalWrite(bomba,LOW);
    delay(15000);
    digitalWrite(bomba,HIGH);
  }
}

//Funcion de LED RGB (Este cambiara segun la lectura de la Temperatura)
void ledRGB(double lectura)
{
  //Condiciones
  if(lectura < 18){   // Temperatura menor que 18°C: Color azul(Frio)
    digitalWrite(Rojo,0);
    digitalWrite(Verde,0);
    digitalWrite(Azul,255);
  }
  if(lectura >=18 || lectura < 25) // Temperatura entre que 18°C y 25°C: Color Verde(Ambiente)
  {
    digitalWrite(Rojo,0);
    digitalWrite(Verde,255);
    digitalWrite(Azul,0);
  }
  if(lectura >= 25) // Temperatura mayor que 25°C: Color rojo(Calor)
  {
    digitalWrite(Rojo,255);
    digitalWrite(Verde,0);
    digitalWrite(Azul,0);
  }
}
