#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <Wire.h>

// --- Hardware Heltec V2 ---
#define RELAY_BOMBA 2       
#define RELAY_OXIGENADOR 17 
#define OLED_RST 16
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, /* clock=*/ 15, /* data=*/ 4);

// --- Objetos ---
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- Variáveis de Memória ---
bool trava24h = true; 
bool v1 = true, v2 = true;
String mBroker, mUser, mPass, mTopic = "hidroponia_esp";

void saveSettings() {
  File f = LittleFS.open("/config.txt", "w");
  if(f){
    f.println(trava24h);
    f.println(mBroker); f.println(mUser); f.println(mPass); f.println(mTopic);
    f.close();
  }
}

void loadSettings() {
  if(LittleFS.exists("/config.txt")){
    File f = LittleFS.open("/config.txt", "r");
    if(f){
      trava24h = f.readStringUntil('\n').toInt();
      mBroker = f.readStringUntil('\n'); mBroker.trim();
      mUser   = f.readStringUntil('\n'); mUser.trim();
      mPass   = f.readStringUntil('\n'); mPass.trim();
      mTopic  = f.readStringUntil('\n'); mTopic.trim();
      f.close();
    }
  }
}

void updateOLED() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "PAINEL HIDROPONIA");
  u8g2.drawHLine(0, 13, 128);
  u8g2.setCursor(0, 32);
  u8g2.print("IP: "); u8g2.print(WiFi.localIP().toString());
  u8g2.setCursor(0, 48);
  u8g2.print("BOMBA: "); u8g2.print(v1 ? "ON" : "OFF");
  u8g2.setCursor(0, 62);
  u8g2.print("OXI:   "); u8g2.print(v2 ? "ON" : "OFF");
  u8g2.sendBuffer();
}

void sendRelay(int pin, bool state) {
  digitalWrite(pin, state ? LOW : HIGH); 
  updateOLED();
}

// --- HTML Principal ---
const char htmlPage[] PROGMEM = R"=====(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  body { background:#111; color:#e1e1e1; font-family:sans-serif; padding:15px; display:flex; flex-direction:column; align-items:center; }
  .container { width:100%; max-width:600px; } /* Aumentado para ser mais comprido na horizontal */
  .card { background:#1c1c1e; border-radius:15px; padding:25px; margin-bottom:15px; box-shadow:0 10px 25px rgba(0,0,0,0.5); }
  h2 { font-size:14px; color:#aeaeb2; text-transform:uppercase; margin:0 0 25px 0; letter-spacing:1px; }
  .row { display:flex; justify-content:space-between; align-items:center; margin-bottom:20px; }
  .label-group { display:flex; align-items:center; gap:15px; }
  .icon { width:28px; height:28px; fill:#007aff; }
  .switch { position:relative; width:54px; height:30px; }
  .switch input { opacity:0; width:0; height:0; }
  .slider { position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; background:#3e3e42; transition:.4s; border-radius:30px; }
  .slider:before { position:absolute; content:""; height:24px; width:24px; left:3px; bottom:3px; background:#fff; transition:.4s; border-radius:50%; }
  input:checked + .slider { background:#34c759; }
  input:checked + .slider:before { transform:translateX(24px); }
  .btn-set { display:block; text-align:center; padding:18px; background:#2c2c2e; color:#007aff; text-decoration:none; border-radius:12px; font-weight:bold; border:1px solid #3e3e42; font-size:15px; transition:0.2s; }
  .btn-set:active { opacity:0.7; }
</style></head>
<body>
<div class="container">
  <div class="card">
    <h2>Controle de Fluxo</h2>
    
    <div class="row">
      <div class="label-group">
        <svg class="icon" viewBox="0 0 24 24"><path d="M12,2A10,10 0 1,0 22,12A10,10 0 0,0 12,2M12,20A8,8 0 1,1 20,12A8,8 0 0,1 12,20M11,11V6H13V11H18V13H13V18H11V13H6V11H11Z"/></svg>
        <span>Modo Sempre Ligado</span>
      </div>
      <label class="switch"><input type="checkbox" id="t24" onchange="update('t24', this.checked)"><span class="slider"></span></label>
    </div>

    <div class="row">
      <div class="label-group">
        <svg class="icon" viewBox="0 0 24 24"><path d="M12,2L4.5,20.29L5.21,21L12,18L18.79,21L19.5,20.29L12,2Z"/></svg>
        <span>Bomba d'Água</span>
      </div>
      <label class="switch"><input type="checkbox" id="v1" onchange="update('v1', this.checked)"><span class="slider"></span></label>
    </div>

    <div class="row">
      <div class="label-group">
        <svg class="icon" viewBox="0 0 24 24"><path d="M12,14A4,4 0 1,1 8,10A4,4 0 0,1 12,14M17,10A3,3 0 1,1 14,7A3,3 0 0,1 17,10M12,6A2,2 0 1,1 10,4A2,2 0 0,1 12,6Z"/></svg>
        <span>Oxigenador (Ar)</span>
      </div>
      <label class="switch"><input type="checkbox" id="v2" onchange="update('v2', this.checked)"><span class="slider"></span></label>
    </div>

    <a href="/settings" class="btn-set">⚙️ CONFIGURAÇÕES MQTT</a>
  </div>
</div>
<script>
function update(k,v){ fetch(`/set?${k}=${v}`); }
function load(){ fetch('/status').then(r=>r.json()).then(d=>{ 
  document.getElementById('t24').checked=d.t24; 
  document.getElementById('v1').checked=d.v1; 
  document.getElementById('v2').checked=d.v2; 
}); }
window.onload=load; setInterval(load,4000);
</script></body></html>
)=====";

// --- Configurações Simétricas e Compridas ---
void handleSettingsPage() {
  String h = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><style>";
  h += "body{background:#111;color:#fff;font-family:sans-serif;padding:20px;display:flex;flex-direction:column;align-items:center;}";
  h += ".container{width:100%; max-width:600px;}"; // Simetria horizontal com o painel principal
  h += "h2{font-size:18px;color:#aeaeb2;text-transform:uppercase;margin-bottom:25px;text-align:center;}";
  h += "label{font-size:14px;color:#aeaeb2;display:block;margin-bottom:8px;margin-left:5px;}";
  h += "input{width:100%;padding:16px;margin-bottom:25px;background:#2c2c2e;border:1px solid #3e3e42;color:#fff;border-radius:12px;font-size:16px;box-sizing:border-box;}";
  h += ".btn{width:100%;padding:18px;border:none;border-radius:12px;font-weight:bold;font-size:16px;cursor:pointer;text-align:center;display:block;box-sizing:border-box;margin-top:12px;text-decoration:none;}";
  h += ".save{background:#007aff;color:#fff;} .reset{background:#cf6679;color:#fff;margin-top:25px;}";
  h += ".back{color:#aeaeb2;display:block;text-align:center;margin-top:30px;text-decoration:none;font-size:14px;border:1px solid #3e3e42;padding:10px;border-radius:8px;}";
  h += "</style></head><body><div class='container'>";
  h += "<h2>Ajustes MQTT</h2><form action='/save_mqtt' method='POST'>";
  h += "<label>Broker IP:</label><input name='b' value='"+mBroker+"' placeholder='Ex: 192.168.1.50'>";
  h += "<label>Usuário:</label><input name='u' value='"+mUser+"'>";
  h += "<label>Senha:</label><input type='password' name='p' value='"+mPass+"'>";
  h += "<label>Tópico Base:</label><input name='t' value='"+mTopic+"'>";
  h += "<button type='submit' class='btn save'>SALVAR E REINICIAR</button></form>";
  h += "<a href='/full_reset' class='btn reset' onclick=\"return confirm('Resetar tudo?')\">FACTORY RESET</a>";
  h += "<a href='/' class='back'>Voltar ao Painel</a>";
  h += "</div></body></html>";
  server.send(200, "text/html", h);
}

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  if(!LittleFS.begin(true)) Serial.println("Erro LittleFS");
  loadSettings();
  pinMode(RELAY_BOMBA, OUTPUT);
  pinMode(RELAY_OXIGENADOR, OUTPUT);
  if(trava24h) { v1 = true; v2 = true; }
  sendRelay(RELAY_BOMBA, v1);
  sendRelay(RELAY_OXIGENADOR, v2);
  WiFiManager wm;
  wm.autoConnect("ESP_Hidroponia");
  MDNS.begin("hidroponia");
  server.on("/", [](){ server.send(200, "text/html", htmlPage); });
  server.on("/settings", handleSettingsPage);
  server.on("/status", [](){
    String json = "{ \"t24\":" + String(trava24h?"true":"false") + ",\"v1\":" + String(v1?"true":"false") + ",\"v2\":" + String(v2?"true":"false") + "}";
    server.send(200, "application/json", json);
  });
  server.on("/set", [](){
    if(server.hasArg("t24")){ trava24h=(server.arg("t24")=="true"); saveSettings(); }
    if(server.hasArg("v1")){ v1=(server.arg("v1")=="true"); sendRelay(RELAY_BOMBA, v1); }
    if(server.hasArg("v2")){ v2=(server.arg("v2")=="true"); sendRelay(RELAY_OXIGENADOR, v2); }
    server.send(200, "text/plain", "OK");
  });
  server.on("/save_mqtt", HTTP_POST, [](){
    mBroker = server.arg("b"); mUser = server.arg("u"); mPass = server.arg("p"); mTopic = server.arg("t");
    saveSettings();
    server.send(200, "text/html", "Salvo! Reiniciando...");
    delay(2000); ESP.restart();
  });
  server.on("/full_reset", [](){
    WiFiManager wm; wm.resetSettings(); LittleFS.format(); ESP.restart();
  });
  server.begin();
  updateOLED();
}

void loop() {
  server.handleClient();
  if (trava24h && (!v1 || !v2)) {
    v1 = true; v2 = true;
    sendRelay(RELAY_BOMBA, true);
    sendRelay(RELAY_OXIGENADOR, true);
  }
  static unsigned long lastOled = 0;
  if (millis() - lastOled > 15000) { lastOled = millis(); updateOLED(); }
}