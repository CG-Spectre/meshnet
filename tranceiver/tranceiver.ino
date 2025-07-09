#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";
int lastWrite = 0;
int count = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);  // Give time for serial monitor to open

  Serial.println("Starting nRF24L01+ test...");

  if (!radio.begin()) {
    Serial.println("ERROR: nRF24L01+ module not responding!");
    radio.printDetails(); 
    Serial.print("Is chip connected? ");
Serial.println(radio.isChipConnected() ? "YES" : "NO");

Serial.print("CONFIG register: 0x");
uint8_t config;
radio.read(&config, 1);  // Not CONFIG specifically, but triggers SPI read
Serial.println(config, HEX);
    while (1); // Stop execution if failed
  }

  radio.setPALevel(RF24_PA_MIN);
  radio.setDataRate(RF24_1MBPS); // Optional: explicitly set
  radio.setChannel(108);
  radio.openReadingPipe(1, address);
  radio.openWritingPipe(address);
  radio.startListening();
   //radio.printDetails(); 
  Serial.println("nRF24L01+ initialized successfully.");
  Serial.println("Listening for incoming messages...");
  radio.printDetails(); 
  lastWrite = millis();
}

void loop() {
  if (radio.available()) {
    char text[32] = {0};
    radio.read(&text, sizeof(text));
    Serial.print("Received: ");
    Serial.println(text);
  }
  if(millis() - lastWrite > 1000){
    radio.stopListening();
    char buffer[100];
    buffer[0] = '\0';
    sprintf(buffer, "%s%d", "Hello World: ", count);
    count++;
    radio.write(&buffer, sizeof(buffer));
    Serial.print("Wrote: ");
    Serial.println(buffer);
    radio.startListening();
    lastWrite = millis();
  }
  //radio.stopListening();
  //radio.startListening();
}