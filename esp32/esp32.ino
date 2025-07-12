#include <WiFi.h>
#include <esp_now.h>
#include <functional>

// Change this to the MAC address of the *other* ESP32
uint8_t peerAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
std::function<void(String)> callback;
bool callbackValid = false;
String getData = "";

const int txPin = 4;
int lastTx;
bool txOn = false;


char id[6] = "0";

typedef struct Packet{
  uint8_t req;
  char rid[11];
  uint16_t order;
  char payload[32];
  bool lastPacket;
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
 // Serial.println(incomingData.req);
  switch(incomingData.req){
    case 0:
      handlePing(idReceived);
      break;
    case 1:
      handleGETRequest(idReceived, incomingData.rid, incomingData.payload, incomingData.order);
      break;
    case 2:
      handleRESPRequest(idReceived, incomingData.rid, incomingData.payload, incomingData.order, incomingData.lastPacket);
      break;
    default:
      Serial.printf("Received unknown request %d.\n", incomingData.req);
  }
  //Serial.printf("Received from %s | Number: %s | Message: %s\n", macStr, incomingData.rid, incomingData.payload);
}

esp_err_t breakUpPacket(Packet packet, String response, uint16_t order){
  if(response.length() <= sizeof(packet.payload) - 1){
    packet.lastPacket = true;
  }else{
    packet.lastPacket = false;
  }
  response.toCharArray(packet.payload, sizeof(packet.payload));
  packet.order = order;
  esp_err_t result;
  result = esp_now_send(peerAddress, (uint8_t *)&packet, sizeof(packet));
  if(packet.lastPacket){
    return result;
  }
  return breakUpPacket(packet, response.substring(sizeof(packet.payload) - 1), order + 1);
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
      //Serial.println(response);
      Packet outbound;
      memset(outbound.payload, 0, sizeof(outbound.payload));
      esp_err_t result;
      outbound.req = 2;
      strncpy(outbound.rid, ridCopy.c_str(), sizeof(outbound.rid));
      //Serial.println(response.length());
      if(response.length() <= sizeof(outbound.payload) - 1){
        response.toCharArray(outbound.payload, sizeof(outbound.payload));
        outbound.order = 0;
        outbound.lastPacket = true;
        result = esp_now_send(peerAddress, (uint8_t *)&outbound, sizeof(outbound));
      }else{
        result = breakUpPacket(outbound, response, 0);
      }
      
      //Serial.println(incomingRId);
    };

    callbackValid = true;
  }
}


void handleRESPRequest(char incomingId[6], char incomingRId[11], char incomingPayload[32], uint16_t order, bool last){
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
      if(order == 0){
        getData = "";
      }
      getData += String(incomingPayload);
      if(last){
        handleRESPRequestFull(getData);
      }
    }
  }
}

void handleRESPRequestFull(String incomingPayload){
  Serial.println("RESPR " + incomingPayload);
}

void handlePing(char incomingId[6]){
  Serial.printf("Ping received from %s.\n", incomingId);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //Serial.print("Last Packet Send Status: ");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
  digitalWrite(txPin, HIGH);
  txOn = true;
  lastTx = millis();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting up...");
  pinMode(txPin, OUTPUT);
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
  int start = 0;
  if(Serial.available()/* || millis() - start < 10*/){
    wasAvailable = true;
    while(true){
      char in = (char)Serial.read();
      if(in == '$'){
        break;
      }
      input += in;
    }
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
  if(millis() - lastTx > 100 && txOn){

    digitalWrite(txPin, LOW);
    txOn = false;
  }
}