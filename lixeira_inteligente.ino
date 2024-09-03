#include <Servo.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Base64.h>

Servo servo;
const int pinoServo = 8;

const int pinoEchoPresenca = 6;
const int pinoTrigPresenca = 7;

const int pinoEchoCapacidade = 3;
const int pinoTrigCapacidade = 4;

const int ANGULO_INICIAL_SERVO = 5;
const int ANGULO_FINAL_SERVO = 90;

const int DISTANCIA_ACIONAMENTO = 20;
const int LIMITE_CAPACIDADE = 5;

// Configurações de rede
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetClient ethClient;

// Configurações do servidor MQTT
const char* mqtt_server = "200.143.224.99";
const int mqtt_port = 1883;
PubSubClient client(ethClient);

void setup() {
    Serial.begin(9600);

    // Configurações iniciais do servo
    servo.attach(pinoServo);
    acionarServo(ANGULO_INICIAL_SERVO);

    // Configuração dos pinos
    pinMode(pinoTrigPresenca, OUTPUT);
    pinMode(pinoEchoPresenca, INPUT);
    pinMode(pinoTrigCapacidade, OUTPUT);
    pinMode(pinoEchoCapacidade, INPUT);

    // Inicialização da conexão Ethernet com DHCP
    if (Ethernet.begin(mac) == 0) {
        Serial.println("Falha ao configurar DHCP");
        while (true) {
            delay(1);
        }
    }

    // Configuração do servidor MQTT
    client.setServer(mqtt_server, mqtt_port);
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Tentando se reconectar ao servidor MQTT...");
        if (client.connect("arduinoClient")) {
            Serial.println("Conectado!");
        } else {
            Serial.print("Falha, rc=");
            Serial.print(client.state());
            Serial.println(" Tentando novamente em 5 segundos...");
            delay(5000);
        }
    }
}

float medirDistancia(int pinoTrig, int pinoEcho) {
    digitalWrite(pinoTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(pinoTrig, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinoTrig, LOW);

    long duracao = pulseIn(pinoEcho, HIGH);
    return (duracao / 2.0) / 29.1;
}

void acionarServo(int angulo) {
    servo.write(angulo);
}

void moverServoGradualmente(int anguloInicial, int anguloFinal, int incremento = 1, int delayTempo = 20) {
    if (anguloInicial < anguloFinal) {
        for (int angulo = anguloInicial; angulo <= anguloFinal; angulo += incremento) {
            servo.write(angulo);
            delay(delayTempo);
        }
    } else {
        for (int angulo = anguloInicial; angulo >= anguloFinal; angulo -= incremento) {
            servo.write(angulo);
            delay(delayTempo);
        }
    }
}

void verificarSensores(float distanciaCapacidade, float distanciaPresenca) {
    unsigned long tempoAtual = millis();

    if (distanciaCapacidade < LIMITE_CAPACIDADE) {
        if (!client.connected()) {
            reconnect();
        }

        String msgCapacidade = "Capacidade baixa: " + String(distanciaCapacidade, 2) + " cm";

        // Codificação Base64 da mensagem
        char msgEncoded[100];
        int encodedLength = Base64.encode(msgEncoded, msgCapacidade.c_str(), msgCapacidade.length());
        msgEncoded[encodedLength] = '\0';

        // Publicação da mensagem codificada
        client.publish("lixeira-inteligente", msgEncoded);
        Serial.println("Mensagem codificada: " + String(msgEncoded));

    } else {
        if (distanciaPresenca <= DISTANCIA_ACIONAMENTO) {
            moverServoGradualmente(ANGULO_INICIAL_SERVO, ANGULO_FINAL_SERVO);
            delay(5000);
        } else {
            moverServoGradualmente(servo.read(), ANGULO_INICIAL_SERVO);
        }
    }
}

void loop() {

    float distanciaPresenca = medirDistancia(pinoTrigPresenca, pinoEchoPresenca);
    Serial.print("Distância de Presença: ");
    Serial.print(distanciaPresenca);
    Serial.println(" cm");

    float distanciaCapacidade = medirDistancia(pinoTrigCapacidade, pinoEchoCapacidade);
    Serial.print("Distância de Capacidade: ");
    Serial.print(distanciaCapacidade);
    Serial.println(" cm");

    verificarSensores(distanciaCapacidade, distanciaPresenca);

    client.loop();
    Serial.println("_______________________________________________");
    delay(2000);
}
