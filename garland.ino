//************************************************************
// nodeId = 2224853816
// Строгий режим: приймаємо ТІЛЬКИ повідомлення з CRC "*XX"
// і з коректною сумою. Відповіді шлемо завжди з CRC.
//************************************************************
#include <painlessMesh.h>
#include "mash_parameter.h"
#include <stdlib.h>   // strtol для парсингу hex CRC

Scheduler userScheduler;
painlessMesh  mesh;

static const int RELAY_PIN = 5;

// Твоя змінна стану (0/1). Лишаю byte/char-семантику, як було.
uint8_t buttonState = 1;

// ===== CRC8 Dallas (poly 0x31, ref 0x8C) =====
// Обчислює контрольну суму для довільного буфера.
uint8_t crc8_dallas(const uint8_t* data, size_t len){
  uint8_t crc = 0;
  while (len--){
    uint8_t inbyte = *data++;
    for (uint8_t i = 8; i; --i){
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

// Додає до рядка суфікс "*XX" (де XX — hex CRC), напр. "ok*5A"
String addCrcStar(const String& s){
  uint8_t c = crc8_dallas((const uint8_t*)s.c_str(), s.length());
  char buf[4]; snprintf(buf, sizeof(buf), "%02X", c);
  return s + "*" + String(buf);
}

// Перевіряє, що в кінці є "*XX" і CRC валідний; якщо так — повертає body без CRC.
bool stripAndVerifyCrcStar(const String& in, String& out){
  int p = in.lastIndexOf('*');
  if (p < 0 || p+3 != (int)in.length()) return false;   // немає CRC або не в кінці
  String hex = in.substring(p+1);
  char* endp = nullptr;
  long v = strtol(hex.c_str(), &endp, 16);
  if (!hex.length() || endp==nullptr || *endp!='\0' || v<0 || v>255) return false;
  String body = in.substring(0,p);
  uint8_t calc = crc8_dallas((const uint8_t*)body.c_str(), body.length());
  if (calc != (uint8_t)v) return false;
  out = body;
  return true;
}

// Короткий хелпер: широкомовно з додаванням CRC
inline void sendB(const String& s){ mesh.sendBroadcast(addCrcStar(s)); }

// Публікує поточний стан у двох форматах (як у тебе було)
void publishState(){
  if (buttonState == 0){
    sendB("garland_off");
    sendB("garl0");
  } else {
    sendB("garland_on");
    sendB("garl1");
  }
}

// ======= СТРОГИЙ прийом з CRC =======
// Приймаємо лише повідомлення з валідним CRC "*XX".
void receivedCallback(uint32_t from, String &msg){

  String in = msg; in.trim();
  if (!in.length()) return;

  // 1) Строга перевірка CRC
  String body;
  if (!stripAndVerifyCrcStar(in, body)){
    // Будь-що без валідного CRC — ігноруємо
    return;
  }

  // 2) Твоя логіка команд (працюємо з "чистим" body без "*XX")
  if (body == "garland"){
    // toggle
    buttonState = (buttonState == 1) ? 0 : 1;
    publishState();
    return;
  }

  if (body == "garland_echo"){
    publishState();
    return;
  }

  // Інше — поки ігноруємо або додавай далі:
  // if (body == "xyz"){ ...; sendB("xyz_ok"); return; }
}

void setup() {
  Serial.begin(115200);

  // Зберігаю твою модель керування: "вимкнено" = INPUT (Hi-Z), "увімкнено" = OUTPUT
  if (buttonState == 0) pinMode(RELAY_PIN, INPUT);
  else                  pinMode(RELAY_PIN, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  // (опціонально) Початковий репорт стану:
  publishState();
}

void loop() {
  mesh.update();

  // Як у твоєму коді — перемикаємо режим піну залежно від стану
  if (buttonState == 0) {
    pinMode(RELAY_PIN, OUTPUT);
  } else {
    pinMode(RELAY_PIN, INPUT);
  }
}
