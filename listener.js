const mqtt = require('mqtt');
const axios = require('axios');

// Configurações do servidor MQTT
const mqtt_server = "mqtt://200.143.224.99"; 
const mqtt_port = 1883;
const topic = "lixeira-teste";

// Configurações do Telegram Bot
const telegramToken = '7052074910:AAFP9BnA0nV1gZo1lMX8VQQDglpH_qXMggk';
const chatId = '996535742';

// Função para enviar mensagens para o Telegram
function sendMessageToTelegram(message) {
    const url = `https://api.telegram.org/bot${telegramToken}/sendMessage`;
    axios.post(url, {
        chat_id: chatId,
        text: message
    })
    .then(response => {
        console.log('Mensagem enviada ao Telegram com sucesso');
    })
    .catch(error => {
        console.error('Erro ao enviar mensagem ao Telegram:', error);
    });
}

// Conectar ao broker MQTT
const client = mqtt.connect(mqtt_server, { port: mqtt_port });

client.on('connect', () => {
    console.log(`Conectado ao broker MQTT em ${mqtt_server}:${mqtt_port}`);

    // Inscrever-se no tópico
    client.subscribe(topic, (err) => {
        if (err) {
            console.error(`Falha ao se inscrever no tópico ${topic}:`, err);
        } else {
            console.log(`Inscrito no tópico: ${topic}`);
        }
    });
});

client.on('message', (topic, message) => {
    // Decodificação da mensagem Base64
    const decodedMessage = Buffer.from(message.toString(), 'base64').toString('utf-8');
    console.log(`Mensagem decodificada: ${decodedMessage}`);

    // Enviar a mensagem para o Telegram
    sendMessageToTelegram(`Mensagem recebida no tópico ${topic}: ${decodedMessage}`);

    // Aqui você pode adicionar lógica para processar a mensagem recebida
});

client.on('error', (err) => {
    console.error('Erro no cliente MQTT:', err);
});

client.on('close', () => {
    console.log('Conexão com o broker MQTT encerrada');
});
