//************************************************************
// nodeId = 2224853816
//************************************************************
#include <painlessMesh.h>
#include "mash_parameter.h"
#include "CRCMASH.h"

Scheduler userScheduler;

static const int RELAY_PIN = 5;      // як у твоєму прикладі
uint8_t buttonState = 1;             // 0/1 — стан "гірлянди"

void publishState(){
  if (buttonState == 0){
    sendB("garland_off");
    sendB("garl0");
  } else {
    sendB("garland_on");
    sendB("garl1");
  }
}
 
// ===== Обробка одного повідомлення з черги =====
void handleBody(const String& body){
  // --- працюємо з уже перевіреним body ---
  if (body == "garland"){
    buttonState = (buttonState == 1) ? 0 : 1;
    publishState();
    return;
  }

  if (body == "garland_echo"){
    publishState();
    return;
  }

  // if (body == "xyz"){ ...; sendB("xyz_ok"); return; }
}

void setup() {
  Serial.begin(115200);

  // Початковий стан піну (за твоєю схемою в loop все одно перемикається)
  pinMode(RELAY_PIN, (buttonState==0) ? OUTPUT : INPUT);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  // Оголосити початковий стан
  publishState();
}

void loop() {
  mesh.update();

  // Обробити до кількох повідомлень за ітерацію — щоб не фризило інші задачі
  for (uint8_t i=0; i<3; ++i){
    String body;
    if (!qPop(body)) break;
    handleBody(body);
  }

  if (buttonState == 0) {
    pinMode(RELAY_PIN, OUTPUT);
  } else {
    pinMode(RELAY_PIN, INPUT);
  }
}
