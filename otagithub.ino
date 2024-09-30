#include <WiFi.h>
#include <SPIFFS.h>
#include "Update.h"
#include <WiFiClientSecure.h>

// Definir credenciais de WiFi
#definir ssid "SSID"
#definir senha "PASSWORD"

// Definir detalhes do servidor e caminho do arquivo
#definir HOST "raw.githubusercontent.com"
#definir PATH "/adityabangde/ESP32-OTA-Update-via-GitHub/dev/ota_code.bin"
#definir PORTA 443

// Definir o nome do arquivo de firmware baixado
#definir FILE_NAME "firmware.bin"

void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("Falha na montagem do SPIFFS");
    return;
  }

  connectToWiFi();
  getFileFromServer();
  performOTAUpdateFromSPIFFS();
}

void loop() {
  // Nada a fazer aqui
}

void connectToWiFi() {
  // Comece a se conectar ao WiFi usando o SSID e a senha fornecidos
  WiFi.begin(ssid, password);

  // Exiba o progresso da conexão
  Serial.print("Conectando ao WiFi");

  // Aguarde até que o WiFi esteja conectado
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Imprima a mensagem de confirmação quando o WiFi estiver conectado
  Serial.println("WiFi conectado");
}


void getFileFromServer() {
  WiFiClientSecure client;
  client.setInsecure();  // Defina o cliente para permitir conexões inseguras

  if (client.connect(HOST, PORT)) {  // Conecte-se ao servidor
    Serial.println("Conectado ao servidor");
    client.imprimir("OBTER" + String(PATH) + " HTTP/1.1\r\n");  // Envia solicitação HTTP GET
    client.print("Host: " + String(HOST) + "\r\n");             // Especifica o host
    client.println("Connection: close\r\n");                    // Fecha a conexão após a resposta
    client.println();                                           // Envia uma linha vazia para indicar o fim dos cabeçalhos da solicitação

    File file = SPIFFS.open("/" + String(FILE_NAME), FILE_WRITE);  // Abre o arquivo em SPIFFS para gravação
    if (!file) {
      Serial.println("Falha ao abrir o arquivo para gravação");
      return;
    }

    bool endOfHeaders = false;
    String headers = "";
    String http_response_code = "error";
    const size_t bufferSize = 1024;  // Tamanho do buffer para leitura de dados
    uint8_t buffer[bufferSize];

    // Loop para ler cabeçalhos de resposta HTTP
    while (client.connected() && !endOfHeaders) {
      if (client.available()) {
        char c = client.read();
        headers += c;
        if (headers.startsWith("HTTP/1.1")) {
          http_response_code = headers.substring(9, 12);
        }
        if (headers.endsWith("\r\n\r\n")) {  // Verificar o fim dos cabeçalhos
          endOfHeaders = true;
        }
      }
    }

    Serial.println("HTTP response code: " + http_response_code);  // Imprime cabeçalhos recebidos

    // Loop para ler e gravar dados brutos no arquivo
    while (client.connected()) {
      if (client.available()) {
        size_t bytesRead = client.readBytes(buffer, bufferSize);
        file.write(buffer, bytesRead);  // Grava dados no arquivo
      }
    }
    file.fechar();  // Fecha o arquivo
    client.stop();  // Fecha a conexão do cliente
    Serial.println("Arquivo salvo com sucesso");
  } else {
    Serial.println("Falha ao conectar ao servidor");
  }
}

void performOTAUpdateFromSPIFFS() {
  // Abre o arquivo de firmware no SPIFFS para leitura
  Arquivo file = SPIFFS.open("/" + String(FILE_NAME), FILE_READ);
  if (!file) {
    Serial.println("Falha ao abrir o arquivo para leitura");
    return;
  }

  Serial.println("Iniciando atualização..");
  size_t fileSize = file.size();  // Obtém o tamanho do arquivo
  Serial.println(fileSize);

  // Inicia o processo de atualização OTA com o tamanho e o destino flash especificados
  if (!Update.begin(fileSize, U_FLASH)) {
    Serial.println("Não é possível fazer a atualização");
    return;
  }

  // Grava dados de firmware do arquivo na atualização OTA
  Update.writeStream(file);

  // Conclua o processo de atualização OTA
  if (Update.end()) {
    Serial.println("Atualização bem-sucedida");
  } else {
    Serial.println("Ocorreu um erro:" + String(Update.getError()));
    return;
  }

  file.close();  // Feche o arquivo
  Serial.println("Redefinir em 4 segundos...");
  delay(4000);
  ESP.restart();  // Reinicie o ESP32 para aplicar a atualização
}
