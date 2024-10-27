#include <ArduinoJson.h>  // 統一傳送資料格式用（Json）
#include <HTTPClient.h>   // 抓取 Yahoo 股市 API 用
#include <SSD1306Wire.h>  // 控制 OLED 用
#include <WebServer.h>    // 架前後端網站用
#include <WiFi.h>

#define OLED_WIDTH 128    // OLED 寬度
#define OLED_HEIGHT 64    // OLED 長度
#define SDA_PIN 3         // OLED SDA 腳位
#define SCL_PIN 2         // OLED SCL 腳位
#define BUTTON_PIN 10     // 按鈕腳位
#define R_PIN 0           // 量電池腳位
#define BATT_AVG_SIZE 10  // 要取多少電量的平均
#define TICK 5000         // 每多少刻抓一次眼動儀資料

// 你的手機熱點
#define WIFI_SSID "韓劇大長今"
#define WIFI_PASSWORD "90082201"

// 你的後端資訊
#define BACKEND_IP "192.168.137.145"
#define BACKEND_PORT "5000"

WebServer server(80);  // Server 物件宣告
SSD1306Wire oled(0x3c, SDA_PIN, SCL_PIN);  // OLED 物件宣告

bool nowBtn = false, preBtn = false;  // 記錄按鈕狀態（此輪與上一輪）
uint16_t batt[BATT_AVG_SIZE] = {}, batt_sum = 0;  // 存電量取平均
uint8_t batt_pt = 0;  // 記錄這次電量要存第幾格
uint8_t tick = 0;  // 記錄目前幀數
enum Status{NORMAL, STUDY, GAZING, NERVOUS} now_status = NORMAL;  // 記錄目前使用者模式

// HTML 網頁代碼
String index_html = R"rawliteral(
<!DOCTYPE html>
<html lang='zh-TW'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>學習專注力追蹤</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #F7F8FA;
            color: #333;
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        .container {
            max-width: 800px;
            width: 100%;
            padding: 20px;
        }
        .stats-card {
            background-color: #fff;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.1);
        }
        .stats-card h3 {
            margin: 0 0 10px 0;
            font-size: 1.2em;
        }
        .chart-container {
            background-color: #FFF;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.1);
        }
    </style>
</head>
<body>
    <div class='container'>
        <div class='stats-card'>
            <h3>Study Time：<t id='studyTime'>Loading...</t></h3>
        </div>
        <div class='stats-card'>
            <h3>Max Focus Duration：<t id='maxFocusDuration'>Loading...</t></h3>
        </div>
        <div class='stats-card'>
            <h3>Average Distraction Time：<t id='averageDistractionInterval'>Loading...</t></h3>
        </div>
        <div class='chart-container'>
            <h3>Distraction Chart</h3>
            <canvas id='distractionChart'></canvas>
        </div>
    </div>

    <script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
    <script>
        const backend_ip = '192.168.137.145';
        const backend_port = '5000';

        const startTime = new Date();

        function sec2time(sec){
            const hours = String(Math.floor(sec / 3600)).padStart(2, '0');
            const minutes = String(Math.floor((sec % 3600) / 60)).padStart(2, '0');
            const seconds = String(sec % 60).padStart(2, '0');

            return `${hours}:${minutes}:${seconds}`;
        }

        async function fetchFocusData() {
            try {
                const response = await fetch(`http://${backend_ip}:${backend_port}/`);
                if (response.ok) {
                    const data = await response.json();
                    return data;
                } else {
                    console.error('HTTP Error:', response.status, response.statusText);
                    return -1;
                }
            } catch (error) {
                console.error('Fetch Error:', error);
                return -1;
            }
        }

        async function updateStats() {
            const data = await fetchFocusData();

            console.log(data);
            if(data == -1)
                return;

            document.getElementById('studyTime').textContent = (data.studyStartTime != -1 ? sec2time(data.now - data.studyStartTime) : 'not studying');
            document.getElementById('maxFocusDuration').textContent = (data.studyStartTime != -1 ? sec2time(Math.max(data.maxStudyDuration, (data.studyTime != -1 ? data.now - data.studyTime : 0))) : 'not studying');
            document.getElementById('averageDistractionInterval').textContent = (data.studyStartTime != -1 ? sec2time(Math.round(data.distractionsInterval / ((data.now - data.studyStartTime) / 600))) + ' per 10 min' : 'not studying');

            if(data.distractionsTimePerTenMin.length > 0)
                updateDistractionChart(data.distractionsTimePerTenMin);
        }

        function updateDistractionChart(data) {
            const ctx = document.getElementById('distractionChart').getContext('2d');
            new Chart(ctx, {
                type: 'line',
                data: {
                    labels: Array.from({ length: data.length }, (_, i) => `${(i + 1) * 10} min`),
                    datasets: [{
                        label: 'Distraction Time (sec)',
                        data: data,
                        backgroundColor: 'rgba(93, 173, 226, 0.2)',
                        borderColor: '#5DADE2',
                        borderWidth: 2,
                        fill: true
                    }]
                },
                options: {
                    responsive: true,
                    scales: {
                        y: {
                            beginAtZero: true,
                            ticks: { stepSize: 1 }
                        }
                    }
                }
            });
        }

        setInterval(updateStats, 1000);
        updateStats();
    </script>
</body>
</html>
)rawliteral";

// 先宣告函式
void handleRoot(); // 架網頁
void gazing(); // 取得目前視線資訊
void OLEDprint(String); // 印出文字
void wifiConnect(); // 等待連接手機熱點
void checkBattery(uint16_t);  // 檢查電量
String toString(double, bool, uint8_t); //將浮點數轉成指定格式的字串

/******************************************/
/*             MAIN FUNCTIONS             */
/******************************************/

void setup() {
  // 設定輸入腳位
  pinMode(BUTTON_PIN, INPUT);
  pinMode(R_PIN, INPUT);

  // OLED 初始化設定
  oled.init();
  oled.flipScreenVertically();
  oled.mirrorScreen();
  oled.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  oled.setFont(ArialMT_Plain_16);
  oled.setContrast(255);
  oled.clear();
  OLEDprint("Starting");
  delay(1000);  // 休息1秒

  // 存取電量初始化設定：先填滿每一格
  for(uint8_t i = 0; i < BATT_AVG_SIZE; i++){
    batt[i] = analogRead(R_PIN);
    batt_sum += batt[i];
  }
  checkBattery(2500); // 檢查電量

  // 初始化、連接 Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifiConnect();

  // 準備網頁路徑，啟動 Server
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();  // 處理網頁請求

  if(tick == 0)  // 每TICK刻執行一次
    gazing();
  
  nowBtn = digitalRead(BUTTON_PIN); // 抓取目前按鈕狀態
  // 按鈕被點擊的話，就關閉螢幕
  if(!nowBtn && preBtn)
    OLEDprint("");
  preBtn = nowBtn;

  checkBattery(2600); // 檢查電量

  if(WiFi.status() != WL_CONNECTED){  // 檢查 Wi-Fi 連線狀態
    wifiConnect();
  }

  tick = (tick + 1) % TICK;
}

/*******************************************/
/*          FUNCTION DECLARATIONS          */
/*******************************************/

// 架網頁
void handleRoot() {
  server.send(200, "text/html", index_html);  // 把 HTML 架到前端
}

// 取得目前視線資訊
void gazing() {
  String url = "http://" + String(BACKEND_IP) + ":" + String(BACKEND_PORT) + "/";

  HTTPClient http;
  http.begin(url);

  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String payload = http.getString();

    DynamicJsonDocument jsonDoc(payload.length());
    DeserializationError error = deserializeJson(jsonDoc, payload);
    if (error) {  // string to json
      server.send(500, "application/json", "{\"error\":\"Error fetching data\"}");
      return;
    }

    if(now_status == STUDY && jsonDoc["studyStartTime"].as<int>() != -1){
      if(jsonDoc["distractingStartTime"].as<int>() != -1 && jsonDoc["now"].as<int>() - jsonDoc["distractingStartTime"].as<int>() <= 10){
        OLEDprint("Don't Be\nDistracted!");
      }else if(jsonDoc["now"].as<int>() - jsonDoc["studyStartTime"].as<int>() > 40){
        OLEDprint("");
      }
    }else if(now_status != STUDY && jsonDoc["studyStartTime"].as<int>() != -1){
      OLEDprint("Studying?\nGood luck!");
      now_status = STUDY;
    }else if(now_status != GAZING && jsonDoc["overTenSec"].as<bool>()){
      OLEDprint(jsonDoc["nowObject"].as<String>());
      now_status = GAZING;
    }else if(now_status != STUDY && now_status != NERVOUS && jsonDoc["overTwentySec"].as<bool>() && jsonDoc["nowEyetrack"].as<String>() == "saccade"){
      OLEDprint("Are you alright?");
      now_status = NERVOUS;
    }else if(now_status != NORMAL && !jsonDoc["overTenSec"].as<bool>()){
      OLEDprint("");
      now_status = NORMAL;
    }
  }

  http.end();
}

// 印出文字
void OLEDprint(String txt){
  oled.clear();
  oled.drawString(OLED_WIDTH / 2, OLED_HEIGHT / 2, txt);
  oled.display();
}

// 等待連接手機熱點
void wifiConnect(){
  byte rnd = 1;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    checkBattery(2500);
    
    switch(rnd){
      case 1:
        OLEDprint("Connecting.");
        rnd = 2;
        break;
      case 2:
        OLEDprint("Connecting..");
        rnd = 3;
        break;
      case 3:
        OLEDprint("Connecting...");
        rnd = 1;
        break;
    }
  }
  String ip = String(WiFi.localIP().toString().c_str());
  OLEDprint("Connected.\n" + ip);
}

// 檢查電量
void checkBattery(uint16_t initBattVal){
  uint16_t battVal = initBattVal;
  
  UPDATE_BATTERY:  // 更新電量平均
  batt_sum -= batt[batt_pt];
  batt[batt_pt] = analogRead(R_PIN);
  batt_sum += batt[batt_pt];
  batt_pt = (batt_pt + 1) % BATT_AVG_SIZE;
  
  if(double(batt_sum) / BATT_AVG_SIZE < battVal){  // 檢查數值
    OLEDprint("LOW BATTERY");
    battVal = initBattVal + 150;  // 要充夠電才行
    goto UPDATE_BATTERY;
  }else if(battVal == initBattVal + 100)  // 確認是否為充夠電後的狀態
    wifiConnect();
}

//將浮點數轉成指定格式的字串
String toString(double num, bool abbr, uint8_t precision){
  String units[] = {"", "K", "M", "B", "T"};
  uint8_t unit = 0;
  if(abbr){
    while(num / 1000 > 1 && unit < 5){
      num /= 1000;
      unit++;
    }
  }
  uint16_t ten = 1;
  for(int8_t i = 0; i < precision; i++)
    ten *= 10;
  num = int((num + 0.5 / ten) * ten) / double(ten);
  String rtn = String(num);
  while (rtn.endsWith("0"))
    rtn.remove(rtn.length() - 1);
  if (rtn.endsWith("."))
    rtn.remove(rtn.length() - 1);
  rtn += units[unit];
  return rtn;
}
