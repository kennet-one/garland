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


char buttonState = 1;

void receivedCallback( uint32_t from, String &msg ) {

  String str1 = msg.c_str();
  String str2 = "garland";
  String str3 = "garland_echo";

  if (str1.equals(str2)) {

    if (buttonState == 1) {
      buttonState = 0;
      mesh.sendSingle(2661345693,"garland_off");
      mesh.sendSingle(624409705,"garland_off");
    } else {
      buttonState++;
      mesh.sendSingle(2661345693,"garland_on");
      mesh.sendSingle(624409705,"garland_on");
    } 
  }

  if (str1.equals(str3)) {
    if (buttonState == 0) {
      mesh.sendSingle(2661345693,"garland_off");
      mesh.sendSingle(624409705,"garland_off");
    } else {
      mesh.sendSingle(2661345693,"garland_on");
      mesh.sendSingle(624409705,"garland_on");
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

  if (buttonState == 0) {
    pinMode(5, OUTPUT);
  } else {
    pinMode(5, INPUT);
  }
}
