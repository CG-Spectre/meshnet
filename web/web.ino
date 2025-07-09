#include <WiFiS3.h>

char ssid[] = "Namma";       // Replace with your network SSID
char pass[] = "MisoNori2015";   // Replace with your Wi-Fi password

WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Connecting to WiFi...");
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Wait for command from PC
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.startsWith("GET ")) {
      String url = cmd.substring(4);
      const char* resp = makeGETRequest(url);
      Serial.println("!!&&START&&!!");
      Serial.println(resp);
    } else {
      Serial.println("Unknown command. Use: GET <hostname>");
    }
    Serial.println("!!&&END&&!!");
  }
}

const char* makeGETRequest(String host) {
  static char buffer[10000] = {0};
  const int port = 80;

  Serial.print("Connecting to ");
  Serial.println(host);
  int readingCount = 0;
  if (client.connect(host.c_str(), port)) {
    Serial.println("Connected. Sending request...");

    client.println("GET / HTTP/1.1");
    client.print("Host: ");
    client.println(host);
    client.println("Connection: close");
    client.println();
    
    while (client.connected() || client.available()) {
      if (client.available()) {
        char c = client.read();
        if(readingCount % 100 == 0){
          Serial.print(readingCount);
          Serial.println(" Reading...");
        }
        buffer[readingCount] = c;
        readingCount++;
        
      }
    }
    client.stop();
    Serial.println("\nConnection closed");
  } else {
    Serial.println("Connection failed");
  }
  buffer[readingCount] = '\0';
  return buffer;
}