#include <WiFi.h>
#include <esp_now.h>
#include <functional>

// Change this to the MAC address of the *other* ESP32
uint8_t peerAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
std::function<void(String)> callback;
bool callbackValid = false;

char id[6] = "0";

typedef struct Packet{
  uint8_t req;
  char rid[11];
  uint16_t order;
  char payload[32];
};

char reqsOut[100][11];
uint16_t reqsOutSize = 0;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);
  Packet incomingData;
  memcpy(&incomingData, data, sizeof(incomingData));
  char idReceived[6];
  char rid[11];
  char* colonPos = strchr(incomingData.rid, ':');
  if (colonPos != nullptr) {
    int len = colonPos - incomingData.rid;
    len = min(len, (int)(sizeof(idReceived) - 1));
    strncpy(idReceived, incomingData.rid, len);
    idReceived[len] = '\0';
  }else{
    strncpy(idReceived, "null", sizeof("null"));
  }
  //strcpy(rid, incomingData.rid);
  //char *colonPos = strchr(incomingData.rid, ':');
  //strncpy(idReceived, incomingData.rid, colonPos - incomingData.rid);
  switch(incomingData.req){
    case 0:
      handlePing(idReceived);
      break;
    case 1:
      handleGETRequest(idReceived, incomingData.rid, incomingData.payload, incomingData.order);
      break;
    case 2:
      handleRESPRequest(idReceived, incomingData.rid, incomingData.payload, incomingData.order);
      break;
    default:
      Serial.printf("Received unknown request %d.\n", incomingData.req);
  }
  //Serial.printf("Received from %s | Number: %s | Message: %s\n", macStr, incomingData.rid, incomingData.payload);
}

void handleGETRequest(char incomingId[6], char incomingRId[11], char incomingPayload[32], uint16_t order){
  //Serial.printf("%s from %s request %s\n", incomingPayload, incomingId, incomingRId);
  String payload = String(incomingPayload);
//  strncpy(payload, incomingPayload, sizeof(payload));
  if(payload.startsWith(id)){
    Serial.printf("HANDLE GET %s\n", payload);
    String response = "";
    //Serial.println(incomingRId);
    String ridCopy = String(incomingRId);
    callback = [ridCopy](String response){
      Packet outbound;
      memset(outbound.payload, 0, sizeof(outbound.payload));
      response.toCharArray(outbound.payload, sizeof(outbound.payload));
      strncpy(outbound.rid, ridCopy.c_str(), sizeof(outbound.rid));
      outbound.req = 2;
      outbound.order = 0;
      esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&outbound, sizeof(outbound));
      //Serial.println(incomingRId);
    };

    callbackValid = true;
  }
}

void handleRESPRequest(char incomingId[6], char incomingRId[11], char incomingPayload[32], uint16_t order){
  //Serial.printf("%s from %s request %s\n", incomingPayload, incomingId, incomingRId);
  if(strcmp(incomingId, id) == 0){
    //Serial.println(incomingRId);
    bool found = false;
    for(int i = 0; i < reqsOutSize; i++){
      //Serial.println(reqsOut[i]);
      if(strcmp(reqsOut[i], incomingRId) == 0){
        found = true;
        break;
      }
    }
    //Serial.println(found);
    if(found){
      Serial.println(incomingPayload);
    }
  }
}

void handlePing(char incomingId[6]){
  Serial.printf("Ping received from %s.\n", incomingId);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //Serial.print("Last Packet Send Status: ");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting up...");
  // Set device as Wi-Fi Station
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(peerAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
      return;
    }
  }
  Serial.println("Module booted.");
}

unsigned long lastSendTime = 0;
int count = 0;

void makeGetRequest(String url){
  Packet outgoingData;
  snprintf(outgoingData.payload, sizeof(outgoingData.payload), "%s", url);
  snprintf(outgoingData.rid, sizeof(outgoingData.rid), "%s:%d", id, random(0, 1000));
  outgoingData.req = 1;
  outgoingData.order = 0;
  esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));
  if (result == ESP_OK) {
    strncpy(reqsOut[reqsOutSize], outgoingData.rid, sizeof(reqsOut[reqsOutSize]));
    reqsOutSize++;
    Serial.println("Request sent.");
  } else {
    Serial.println("Request failed.");
  }
}

void loop() {
  if (millis() - lastSendTime > 3000) {
    Packet outgoingData;
    count++;
    snprintf(outgoingData.payload, sizeof(outgoingData.payload), "", 0);
    snprintf(outgoingData.rid, sizeof(outgoingData.rid), "%s:%d", id, random(0, 1000));
    outgoingData.req = 0;

    esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));

    if (result == ESP_OK) {
      //Serial.println("Pinging presence.");
    } else {
      //Serial.println("Could not ping.");
    }

    lastSendTime = millis();
  }
  String input = "";
  bool wasAvailable = false;
  while(Serial.available()){
    wasAvailable = true;
    input += (char)Serial.read();
  }
  if(wasAvailable){
    if(input.startsWith("GET")){
      String url = input.substring(4);
      Serial.print("Making GET request to ");
      Serial.println(url);
      makeGetRequest(url);
    }else if(input.startsWith("ID")){
      String newId = input.substring(3);
      newId.toCharArray(id, sizeof(id));
    }else if(callbackValid){
      callbackValid = false;
      callback(input);
    }
  }
}