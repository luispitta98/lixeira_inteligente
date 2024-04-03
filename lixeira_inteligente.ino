#include <Servo.h>                  // Biblioteca do Servo Motor
#include <Ethernet.h>               // Biblioteca do Ethernet
#include <PubSubClient.h>           // Biblioteca MQTT
#include <UniversalTelegramBot.h>   // Biblioteca do Telegram

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    // Endereço MAC
IPAddress ip(192, 168, 1, 100);                         // Endereço IP do Arduino

// Configuração do Telegram
#define BOTtoken "SEU_TOKEN_DO_BOT"
#define CHAT_ID "SEU_CHAT_ID"
UniversalTelegramBot bot(BOTtoken);

// Configuração do MQTT
const char* mqtt_server = "bsi.cefet-rj.br";
const int mqtt_port = 1883;
EthernetClient ethClient;
PubSubClient client(ethClient);

Servo servo;     
int pinoTrigPresenca = 5;                           // Pino do sensor sonar para medir presença
int pinoEchoPresenca = 6;   
int pinoTrigCapacidade = 8;                         // Pino do sensor sonar para medir capacidade da lixeira
int pinoEchoCapacidade = 9;   
int pinoServo = 7;                                  // Pino do servo motor
long duracao, distPresenca, distCapacidade, media;
const int LIMITE_VAZIO = 20;                        // Limite inferior para lixeira vazia
const int LIMITE_BAIXO = 40;                        // Limite inferior para baixa capacidade
const int LIMITE_MEDIO = 60;                        // Limite inferior para média capacidade

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle message arrived
}

void reconnect() {
  // Loop até estar conectado
    while (!client.connected()) {
        Serial.print("Tentando se conectar ao MQTT...");
        // Tentativa de conexão
        if (client.connect("ArduinoClient")) {
            Serial.println("Conectado ao MQTT");
            // Assina o tópico de controle
            client.subscribe("topico/controle");
        } else {
            Serial.print("Falha na conexão, rc=");
            Serial.print(client.state());
            Serial.println("Tentando novamente em 5 segundos");
            // Aguarda 5 segundos antes de tentar novamente
            delay(5000);
        }
    }
}

void setup() {       
    Serial.begin(9600);
    servo.attach(pinoServo);  
    pinMode(pinoTrigPresenca, OUTPUT);  
    pinMode(pinoEchoPresenca, INPUT);  
    pinMode(pinoTrigCapacidade, OUTPUT);  
    pinMode(pinoEchoCapacidade, INPUT);  
    servo.write(0);                                 // Inicia com a tampa fechada
    delay(100);
    servo.detach(); 

    // Inicialização do Ethernet
    Ethernet.begin(mac, ip);
    Serial.println("Ethernet conectado");

    // Inicialização do MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void medir(int pinoTrig, int pinoEcho, long &dist) {  
    digitalWrite(10, HIGH);
    digitalWrite(pinoTrig, LOW);
    delayMicroseconds(5);
    digitalWrite(pinoTrig, HIGH);
    delayMicroseconds(15);
    digitalWrite(pinoTrig, LOW);
    pinMode(pinoEcho, INPUT);
    duracao = pulseIn(pinoEcho, HIGH);
    dist = (duracao/2) / 29.1;    // Obtem a distancia
}

void loop() { 
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Verifica a presença antes de abrir a tampa
    medir(pinoTrigPresenca, pinoEchoPresenca, distPresenca);
    if (distPresenca < 50) {
        servo.attach(pinoServo);
        delay(1);
        servo.write(0);  
        delay(3000);       
        servo.write(150);    
        delay(1000);
        servo.detach();      
    }
    
    // Mede a capacidade da lixeira usando o sensor sonar
    medir(pinoTrigCapacidade, pinoEchoCapacidade, distCapacidade);
    
    // Determina a capacidade com base na distância medida
    int nivelCapacidade;
    if (distCapacidade < LIMITE_VAZIO) {
        nivelCapacidade = 0; // Lixeira vazia
    } else if (distCapacidade >= LIMITE_VAZIO && distCapacidade < LIMITE_BAIXO) {
        nivelCapacidade = 1; // Baixa capacidade
    } else if (distCapacidade >= LIMITE_BAIXO && distCapacidade < LIMITE_MEDIO) {
        nivelCapacidade = 2; // Média capacidade
    } else {
        nivelCapacidade = 3; // Alta capacidade
    }
    
    // Imprime a capacidade da lixeira
    Serial.print("Capacidade da lixeira: ");
    switch(nivelCapacidade) {
        case 0:
            Serial.println("Vazia");
            break;
        case 1:
            Serial.println("Baixa");
            break;
        case 2:
            Serial.println("Média");
            break;
        case 3:
            Serial.println("Alta");
            break;
        default:
            Serial.println("Indefinida");
            break;
    }

    // Notifica via Telegram
    if (nivelCapacidade >= 3) {
        Serial.println("Enviando mensagem no telegram...");
        bot.sendMessage(CHAT_ID, "Atenção! A lixeira está quase cheia. Por favor, esvazie-a.");
    }

    // Publica a capacidade da lixeira no tópico MQTT
    String msg = String(nivelCapacidade);
    client.publish("topico/lixeira", msg.c_str());

    delay(1000);
}
