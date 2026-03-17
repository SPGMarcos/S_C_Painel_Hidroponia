#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

// --- Hardware Heltec V2 ---
#define OLED_RST 16
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, 15, 4);
#define RELAY_BOMBA 2       
#define RELAY_OXIGENADOR 17 

// --- OBJETOS GLOBAIS ---
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- Variáveis de Controle ---
bool trava24h = true; 
bool v1 = true, v2 = true;
int tOnMin = 10, tOffMin = 10; 
unsigned long ultimoTempoBomba = 0;
unsigned long lastDisplayUpdate = 0;
String mBroker, mUser, mPass, mTopic = "hidroponia/controle";

// --- Persistência (LittleFS) ---
void saveSettings() {
    File f = LittleFS.open("/config.txt", "w");
    if (f) {
        f.println(trava24h); 
        f.println(tOnMin); 
        f.println(tOffMin);
        f.println(mBroker); 
        f.println(mUser); 
        f.println(mPass); 
        f.println(mTopic);
        f.close();
        Serial.println("Configuracoes salvas!");
    }
}

void loadSettings() {
    if (LittleFS.exists("/config.txt")) {
        File f = LittleFS.open("/config.txt", "r");
        if (f) {
            trava24h = f.readStringUntil('\n').toInt();
            tOnMin = f.readStringUntil('\n').toInt();
            tOffMin = f.readStringUntil('\n').toInt();
            mBroker = f.readStringUntil('\n'); mBroker.trim();
            mUser = f.readStringUntil('\n'); mUser.trim();
            mPass = f.readStringUntil('\n'); mPass.trim();
            mTopic = f.readStringUntil('\n'); mTopic.trim();
            if (tOnMin <= 0) tOnMin = 10; 
            if (tOffMin <= 0) tOffMin = 10;
            f.close();
        }
    }
}

// --- Display OLED ---
void updateDisplay() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 8);
    u8g2.print("IP: "); u8g2.print(WiFi.localIP().toString());
    u8g2.setCursor(0, 18);
    u8g2.print("smarthidroponia.local"); 
    u8g2.drawHLine(0, 21, 128);
    
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.setCursor(0, 36);
    u8g2.print(v1 ? "ON  " : "OFF "); u8g2.drawGlyph(40, 36, 0x2614);
    u8g2.setFont(u8g2_font_6x10_tf); u8g2.print(" BOMBA");
    
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.setCursor(0, 50);
    u8g2.print(v2 ? "ON  " : "OFF "); u8g2.drawGlyph(40, 50, 0x2605);
    u8g2.setFont(u8g2_font_6x10_tf); u8g2.print(" OXIGEN");
    
    if (trava24h) {
        long restante = ((v1 ? tOnMin : tOffMin) * 60) - ((millis() - ultimoTempoBomba) / 1000);
        if (restante < 0) restante = 0;
        u8g2.drawFrame(0, 53, 128, 11);
        u8g2.setCursor(35, 62);
        u8g2.printf("%02d:%02d", (int)(restante / 60), (int)(restante % 60));
    } else { 
        u8g2.setCursor(35, 62); 
        u8g2.print("MANUAL"); 
    }
    u8g2.sendBuffer();
}

// --- Dashboard HTML Principal ---
const char htmlPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <style>
        body { background:#0a0a0a; color:#e1e1e1; font-family:sans-serif; padding:15px; display:flex; flex-direction:column; align-items:center; }
        .container { width:100%; max-width:600px; }
        .logo { text-align:center; margin-top: 20px;}
        .logo svg { width:55px; height:55px; fill:#007aff; }
        .title { text-align:center; font-weight:bold; font-size:20px; margin-bottom:30px; color:#fff; letter-spacing: 2px;}
        .card { background:#1c1c1e; border-radius:15px; padding:25px; margin-bottom:15px; box-shadow:0 10px 25px rgba(0,0,0,0.5); }
        h2 { font-size:11px; color:#555; text-transform:uppercase; margin:0 0 20px 0; letter-spacing:1.5px; }
        .row { display:flex; justify-content:space-between; align-items:center; margin-bottom:20px; }
        .label-group { display:flex; align-items:center; gap:15px; }
        .icon { width:24px; height:24px; fill:#007aff; }
        .switch { position:relative; width:54px; height:30px; }
        .switch input { opacity:0; width:0; height:0; }
        .slider { position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; background:#3e3e42; transition:.4s; border-radius:30px; }
        .slider:before { position:absolute; content:""; height:24px; width:24px; left:3px; bottom:3px; background:#fff; transition:.4s; border-radius:50%; }
        input:checked + .slider { background:#34c759; }
        input:checked + .slider:before { transform:translateX(24px); }
        .adjust-container { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 20px; }
        .adjust-box { background:#2c2c2e; padding:12px; border-radius:12px; border: 1px solid #3e3e42; }
        .adjust-box label { display:block; font-size:10px; color:#aeaeb2; margin-bottom:8px; text-transform:uppercase; }
        .input-group input { background:#1c1c1e; border:1px solid #007aff; color:#fff; padding:12px 5px; border-radius:8px; width:100%; text-align:center; font-size:18px; box-sizing: border-box; outline:none;}
        .timer-box { background:#007aff15; padding:15px; border-radius:12px; text-align:center; margin-top:15px; border: 1px solid #007aff33; }
        .timer-val { font-size:26px; color:#fff; font-weight:bold; font-family:monospace; }
        .config-link { text-align:center; color:#555; font-size:13px; margin-top:25px; cursor:pointer; text-decoration:none; display:block;}
    </style>
</head>
<body>
    <div class="container">
        <div class="logo"><svg viewBox="0 0 24 24"><path d="M12,3L2,12H5V20H11V14H13V20H19V12H22L12,3Z"/></svg></div>
        <div class="title">SMARTCONTROL</div>
        <div class="card">
            <h2>CONTROLE DE FLUXO</h2>
            <div class="row">
                <div class="label-group"><svg class="icon" viewBox="0 0 24 24"><path d="M12,2A10,10 0 1,0 22,12A10,10 0 0,0 12,2M12,20A8,8 0 1,1 20,12A8,8 0 0,1 12,20M11,11V6H13V11H18V13H13V18H11V13H6V11H11Z"/></svg><span>Modo Automático</span></div>
                <label class="switch"><input type="checkbox" id="t24" onchange="update('t24', this.checked)"><span class="slider"></span></label>
            </div>
            <div id="autoControls" style="display:none;">
                <div class="adjust-container">
                    <div class="adjust-box"><label>Tempo ON (Min)</label><div class="input-group"><input type="number" inputmode="numeric" id="tOn" onblur="update('tOn', this.value)"></div></div>
                    <div class="adjust-box"><label>Tempo OFF (Min)</label><div class="input-group"><input type="number" inputmode="numeric" id="tOff" onblur="update('tOff', this.value)"></div></div>
                </div>
                <div class="timer-box"><div id="statusLabel" style="font-size:11px; margin-bottom:5px; text-transform:uppercase;">...</div><div id="clock" class="timer-val">00:00</div></div>
            </div>
            <div style="margin-top:20px; border-top: 1px solid #2c2c2e; padding-top: 20px;">
                <div class="row">
                    <div class="label-group"><svg class="icon" viewBox="0 0 24 24"><path d="M12,2L4.5,20.29L5.21,21L12,18L18.79,21L19.5,20.29L12,2Z"/></svg><span>Bomba d'Água</span></div>
                    <label class="switch"><input type="checkbox" id="v1" onchange="update('v1', this.checked)"><span class="slider"></span></label>
                </div>
                <div class="row">
                    <div class="label-group"><svg class="icon" viewBox="0 0 24 24"><path d="M12,14A4,4 0 1,1 8,10A4,4 0 0,1 12,14M17,10A3,3 0 1,1 14,7A3,3 0 0,1 17,10M12,6A2,2 0 1,1 10,4A2,2 0 0,1 12,6Z"/></svg><span>Oxigenador (24h)</span></div>
                    <label class="switch"><input type="checkbox" id="v2" onchange="update('v2', this.checked)"><span class="slider"></span></label>
                </div>
            </div>
            <a href="/settings" class="config-link">⚙️ CONFIGURAÇÕES MQTT</a>
        </div>
    </div>
    <script>
        function update(k,v){ fetch(`/set?${k}=${v}`); }
        function load(){ 
            fetch('/status').then(r=>r.json()).then(d=>{ 
                document.getElementById('t24').checked=d.t24; 
                document.getElementById('v1').checked=d.v1; 
                document.getElementById('v2').checked=d.v2;
                if(document.activeElement.id !== 'tOn') document.getElementById('tOn').value = d.tOn;
                if(document.activeElement.id !== 'tOff') document.getElementById('tOff').value = d.tOff;
                if(d.t24){
                    document.getElementById('autoControls').style.display = 'block';
                    let min = Math.floor(d.rem / 60); let sec = d.rem % 60;
                    document.getElementById('clock').innerText = (min<10?'0':'')+min + ':' + (sec<10?'0':'')+sec;
                    document.getElementById('statusLabel').innerText = d.v1 ? "BOMBA LIGADA (ON)" : "BOMBA EM REPOUSO (OFF)";
                    document.getElementById('statusLabel').style.color = d.v1 ? "#34c759" : "#ff9500";
                } else { 
                    document.getElementById('autoControls').style.display = 'none'; 
                }
            }); 
        }
        window.onload=load; 
        setInterval(load,1000);
    </script>
</body>
</html>
)=====";

void setup() {
    Serial.begin(115200);
    u8g2.begin();
    LittleFS.begin(true);
    loadSettings();
    
    pinMode(RELAY_BOMBA, OUTPUT); 
    pinMode(RELAY_OXIGENADOR, OUTPUT);
    digitalWrite(RELAY_BOMBA, v1 ? LOW : HIGH); 
    digitalWrite(RELAY_OXIGENADOR, v2 ? LOW : HIGH);
    
    WiFiManager wm; 
    wm.autoConnect("ESP_Hidroponia");

    // --- mDNS (URL Local) ---
    if (MDNS.begin("smarthidroponia")) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS: http://smarthidroponia.local");
    }
    
    // --- OTA ---
    ArduinoOTA.setHostname("SmartControl-Hidroponia");
    ArduinoOTA.onStart([]() { 
        u8g2.clearBuffer(); 
        u8g2.print("Atualizando..."); 
        u8g2.sendBuffer(); 
    });
    ArduinoOTA.begin();

    // --- Rotas do Servidor ---
    server.on("/", [](){ 
        server.send(200, "text/html", htmlPage); 
    });
    
    server.on("/status", [](){
        long restante = ((v1 ? tOnMin : tOffMin) * 60) - ((millis() - ultimoTempoBomba) / 1000);
        if (restante < 0) restante = 0;
        String json = "{\"t24\":"+String(trava24h?"true":"false")+",\"v1\":"+String(v1?"true":"false")+",\"v2\":"+String(v2?"true":"false")+",\"rem\":"+String(restante)+",\"tOn\":"+String(tOnMin)+",\"tOff\":"+String(tOffMin)+"}";
        server.send(200, "application/json", json);
    });

    server.on("/set", [](){
        if (server.hasArg("t24")) { trava24h = (server.arg("t24") == "true"); ultimoTempoBomba = millis(); }
        if (server.hasArg("tOn")) { tOnMin = server.arg("tOn").toInt(); if(trava24h) ultimoTempoBomba = millis(); }
        if (server.hasArg("tOff")) { tOffMin = server.arg("tOff").toInt(); if(trava24h) ultimoTempoBomba = millis(); }
        if (server.hasArg("v1")) { v1 = (server.arg("v1") == "true"); digitalWrite(RELAY_BOMBA, v1 ? LOW : HIGH); }
        if (server.hasArg("v2")) { v2 = (server.arg("v2") == "true"); digitalWrite(RELAY_OXIGENADOR, v2 ? LOW : HIGH); }
        saveSettings(); 
        server.send(200, "text/plain", "OK");
    });

    server.on("/settings", [](){
        String h = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{background:#0a0a0a;color:#fff;font-family:sans-serif;padding:20px;display:flex;flex-direction:column;align-items:center;}input{width:100%;padding:16px;margin-bottom:20px;background:#1c1c1e;border:1px solid #3e3e42;color:#fff;border-radius:12px;font-size:16px;box-sizing:border-box;}.btn{width:100%;padding:18px;background:#007aff;color:#fff;border:none;border-radius:12px;font-weight:bold;}.back{margin-top:20px;color:#555;text-decoration:none;}</style></head><body>";
        h += "<h2>Ajustes MQTT</h2><form action='/save_mqtt' method='POST'>";
        h += "<label>Broker IP:</label><input name='b' value='" + mBroker + "'>";
        h += "<label>Usuário:</label><input name='u' value='" + mUser + "'>";
        h += "<label>Senha:</label><input type='password' name='p' value='" + mPass + "'>";
        h += "<label>Tópico:</label><input name='t' value='" + mTopic + "'>";
        h += "<button type='submit' class='btn'>SALVAR E REINICIAR</button></form>";
        h += "<a href='/' class='back'>Voltar ao Painel</a></body></html>";
        server.send(200, "text/html", h);
    });

    server.on("/save_mqtt", HTTP_POST, [](){
        mBroker = server.arg("b"); 
        mUser = server.arg("u"); 
        mPass = server.arg("p"); 
        mTopic = server.arg("t");
        saveSettings(); 
        server.send(200, "text/plain", "Configuracoes Salvas! Reiniciando..."); 
        delay(2000); 
        ESP.restart();
    });

    server.begin();
}

void loop() {
    ArduinoOTA.handle();
    server.handleClient();
    
    if (millis() - lastDisplayUpdate > 500) { 
        lastDisplayUpdate = millis(); 
        updateDisplay(); 
    }
    
    if (trava24h) {
        unsigned long intervalo = (v1 ? tOnMin : tOffMin) * 60000UL;
        if (millis() - ultimoTempoBomba >= intervalo) {
            ultimoTempoBomba = millis(); 
            v1 = !v1; 
            digitalWrite(RELAY_BOMBA, v1 ? LOW : HIGH);
        }
    }
}