#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <Wire.h>

// Definição de Pinos - Heltec LoRa v2
#define RELAY_BOMBA 2       
#define RELAY_OXIGENADOR 17 

// Configuração OLED Nativo Heltec V2 (Ajustado para Hardware I2C)
#define OLED_RST 16
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, /* clock=*/ 15, /* data=*/ 4);

WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool desligaAutomacao = false;
bool v1 = true, v2 = true;
String mBroker, mUser, mPass, mTopic = "hidroponia_esp";

void updateOLED() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "PAINEL HIDROPONIA");
  u8g2.drawHLine(0, 13, 128);
  u8g2.setCursor(0, 30);
  u8g2.print("IP: "); u8g2.print(WiFi.localIP().toString());
  u8g2.setCursor(0, 48);
  u8g2.print("BOMBA [P2]:  "); u8g2.print(v1 ? "ON" : "OFF");
  u8g2.setCursor(0, 62);
  u8g2.print("OXI  [P17]:  "); u8g2.print(v2 ? "ON" : "OFF");
  u8g2.sendBuffer();
}

void sendRelay(int pin, bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
  updateOLED();
}

const char htmlPage[] PROGMEM = R"=====(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  body { background:#111; color:#e1e1e1; font-family:sans-serif; padding:15px; }
  .card { background:#1c1c1e; border-radius:12px; padding:18px; margin-bottom:15px; box-shadow:0 4px 10px rgba(0,0,0,0.5); }
  h2 { font-size:14px; color:#aeaeb2; text-transform:uppercase; margin-top:0; }
  .row { display:flex; justify-content:space-between; align-items:center; margin-bottom:18px; }
  .switch { position:relative; width:46px; height:26px; }
  .switch input { opacity:0; width:0; height:0; }
  .slider { position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; background:#3e3e42; transition:.4s; border-radius:26px; }
  .slider:before { position:absolute; content:""; height:20px; width:20px; left:3px; bottom:3px; background:#fff; transition:.4s; border-radius:50%; }
  input:checked + .slider { background:#34c759; }
  input:checked + .slider:before { transform:translateX(20px); }
  .btn-set { display:block; text-align:center; padding:12px; background:#2c2c2e; color:#007aff; text-decoration:none; border-radius:8px; font-weight:bold; border:1px solid #3e3e42;}
</style></head>
<body>
<div class="card">
  <h2>Hidroponia Automática</h2>
  <div class="row"><span>Sempre Ligado (24h)</span><label class="switch"><input type="checkbox" id="autoOff" onchange="update('autoOff', this.checked)"><span class="slider"></span></label></div>
  <div class="row"><span>Bomba (Pino 2)</span><label class="switch"><input type="checkbox" id="v1" onchange="update('v1', this.checked)"><span class="slider"></span></label></div>
  <div class="row"><span>Oxigenador (Pino 17)</span><label class="switch"><input type="checkbox" id="v2" onchange="update('v2', this.checked)"><span class="slider"></span></label></div>
  <a href="/settings" class="btn-set">⚙️ CONFIGURAÇÕES MQTT</a>
</div>
<script>
function update(k,v){ fetch(`/set?${k}=${v}`); }
function load(){ fetch('/status').then(r=>r.json()).then(d=>{ 
  document.getElementById('autoOff').checked = !d.autoOff; 
  document.getElementById('v1').checked = d.v1; 
  document.getElementById('v2').checked = d.v2; 
}); }
window.onload=load; setInterval(load,4000);
</script></body></html>
)=====";

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  pinMode(RELAY_BOMBA, OUTPUT);
  pinMode(RELAY_OXIGENADOR, OUTPUT);

  WiFiManager wm;
  wm.autoConnect("Heltec_Hidroponia_AP");

  server.on("/", [](){ server.send(200, "text/html", htmlPage); });
  server.on("/status", [](){
    String json = "{\"autoOff\":" + String(desligaAutomacao?"true":"false") + ",\"v1\":" + String(v1?"true":"false") + ",\"v2\":" + String(v2?"true":"false") + "}";
    server.send(200, "application/json", json);
  });
  server.on("/set", [](){
    if(server.hasArg("v1")){ v1=(server.arg("v1")=="true"); sendRelay(RELAY_BOMBA, v1); }
    if(server.hasArg("v2")){ v2=(server.arg("v2")=="true"); sendRelay(RELAY_OXIGENADOR, v2); }
    if(server.hasArg("autoOff")){ desligaAutomacao=(server.arg("autoOff")=="false"); }
    server.send(200, "text/plain", "OK");
  });

  server.begin();
  updateOLED();
}

void loop() {
  server.handleClient();
  if (!desligaAutomacao) {
    if (!v1 || !v2) {
      v1 = true; v2 = true;
      sendRelay(RELAY_BOMBA, true);
      sendRelay(RELAY_OXIGENADOR, true);
    }
  }
}
