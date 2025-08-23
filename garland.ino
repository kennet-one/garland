//************************************************************
// nodeId = 2224853816
//
//************************************************************
#include "painlessMesh.h"
#include "mash_parameter.h"


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
      mesh.sendBroadcast("garland_off");
      mesh.sendBroadcast("garl0");
    } else {
      buttonState++;
      mesh.sendBroadcast("garland_on");
      mesh.sendBroadcast("garl1");

    } 
  }

  if (str1.equals(str3)) {
    if (buttonState == 0) {
      mesh.sendBroadcast("garland_off");
      mesh.sendBroadcast("garl0");
    } else {
      mesh.sendBroadcast("garland_on");
      mesh.sendBroadcast("garl1");
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
