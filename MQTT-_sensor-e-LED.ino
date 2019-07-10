#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h> 
#include <PubSubClient.h>

//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1

//sendores de temperatura e umidade
#define DHTTYPE1    DHT11     // DHT 11
DHT_Unified dht1(D3, DHTTYPE1);
#define DHTTYPE2    DHT22     // DHT 22 (AM2302)
DHT_Unified dht2(D5, DHTTYPE2);
uint32_t delayMS;

//WiFi
const char* SSID = "Lucas Giovani";                // SSID / nome da rede WiFi que deseja se conectar
const char* PASSWORD = "59456797";   // Senha da rede WiFi que deseja se conectar
WiFiClient wifiClient;                        
 
//MQTT Server
const char* BROKER_MQTT = "iot.eclipse.org"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883;                      // Porta do Broker MQTT

#define ID_MQTT  "ESPSalgado1"            //Informe um ID unico e seu. Caso sejam usados IDs repetidos a ultima conexão irá sobrepor a anterior. 
#define TOPIC_PUBLISH "SalgadoEnvia"    //Informe um Tópico único. Caso sejam usados tópicos em duplicidade, o último irá eliminar o anterior.
#define TOPIC_SUBSCRIBE "SalgadoRecebe" //Informe um Tópico único. Caso sejam usados tópicos em duplicidade, o último irá eliminar o anterior.
PubSubClient MQTT(wifiClient);        // Instancia o Cliente MQTT passando o objeto espClient

//Declaração das Funções
void mantemConexoes();  //Garante que as conexoes com WiFi e MQTT Broker se mantenham ativas
void conectaWiFi();     //Faz conexão com WiFi
void conectaMQTT();     //Faz conexão com Broker MQTT
void enviaPacote();     //
void recebePacote(char* topic, byte* payload, unsigned int length);

void setup() {
  pinMode(D2, OUTPUT); // definindo o pino D2 para controlar o LED anexado

  Serial.begin(115200);

  dht1.begin(); // inicializando o dht11
  dht2.begin(); // inicializando o dht22
  sensor_t sensor1;
  sensor_t sensor2;
  dht1.temperature().getSensor(&sensor1);
  dht2.temperature().getSensor(&sensor2);
  dht1.humidity().getSensor(&sensor1);
  dht2.humidity().getSensor(&sensor2);
  delayMS = sensor1.min_delay / 1000;
  delayMS = sensor2.min_delay / 1000;
  
  conectaWiFi();
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(recebePacote);   
}

void loop() {
  mantemConexoes();
  MQTT.loop();
}

void mantemConexoes() {
    if (!MQTT.connected()) {
       conectaMQTT(); 
    }
    
    conectaWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

void conectaWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
     return;
  }
        
  Serial.print("Conectando-se na rede: ");
  Serial.print(SSID);
  Serial.println("  Aguarde!");

  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI  
  while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Conectado com sucesso, na rede: ");
  Serial.print(SSID);  
  Serial.print("  IP obtido: ");
  Serial.println(WiFi.localIP()); 
}

void conectaMQTT() { 
    while (!MQTT.connected()) {
        Serial.print("Conectando ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado ao Broker com sucesso!");
            MQTT.subscribe(TOPIC_SUBSCRIBE);
        } 
        else {
            Serial.println("Nao foi possivel se conectar ao broker.");
            Serial.println("Nova tentatica de conexao em 5s");
            delay(5000);
        }
    }
}

void recebePacote(char* topic, byte* payload, unsigned int length){

     String msg;

    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }

  if (msg == "0") {
    digitalWrite(D2, LOW);   // Desliga o LED caso receba o valor 0
    
  }
  else if(msg == "1") {
    digitalWrite(D2, HIGH);  // Liga o LED caso receba o valor 1
    
  }
  else if(msg == "2"){ // Envia informações do sensor ligado ao pino D3 caso receba o valor 2
    
    sensors_event_t event1;
    dht1.temperature().getEvent(&event1);
    MQTT.publish(TOPIC_PUBLISH, String(event1.temperature).c_str()); // como a funcão publish só envia Strings,
    dht1.humidity().getEvent(&event1); //  foi necessário converter as informações vindas do sensor de float para string.
    MQTT.publish(TOPIC_PUBLISH, String(event1.relative_humidity).c_str());
    
  }
  else if(msg == "3"){ // Envia informações do sensor ligado ao pino D5 caso receba o valor 3
    
    sensors_event_t event2;
    dht2.temperature().getEvent(&event2);
    MQTT.publish(TOPIC_PUBLISH, String(event2.temperature).c_str());
    dht2.humidity().getEvent(&event2);
    MQTT.publish(TOPIC_PUBLISH, String(event2.relative_humidity).c_str());
    
  }

}
