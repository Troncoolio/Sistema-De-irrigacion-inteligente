#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "iPhone de Anthony";
const char* password = "123456789";

#define PIN_SENSOR 34
#define PIN_BOMBA  25

WebServer server(80);

int valorSeco   = 3500;
int valorHumedo = 1200;

int umbral = 60; // porcentaje
bool bombaActiva = false;
bool modoAuto = true;

int leerHumedadRaw() {
  long suma = 0;
  for (int i = 0; i < 15; i++) {
    suma += analogRead(PIN_SENSOR);
    delay(2);
  }
  return suma / 15;
}

int humedadPorcentaje(int raw) {
  int porcentaje = map(raw, valorSeco, valorHumedo, 0, 100);

  if (porcentaje > 100) porcentaje = 100;
  if (porcentaje < 0) porcentaje = 0;

  return porcentaje;
}

void controlAutomatico(int humedad) {
  if (!modoAuto) return;

  if (humedad < umbral && !bombaActiva) {
    digitalWrite(PIN_BOMBA, HIGH);
    bombaActiva = true;
  }

  if (humedad >= umbral && bombaActiva) {
    digitalWrite(PIN_BOMBA, LOW);
    bombaActiva = false;
  }
}

String paginaHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Riego Inteligente</title>
  <link href="https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Barlow:wght@400;600;700&display=swap" rel="stylesheet">
<link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>💧</text></svg>">  <style>
    :root {
      --bg: #0d1117;
      --surface: #161b22;
      --border: #21262d;
      --accent: #39d353;
      --accent-dim: #1a4a25;
      --warn: #f0883e;
      --warn-dim: #3d2410;
      --blue: #58a6ff;
      --blue-dim: #0d2a4a;
      --text: #e6edf3;
      --muted: #8b949e;
      --mono: 'Share Tech Mono', monospace;
      --sans: 'Barlow', sans-serif;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      font-family: var(--sans);
      background-color: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 24px;
      background-image:
        radial-gradient(ellipse at 20% 50%, rgba(57,211,83,0.04) 0%, transparent 60%),
        radial-gradient(ellipse at 80% 20%, rgba(88,166,255,0.04) 0%, transparent 50%);
    }

    .card {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 16px;
      padding: 32px;
      width: 100%;
      max-width: 420px;
      box-shadow: 0 0 0 1px rgba(255,255,255,0.03), 0 24px 48px rgba(0,0,0,0.5);
      animation: fadeUp 0.5s ease both;
    }

    @keyframes fadeUp {
      from { opacity: 0; transform: translateY(16px); }
      to   { opacity: 1; transform: translateY(0); }
    }

    .header {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-bottom: 28px;
      padding-bottom: 20px;
      border-bottom: 1px solid var(--border);
    }
    .header-icon {
      width: 40px; height: 40px;
      background: var(--accent-dim);
      border: 1px solid var(--accent);
      border-radius: 10px;
      display: grid; place-items: center;
      font-size: 20px;
      box-shadow: 0 0 12px rgba(57,211,83,0.2);
    }
    .header h1 {
      font-size: 18px;
      font-weight: 700;
      letter-spacing: 0.02em;
    }
    .header p {
      font-size: 12px;
      color: var(--muted);
      font-family: var(--mono);
    }

    .status-grid {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      gap: 10px;
      margin-bottom: 24px;
    }
    .stat-box {
      background: var(--bg);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 14px 10px;
      text-align: center;
    }
    .stat-label {
      font-size: 10px;
      text-transform: uppercase;
      letter-spacing: 0.08em;
      color: var(--muted);
      margin-bottom: 6px;
    }
    .stat-value {
      font-family: var(--mono);
      font-size: 22px;
      color: var(--text);
      line-height: 1;
    }
    .stat-value.on  { color: var(--accent); text-shadow: 0 0 10px rgba(57,211,83,0.4); }
    .stat-value.off { color: var(--muted); }
    .stat-value.auto { color: var(--blue); }

    .humidity-bar-wrap {
      margin: -4px 0 20px;
      background: var(--bg);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 14px 16px;
    }
    .bar-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 8px;
    }
    .bar-label { font-size: 11px; color: var(--muted); text-transform: uppercase; letter-spacing: 0.07em; }
    .bar-val   { font-family: var(--mono); font-size: 14px; color: var(--accent); }
    .bar-track {
      height: 6px;
      background: var(--border);
      border-radius: 99px;
      overflow: hidden;
    }
    .bar-fill {
      height: 100%;
      width: 0%;
      background: linear-gradient(90deg, var(--accent-dim), var(--accent));
      border-radius: 99px;
      transition: width 0.8s cubic-bezier(.4,0,.2,1);
      box-shadow: 0 0 8px rgba(57,211,83,0.5);
    }

    .controls { display: flex; flex-direction: column; gap: 10px; }

    .umbral-row {
      display: flex;
      gap: 8px;
    }
    .umbral-row input {
      flex: 1;
      background: var(--bg);
      border: 1px solid var(--border);
      border-radius: 8px;
      color: var(--text);
      font-family: var(--mono);
      font-size: 14px;
      padding: 0 14px;
      height: 44px;
      outline: none;
      transition: border-color 0.2s;
    }
    .umbral-row input::placeholder { color: var(--muted); }
    .umbral-row input:focus { border-color: var(--accent); box-shadow: 0 0 0 3px rgba(57,211,83,0.1); }

    .btn-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }

    button {
      height: 44px;
      border: 1px solid transparent;
      border-radius: 8px;
      font-family: var(--sans);
      font-size: 13px;
      font-weight: 600;
      letter-spacing: 0.04em;
      cursor: pointer;
      transition: all 0.15s ease;
      display: flex; align-items: center; justify-content: center; gap: 6px;
    }
    button:active { transform: scale(0.97); }

    .btn-green {
      background: var(--accent-dim);
      border-color: var(--accent);
      color: var(--accent);
    }
    .btn-green:hover { background: rgba(57,211,83,0.2); box-shadow: 0 0 12px rgba(57,211,83,0.2); }

    .btn-orange {
      background: var(--warn-dim);
      border-color: var(--warn);
      color: var(--warn);
    }
    .btn-orange:hover { background: rgba(240,136,62,0.2); box-shadow: 0 0 12px rgba(240,136,62,0.2); }

    .btn-blue {
      background: var(--blue-dim);
      border-color: var(--blue);
      color: var(--blue);
    }
    .btn-blue:hover { background: rgba(88,166,255,0.2); box-shadow: 0 0 12px rgba(88,166,255,0.2); }

    .btn-full { grid-column: span 2; }

    .footer {
      margin-top: 20px;
      padding-top: 16px;
      border-top: 1px solid var(--border);
      display: flex;
      align-items: center;
      justify-content: space-between;
    }
    .pulse-dot {
      width: 8px; height: 8px; border-radius: 50%;
      background: var(--accent);
      box-shadow: 0 0 6px var(--accent);
      animation: pulse 2s ease infinite;
    }
    @keyframes pulse {
      0%,100% { opacity: 1; transform: scale(1); }
      50%      { opacity: 0.4; transform: scale(0.8); }
    }
    .footer-text { font-size: 11px; color: var(--muted); font-family: var(--mono); }
    #last-update { font-size: 11px; color: var(--muted); font-family: var(--mono); }
  </style>
</head>
<body>
  <div class="card">

    <div class="header">
      <div class="header-icon"><svg width="20" height="20" viewBox="0 0 24 24" fill="#39d353">
  <path d="M12 2C6 10 4 14 4 17a8 8 0 0016 0c0-3-2-7-8-15z"/>
</svg></div>
      <div>
        <h1>Sistema de Riego</h1>
        <p>Control de irrigacion inteligente</p>
      </div>
    </div>

    <div class="status-grid">
      <div class="stat-box">
        <div class="stat-label">Bomba</div>
        <div class="stat-value off" id="estado">--</div>
      </div>
      <div class="stat-box">
        <div class="stat-label">Modo</div>
        <div class="stat-value" id="modo">--</div>
      </div>
      <div class="stat-box">
        <div class="stat-label">Umbral</div>
        <div class="stat-value" id="umbral-display" style="font-size:18px">--</div>
      </div>
    </div>

    <div class="humidity-bar-wrap">
      <div class="bar-header">
        <span class="bar-label">Humedad del suelo</span>
        <span class="bar-val" id="humedad">--%</span>
      </div>
      <div class="bar-track">
        <div class="bar-fill" id="bar"></div>
      </div>
    </div>

    <div class="controls">
      <div class="umbral-row">
        <input type="number" id="umbral" placeholder="Nuevo umbral (0-100)">
        <button class="btn-green" onclick="cambiarUmbral()" style="width:140px;flex-shrink:0">
          Aplicar
        </button>
      </div>
      <div class="btn-row">
        <button class="btn-orange" onclick="toggleBomba()">Encender/Apagar</button>
        <button class="btn-blue"   onclick="toggleModo()">Auto / Manual</button>
      </div>
    </div>

    <div class="footer">
      <div style="display:flex;align-items:center;gap:8px">
        <div class="pulse-dot"></div>
        <span class="footer-text">En vivo</span>
      </div>
      <span id="last-update">--</span>
    </div>

  </div>

  <script>
    function actualizar() {
      fetch('/data')
        .then(r => r.json())
        .then(data => {
          const h = parseFloat(data.humedad) || 0;
          document.getElementById('humedad').innerText = h + '%';
          document.getElementById('bar').style.width = Math.min(h, 100) + '%';

          const estado = document.getElementById('estado');
          estado.innerText = data.bomba ? 'ON' : 'OFF';
          estado.className = 'stat-value ' + (data.bomba ? 'on' : 'off');

          const modo = document.getElementById('modo');
          modo.innerText = data.auto ? 'AUTO' : 'MANUAL';
          modo.className = 'stat-value ' + (data.auto ? 'auto' : '');

          if (data.umbral !== undefined)
            document.getElementById('umbral-display').innerText = data.umbral;

          const now = new Date();
          document.getElementById('last-update').innerText =
            now.toLocaleTimeString('es-MX', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        })
        .catch(() => {}); 
    }

    function toggleBomba() {
      fetch('/control?bomba=toggle').catch(()=>{});
    }
    function toggleModo() {
      fetch('/control?modo=toggle').catch(()=>{});
    }
    function cambiarUmbral() {
      const val = document.getElementById('umbral').value;
      if (!val) return;
      fetch('/control?umbral=' + val).catch(()=>{});
      document.getElementById('umbral-display').innerText = val;
      document.getElementById('umbral').value = '';
    }

    setInterval(actualizar, 3000);
    actualizar();
  </script>
</body>
</html>
)rawliteral";
}

void handleRoot() {
  server.send(200, "text/html", paginaHTML());
}

void handleData() {
  int raw = leerHumedadRaw();
  int humedad = humedadPorcentaje(raw);

  String json = "{";
  json += "\"humedad\":" + String(humedad) + ",";
  json += "\"bomba\":" + String(bombaActiva ? "true" : "false") + ",";
  json += "\"auto\":" + String(modoAuto ? "true" : "false") + ",";
  json += "\"umbral\":" + String(umbral);
  json += "}";

  server.send(200, "application/json", json);
}

void handleControl() {
  if (server.hasArg("bomba")) {
    bombaActiva = !bombaActiva;
    digitalWrite(PIN_BOMBA, bombaActiva);
  }

  if (server.hasArg("modo")) {
    modoAuto = !modoAuto;
  }

  if (server.hasArg("umbral")) {
    umbral = server.arg("umbral").toInt();
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BOMBA, OUTPUT);
  digitalWrite(PIN_BOMBA, LOW);

  analogReadResolution(12);

  WiFi.begin(ssid, password);
  Serial.print("Conectando...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/control", handleControl);

  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long lastRead = 0;

  if (millis() - lastRead > 1000) { 
    lastRead = millis();

    int raw = leerHumedadRaw();
    int humedad = humedadPorcentaje(raw);

    controlAutomatico(humedad);

    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.print("% | Bomba: ");
    Serial.println(bombaActiva ? "ON" : "OFF");

  }
}