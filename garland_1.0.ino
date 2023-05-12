//************************************************************
// nodeId = 2224853816
//
//************************************************************
#include "painlessMesh.h"

#define   MESH_PREFIX     "kennet"
#define   MESH_PASSWORD   "kennet123"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

String str1 = "";

char buttonState = 0;

void receivedCallback( uint32_t from, String &msg ) {

  str1 = msg.c_str();
  String str2 = "garland";

  if (str1.equals(str2)) {

    if (buttonState == 0) {
      buttonState++;
    } else {
      buttonState = 0;
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(5, INPUT);

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);

}

void loop() {

  mesh.update();

  if (buttonState == 1) {
    pinMode(5, OUTPUT);
  } else {
    pinMode(5, INPUT);
  }
}
