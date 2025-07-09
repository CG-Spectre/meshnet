#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00000";
int lastPing = 0;
int count = 0;
char id[6];
char reach[100][6];
uint8_t reachLength = 0;
char reqsOut[100][11];
uint8_t reqsOutLength = 0;

struct Packet{
  uint8_t req;
  char rid[11];
  uint16_t order;
  char payload[18];
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing radio module...");
  while(!radio.begin()){
    Serial.println("ERROR: Radio module not responding!");
    delay(1000);
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(108);
  radio.openReadingPipe(1, address);
  radio.setAutoAck(true);
  radio.openWritingPipe(address);
  radio.startListening();
  Serial.println("Radio module initialized successfully.");
  Serial.println("Waiting for ID...");
  while(!Serial.available()){
    // waiting
  }
  Serial.readStringUntil('\n').toCharArray(id, sizeof(id));
  Serial.print("ID is: ");
  Serial.println(id);
  lastPing = millis();
  Serial.println("Mesh module successfully initialized.");
}

bool reInitializeModule(){
  if(!radio.begin()){
    return false;
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(108);
  radio.openReadingPipe(1, address);
  radio.openWritingPipe(address);
  radio.startListening();
  return true;
}

void handlePing(char* id, char* rid){
  Serial.print("Received ping from: ");
  Serial.println(id);
  //Serial.println(reachLength);
  bool found = false;
  for(int i = 0; i < reachLength; i++){
    if(strcmp(reach[i], id) == 0){
      found = true;
      break;
    }
  }
  if(!found){
    strncpy(reach[reachLength], id, sizeof(reach[reachLength]));
    reachLength++;
  }
}

void handleGET(char* id, char* rid, char* payload){
  if(strcmp(id, payload)){
    Packet packet;
    strncpy(packet.rid, rid, sizeof(packet.rid));
    strcpy(packet.payload, "Response");
    packet.req = 2;
    radio.stopListening();
    radio.write(&packet, sizeof(packet));
    radio.startListening();
    
  }
}

void handleRESP(char* id, char* rid, char* payload){
  bool found = false;
  for(int i = 0; i < reqsOutLength; i++){
    if(strcmp(rid, reqsOut[i]) == 0){
      found = true;
      break;
    }
  }
  if(found){
    Serial.println(payload);
  }
}

void newRequestOut(char* rid){
  strncpy(reqsOut[reqsOutLength], rid, sizeof(reqsOut[reqsOutLength]));
  reqsOutLength++;
}
void loop() {
  if(!radio.isChipConnected()){
    while(!reInitializeModule()){
      Serial.println("Disconnected!");
      delay(1000);
    }
    Serial.println("Reinitialized!");
  }
  if (radio.available()) {
    Packet receivedPacket;
    radio.read(&receivedPacket, sizeof(receivedPacket));
    char id[6];
    char rid[11];
    strcpy(rid, receivedPacket.rid);
    char *colonPos = strchr(receivedPacket.rid, ':');
    strncpy(id, receivedPacket.rid, colonPos - receivedPacket.rid);
    switch(receivedPacket.req){
      case 0:
        handlePing(id, rid);
        break;
      case 1:
        handleGET(id, rid, receivedPacket.payload);
        break;
      case 2:
        handleRESP(id, rid, receivedPacket.payload);
        break;
      default:
        Serial.print("Received packet with RID: ");
        Serial.println(receivedPacket.rid);
        Serial.print("Request ID: ");
        Serial.println(receivedPacket.req);
        break;
    }
  }
  if(millis() - lastPing > 2000){
    char rid[11];
    rid[0] = '\0';
    sprintf(rid, "%s:%d", id, random(0, 1000));
    Packet packet;
    packet.req = 0;
    strncpy(packet.rid, rid, sizeof(packet.rid));
    radio.stopListening();
    bool success = radio.write(&packet, sizeof(packet));
    radio.startListening();
    if(success){
      Serial.println("Pinged presence.");
    }else{
      Serial.println("Ping failed. No automatic acknowledgement received.");
    }
    
    lastPing = millis();
  }
  if(Serial.available()){
    String req = Serial.readStringUntil('\n');
    if(req.startsWith("GET")){
      String url = req.substring(4);
      Packet packet;
      packet.req = 1;
      char rid[11];
      rid[0] = '\0';
      sprintf(rid, "%s:%d", id, random(0, 1000));
      newRequestOut(rid);
      strncpy(packet.rid, rid, sizeof(packet.rid));
      if(url.length() < 18){
        char payload[18];
        url.toCharArray(payload, sizeof(payload));
        strncpy(packet.payload, payload, sizeof(packet.payload));
      }
      radio.stopListening();
      radio.write(&packet, sizeof(packet));
      radio.startListening();
    }
  }
}


