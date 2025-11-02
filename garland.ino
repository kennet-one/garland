#include <Arduino.h>
#include <painlessMesh.h>
#include "mash_parameter.h"     // MESH_PREFIX, MESH_PASSWORD, MESH_PORT
#include "CRC.h"

// ========== Mesh ==========
Scheduler    userScheduler;
painlessMesh mesh;              // ← реальний екземпляр (у CRC.h — extern)

// ========== Конфіг гірлянди ==========
static const int RELAY_PIN   = 5;  // як у тебе
static uint8_t   garlandState = 1; // 1=off, 0=on (твоя логіка IN/OUT)

static void applyRelay(){
	if (garlandState == 0){
		pinMode(RELAY_PIN, OUTPUT);
		// тут можна встановити рівень HIGH/LOW якщо потрібен
	} else {
		pinMode(RELAY_PIN, INPUT);  // як у тебе: INPUT = "вимкнено"
	}
}

static void publishGarland(){
	if (garlandState == 1){
		sendB("garland_on");
		sendB("garl1");
	} else {
		sendB("garland_off");
		sendB("garl0");
	}
}

// ========== Обробка команд (з дефер-черги) ==========
static void handleBody(uint32_t from, const String& body){
	// Тумблер
	if (body == "garland"){
		garlandState = (garlandState == 0) ? 1 : 0;
		applyRelay();
		publishGarland();
		return;
	}
	// Ехо стану
	if (body == "garland_echo"){
		publishGarland();
		return;
	}
	// Alias на випадок, якщо хтось шле напряму
	if (body == "garl1"){
		garlandState = 0; applyRelay(); publishGarland(); return;
	}
	if (body == "garl0"){
		garlandState = 1; applyRelay(); publishGarland(); return;
	}

	// приклад адресного ACK (вимкнено за замовчуванням)
	// if (body == "ping_garland"){ sendS(from, "pong_garland"); return; }
}

// ========== Setup / Loop ==========
void setup(){
	Serial.begin(115200);
	delay(50);
	Serial.println("Garland node boot...");

	applyRelay();                 // привести пін до стану
	WiFi.setSleep(false);         // нижча латентність Wi-Fi (опціонально)

	mesh.setDebugMsgTypes(ERROR | STARTUP);
	mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
	mesh.onReceive(&receivedCallback);
	// Якщо нода біля мосту — можна зафіксувати root:
	// mesh.setRoot(true); mesh.setContainsRoot(true);

	// Оголосимо поточний стан у мережу
	publishGarland();
}

void loop(){
	mesh.update();

	// дефер-обробка вхідних повідомлень (до 4 за ітерацію)
	for (uint8_t i=0; i<4; ++i){
		uint32_t from; String body;
		if (!qPop2(from, body)) break;
		handleBody(from, body);
	}
}
