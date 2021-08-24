void setup() {
  Serial.begin(115200);
  delay(1000); // give me time to bring up serial monitor
}

void loop() {
  //for(int i=4; i<20; i++) Serial.printf("%d ", touchRead(i)); 
  Serial.printf("%d %d", touchRead(4), touchRead(14)); 
  Serial.println();
  delay(100);
}
