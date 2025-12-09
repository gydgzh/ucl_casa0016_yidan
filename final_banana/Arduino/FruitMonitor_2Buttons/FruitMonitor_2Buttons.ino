/*
 * Multi-Fruit Freshness Monitor - v3.1 双模式版
 * 
 * 两种模式：
 * 1. 环境监测模式（默认）- 评估环境能存多久
 * 2. 水果测试模式 - 评估单个水果能不能吃
 * 
 * 根据你的传感器数据调整的阈值版本
 */

#include <MKRWAN.h>
#include <DHT.h>
#include <Arduino_GFX_Library.h>

#include "secrets.h"
#include "fruit_profiles.h"
#include "sensors.h"
#include "freshness_model.h"
#include "ui_manager.h"

// ==================== 配置选项 ====================
#define TFT_TEST_MODE false  // TFT测试：true=测试，false=正常
#define TFT_DRIVER 1         // 1=ILI9488, 2=ILI9341, 3=ST7796

// ==================== 全局对象 ====================
LoRaModem modem;
Sensors sensors;
FreshnessModel freshnessModel;
UIManager ui;

// ==================== 按钮配置 ====================
#define BTN_SWITCH_FRUIT  0  // D0 - 黄色按钮
#define BTN_CONFIRM       1  // D1 - 绿色按钮

bool lastSwitchState = HIGH;
bool lastConfirmState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// 长按校准功能
unsigned long greenButtonPressTime = 0;
bool greenButtonLongPressHandled = false;
const unsigned long LONG_PRESS_TIME = 3000;  // 3秒长按

// ==================== 系统状态 ====================
FruitType currentFruit = FRUIT_BANANA;
bool systemReady = false;
bool inFruitTestMode = false;  // 🆕 水果测试模式标志

unsigned long lastUploadTime = 0;
const unsigned long UPLOAD_INTERVAL = 300000;  // 5分钟

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 2000;  // 2秒

// ==================== 🆕 根据你的环境调整的阈值 ====================
// v3.5更新：根据你的实际环境条件(28.8°C, 48.7%)调整阈值
// 你的环境Score约40.5，原45.0阈值太严格

// 香蕉测试阈值（适中）- 用于测试单个水果
const int BANANA_GAS_TEST_THRESHOLD = 10;      // Gas Delta阈值
const float BANANA_SCORE_TEST_THRESHOLD = 38.0; // 🆕 降低！从45.0改为38.0

// 橘子测试阈值（适中）
const int ORANGE_GAS_TEST_THRESHOLD = 15;      
const float ORANGE_SCORE_TEST_THRESHOLD = 45.0; // 🆕 降低！从50.0改为45.0

// 环境判断阈值（宽松）- 用于环境监测
const int ENV_GAS_SPIKE_THRESHOLD = 30;        // 降低！原50
const float ENV_SCORE_THRESHOLD = 30.0;

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  Serial.println("\n========================================");
  Serial.println("  Fruit Monitor v3.1 - Dual Mode");
  Serial.println("========================================");
  Serial.println("Mode A: Env Monitor (default)");
  Serial.println("Mode B: Fruit Test");
  Serial.println("Wiring: CS=7, RST=4, DC=6");
  Serial.println("        MOSI=8, SCK=9, MISO=10");
  Serial.println("========================================\n");
  
  // 1. 按钮
  pinMode(BTN_SWITCH_FRUIT, INPUT_PULLUP);
  pinMode(BTN_CONFIRM, INPUT_PULLUP);
  Serial.println("1. Buttons initialized");
  
  // 2. TFT
  Serial.println("2. Initializing TFT...");
  ui.begin();
  
  #if TFT_TEST_MODE
    Serial.println("\n⚠️ TFT TEST MODE");
    ui.testDisplay();
    Serial.println("Press RESET to continue...\n");
    while(1);
  #endif
  
  ui.showBootScreen();
  delay(2000);
  
  // 3. 传感器
  Serial.println("3. Initializing sensors...");
  sensors.begin();
  
  // 4. 气体校准
  Serial.println("4. Calibrating gas sensor (10s)...");
  ui.showCalibrationScreen();
  
  for (int i = 0; i < 10; i++) {
    sensors.calibrateGasSensor();
    ui.updateCalibrationProgress((i + 1) * 10);
    delay(1000);
  }
  
  int baseline = sensors.getGasBaseline();
  Serial.print("   Gas Baseline: ");
  Serial.print(baseline);
  Serial.println(" ADC");
  
  // 5. LoRa
  Serial.println("5. Initializing LoRaWAN...");
  ui.showLoRaJoiningScreen();
  
  if (!modem.begin(EU868)) {
    Serial.println("   LoRa init failed!");
    ui.showErrorScreen("LoRa Failed");
    while (1);
  }
  
  Serial.print("   Device EUI: ");
  Serial.println(modem.deviceEUI());
  
  int attempts = 0;
  bool connected = false;
  
  while (!connected && attempts < 3) {
    attempts++;
    Serial.print("   Join attempt ");
    Serial.print(attempts);
    Serial.println("/3...");
    
    connected = modem.joinOTAA(TTN_APP_EUI, TTN_APP_KEY);
    
    if (!connected) {
      delay(5000);
    }
  }
  
  if (connected) {
    Serial.println("   ✅ Joined TTN!");
  } else {
    Serial.println("   ⚠️ Offline mode");
  }
  
  // 6. 模型初始化
  freshnessModel.setFruitType(currentFruit);
  
  Serial.println("\n========================================");
  Serial.println("  🟢 System Ready!");
  Serial.println("========================================");
  Serial.println("🌍 Mode: Environment Monitoring");
  Serial.println("   - Shows: Env suitable for storage");
  Serial.println("   - Yellow: Switch fruit (🍌 ↔ 🍊)");
  Serial.println("   - Green: Enter Fruit Test Mode");
  Serial.println("========================================");
  Serial.println("📊 Adjusted Thresholds:");
  Serial.print("   Banana Test: GasΔ>");
  Serial.print(BANANA_GAS_TEST_THRESHOLD);
  Serial.print(" OR Score<");
  Serial.println(BANANA_SCORE_TEST_THRESHOLD);
  Serial.print("   Orange Test: GasΔ>");
  Serial.print(ORANGE_GAS_TEST_THRESHOLD);
  Serial.print(" OR Score<");
  Serial.println(ORANGE_SCORE_TEST_THRESHOLD);
  Serial.println("========================================\n");
  
  systemReady = true;
  
  // 显示环境监测界面
  ui.showMonitoringScreen(currentFruit);
  delay(500);
  updateSensorReadings();
  
  Serial.println("✅ Display initialized!\n");
}

// ==================== Loop ====================
void loop() {
  if (!systemReady) return;
  
  // 处理按钮
  handleButtons();
  
  // 🌍 环境监测模式：自动刷新和上传
  if (!inFruitTestMode) {
    unsigned long currentTime = millis();
    
    // 每2秒更新显示
    if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
      updateSensorReadings();
      lastDisplayUpdate = currentTime;
    }
    
    // 每5分钟上传
    if (currentTime - lastUploadTime >= UPLOAD_INTERVAL) {
      uploadLoRaData();
      lastUploadTime = currentTime;
    }
  }
  // 🧪 水果测试模式：不自动刷新，只响应按钮
}

// ==================== 按钮处理 ====================
void handleButtons() {
  unsigned long currentTime = millis();
  
  bool switchState = digitalRead(BTN_SWITCH_FRUIT);
  bool confirmState = digitalRead(BTN_CONFIRM);
  
  // 🔄 检测绿色按钮长按（环境模式下）
  if (!inFruitTestMode && confirmState == LOW) {
    if (greenButtonPressTime == 0) {
      greenButtonPressTime = currentTime;
      greenButtonLongPressHandled = false;
    }
    
    // 长按3秒 = 重新校准baseline
    if (!greenButtonLongPressHandled && 
        (currentTime - greenButtonPressTime) >= LONG_PRESS_TIME) {
      
      Serial.println("\n>>> 🔵 LONG PRESS: Recalibrating Baseline <<<\n");
      recalibrateGasSensor();
      greenButtonLongPressHandled = true;
      return;  // 不处理短按
    }
  } else if (confirmState == HIGH && greenButtonPressTime != 0) {
    // 按钮释放
    unsigned long pressDuration = currentTime - greenButtonPressTime;
    greenButtonPressTime = 0;
    
    // 如果是短按（<3秒）且已处理过长按，则忽略
    if (greenButtonLongPressHandled) {
      greenButtonLongPressHandled = false;
      lastConfirmState = confirmState;
      return;
    }
  }
  
  // 防抖
  if ((switchState != lastSwitchState || confirmState != lastConfirmState) &&
      (currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    
    lastDebounceTime = currentTime;
    
    // 🟡 黄色按钮
    if (switchState == LOW && lastSwitchState == HIGH) {
      Serial.println("\n>>> 🟡 YELLOW BUTTON <<<");
      
      if (inFruitTestMode) {
        // 🧪 测试模式：退出测试
        exitFruitTestMode();
      } else {
        // 🌍 环境模式：切换水果
        switchFruit();
      }
    }
    
    // 🟢 绿色按钮（短按）
    if (confirmState == LOW && lastConfirmState == HIGH && !greenButtonLongPressHandled) {
      Serial.println("\n>>> 🟢 GREEN BUTTON <<<");
      
      if (inFruitTestMode) {
        // 🧪 测试模式：重新测试
        runFruitTest();
      } else {
        // 🌍 环境模式：进入测试
        enterFruitTestMode();
      }
    }
  }
  
  lastSwitchState = switchState;
  lastConfirmState = confirmState;
}

// ==================== 🔵 重新校准Baseline ====================
void recalibrateGasSensor() {
  ui.showCalibrationScreen();
  
  Serial.println("╔═══════════════════════════════════╗");
  Serial.println("║   RECALIBRATING GAS BASELINE     ║");
  Serial.println("╠═══════════════════════════════════╣");
  Serial.println("║ Please ensure:                    ║");
  Serial.println("║ - No fruit near sensor            ║");
  Serial.println("║ - Clean air environment           ║");
  Serial.println("╠═══════════════════════════════════╣");
  Serial.println("║ Calibrating... (5 samples)        ║");
  Serial.println("╚═══════════════════════════════════╝\n");
  
  int oldBaseline = sensors.getGasBaseline();
  
  // 重新校准（5次取平均）
  for (int i = 0; i < 5; i++) {
    sensors.calibrateGasSensor();
    
    // 读取当前值显示
    SensorData data = sensors.readSensors();
    Serial.print("  Sample ");
    Serial.print(i + 1);
    Serial.print("/5: ");
    Serial.print(data.gasRaw);
    Serial.println(" ADC");
    delay(200);
  }
  
  int newBaseline = sensors.getGasBaseline();
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║   CALIBRATION COMPLETE!           ║");
  Serial.println("╠═══════════════════════════════════╣");
  Serial.print("║ Old Baseline: ");
  Serial.print(oldBaseline);
  Serial.println(" ADC");
  Serial.print("║ New Baseline: ");
  Serial.print(newBaseline);
  Serial.println(" ADC");
  
  int change = newBaseline - oldBaseline;
  Serial.print("║ Change:       ");
  if (change > 0) Serial.print("+");
  Serial.print(change);
  Serial.println(" ADC");
  Serial.println("╚═══════════════════════════════════╝\n");
  
  delay(2000);
  
  // 回到环境监测
  ui.showMonitoringScreen(currentFruit);
  delay(500);
  updateSensorReadings();
}

// ==================== 🟡 切换水果（环境模式）====================
void switchFruit() {
  currentFruit = (FruitType)((currentFruit + 1) % 2);
  
  String fruitName = FruitDatabase::getTypeName(currentFruit);
  String fruitEmoji = FruitDatabase::getEmoji(currentFruit);
  
  Serial.print("Switched to: ");
  Serial.print(fruitEmoji);
  Serial.print(" ");
  Serial.println(fruitName);
  
  freshnessModel.setFruitType(currentFruit);
  
  ui.showFruitSwitchAnimation(currentFruit);
  delay(1000);
  
  ui.showMonitoringScreen(currentFruit);
  delay(500);
  
  updateSensorReadings();
}

// ==================== 🟢 进入水果测试模式 ====================
void enterFruitTestMode() {
  Serial.println("\n═══════════════════════════════════════");
  Serial.println("  🧪 Entering FRUIT TEST MODE");
  Serial.println("═══════════════════════════════════════");
  Serial.println("Put the fruit near MQ-135 sensor");
  Serial.println("Testing if THIS fruit is safe to eat");
  Serial.println("═══════════════════════════════════════\n");
  
  inFruitTestMode = true;
  
  runFruitTest();
}

// ==================== 🧪 运行水果测试 ====================
void runFruitTest() {
  Serial.println("\n--- 🧪 Running Fruit Test ---");
  
  // 读传感器
  SensorData data = sensors.readSensors();
  
  if (!data.valid) {
    Serial.println("❌ Sensor read failed!");
    return;
  }
  
  // 更新模型
  freshnessModel.updateReadings(data.temperature, data.humidity, data.gasDelta);
  float score = freshnessModel.getScore();
  
  // 评估：这个水果能不能吃
  bool isSpoiled = evaluateFruitTest(currentFruit, data.gasDelta, score);
  
  // 打印结果
  printFruitTestResult(data, score, isSpoiled);
  
  // 显示结果
  ui.showFruitTestResult(currentFruit, isSpoiled);
}

// ==================== 评估水果测试 ====================
bool evaluateFruitTest(FruitType fruit, int gasDelta, float score) {
  int gasThreshold;
  float scoreThreshold;
  
  if (fruit == FRUIT_BANANA) {
    gasThreshold = BANANA_GAS_TEST_THRESHOLD;
    scoreThreshold = BANANA_SCORE_TEST_THRESHOLD;
  } else {  // FRUIT_ORANGE
    gasThreshold = ORANGE_GAS_TEST_THRESHOLD;
    scoreThreshold = ORANGE_SCORE_TEST_THRESHOLD;
  }
  
  bool gasBad = (gasDelta > gasThreshold);
  bool scoreBad = (score < scoreThreshold);
  
  Serial.print("   Gas Delta: ");
  Serial.print(gasDelta);
  Serial.print(" (threshold: >");
  Serial.print(gasThreshold);
  Serial.print(") ");
  Serial.println(gasBad ? "❌ HIGH" : "✅ OK");
  
  Serial.print("   Score: ");
  Serial.print(score, 1);
  Serial.print(" (threshold: >");
  Serial.print(scoreThreshold, 1);
  Serial.print(") ");
  Serial.println(scoreBad ? "❌ LOW" : "✅ OK");
  
  return (gasBad || scoreBad);
}

// ==================== 打印测试结果 ====================
void printFruitTestResult(const SensorData& data, float score, bool isSpoiled) {
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.print("║ 🧪 FRUIT TEST: ");
  Serial.print(FruitDatabase::getEmoji(currentFruit));
  Serial.print(" ");
  Serial.print(FruitDatabase::getTypeName(currentFruit));
  Serial.println();
  Serial.println("╠═══════════════════════════════════╣");
  
  Serial.print("║ Temp:     ");
  Serial.print(data.temperature, 1);
  Serial.println(" C");
  
  Serial.print("║ Humidity: ");
  Serial.print(data.humidity, 1);
  Serial.println(" %");
  
  Serial.print("║ Gas Δ:    ");
  if (data.gasDelta > 0) Serial.print("+");
  Serial.print(data.gasDelta);
  Serial.println(" ADC");
  
  Serial.print("║ Score:    ");
  Serial.print(score, 1);
  Serial.println(" / 100");
  
  Serial.println("╠═══════════════════════════════════╣");
  
  Serial.print("║ Result:   ");
  if (isSpoiled) {
    Serial.println("🔴 DO NOT EAT!");
  } else {
    Serial.println("🟢 OK TO EAT");
  }
  
  Serial.println("╚═══════════════════════════════════╝\n");
}

// ==================== 🟡 退出水果测试模式 ====================
void exitFruitTestMode() {
  Serial.println("\n═══════════════════════════════════════");
  Serial.println("  🌍 Exiting FRUIT TEST MODE");
  Serial.println("═══════════════════════════════════════");
  Serial.println("💡 Tip: For best results, wait 1-2 mins");
  Serial.println("    before testing another fruit");
  Serial.println("═══════════════════════════════════════\n");
  
  inFruitTestMode = false;
  
  // 显示返回提示（不强制等待）
  ui.showReturnPrompt();
  delay(2000);  // 显示2秒提示
  
  // 直接回到环境监测
  ui.showMonitoringScreen(currentFruit);
  delay(500);
  updateSensorReadings();
}

// ==================== 🌍 更新环境监测数据 ====================
void updateSensorReadings() {
  SensorData data = sensors.readSensors();
  
  if (!data.valid) {
    Serial.println("Sensor read failed!");
    return;
  }
  
  freshnessModel.updateReadings(data.temperature, data.humidity, data.gasDelta);
  
  float score = freshnessModel.getScore();
  int remainDays = freshnessModel.getRemainingDays();
  FreshnessStage stage = freshnessModel.getStage();
  int storageQuality = freshnessModel.calculateStorageScore(data.temperature, data.humidity);
  
  // 环境判断（宽松阈值）
  bool envBad = checkEnvironmentSpoilage(data.gasDelta, score);
  
  printMonitoringData(data, score, remainDays, stage, storageQuality, envBad);
  
  ui.updateMonitoringData(currentFruit, &data, score, remainDays, stage, storageQuality);
  
  if (envBad) {
    ui.showSpoilageWarning();
  }
}

// ==================== 环境变坏判断（宽松）====================
bool checkEnvironmentSpoilage(int gasDelta, float score) {
  bool gasSpike = (gasDelta > ENV_GAS_SPIKE_THRESHOLD);
  bool lowScore = (score < ENV_SCORE_THRESHOLD);
  
  return (gasSpike || lowScore);
}

// ==================== 打印环境监测数据 ====================
void printMonitoringData(const SensorData& data, float score, int remainDays,
                         FreshnessStage stage, int storageQuality, bool envBad) {
  Serial.println("\n┌─────────────────────────────────────┐");
  Serial.print("│ 🌍 Env Monitor: ");
  Serial.print(FruitDatabase::getEmoji(currentFruit));
  Serial.print(" ");
  Serial.println(FruitDatabase::getTypeName(currentFruit));
  Serial.println("├─────────────────────────────────────┤");
  
  Serial.print("│ Temp:     ");
  Serial.print(data.temperature, 1);
  Serial.println(" C");
  
  Serial.print("│ Humidity: ");
  Serial.print(data.humidity, 1);
  Serial.println(" %");
  
  Serial.print("│ Gas Raw:  ");
  Serial.print(data.gasRaw);
  Serial.println(" ADC");
  
  Serial.print("│ Gas Base: ");
  Serial.print(data.gasBaseline);
  Serial.println(" ADC");
  
  Serial.print("│ Gas Δ:    ");
  if (data.gasDelta > 0) Serial.print("+");
  Serial.print(data.gasDelta);
  Serial.println(" ADC");
  
  Serial.println("├─────────────────────────────────────┤");
  
  Serial.print("│ Score:    ");
  Serial.print(score, 1);
  Serial.println(" / 100");
  
  Serial.print("│ Stage:    ");
  switch (stage) {
    case STAGE_VERY_FRESH: Serial.println("VERY FRESH"); break;
    case STAGE_GOOD:       Serial.println("GOOD"); break;
    case STAGE_EAT_TODAY:  Serial.println("EAT TODAY"); break;
    case STAGE_SPOILED:    Serial.println("SPOILED"); break;
  }
  
  Serial.print("│ Shelf:    ");
  if (remainDays >= 0) {
    Serial.print(remainDays);
    Serial.println(" days");
  } else {
    Serial.println("Expired");
  }
  
  Serial.print("│ Storage:  ");
  Serial.print(storageQuality);
  Serial.println(" / 100");
  
  Serial.println("├─────────────────────────────────────┤");
  
  Serial.print("│ Env:      ");
  if (envBad) {
    Serial.println("🔴 ALERT!");
  } else if (stage == STAGE_VERY_FRESH || stage == STAGE_GOOD) {
    Serial.println("🟢 GOOD");
  } else {
    Serial.println("🟡 WATCH");
  }
  
  Serial.println("└─────────────────────────────────────┘\n");
}

// ==================== 上传LoRa数据 ====================
void uploadLoRaData() {
  Serial.println("\nUploading to TTN...");
  
  SensorData data = sensors.readSensors();
  if (!data.valid) {
    Serial.println("Invalid data, skip");
    return;
  }
  
  freshnessModel.updateReadings(data.temperature, data.humidity, data.gasDelta);
  
  uint8_t payload[13];
  
  payload[0] = (uint8_t)currentFruit;
  
  int16_t temp = (int16_t)(data.temperature * 100);
  payload[1] = (temp >> 8) & 0xFF;
  payload[2] = temp & 0xFF;
  
  uint16_t humid = (uint16_t)(data.humidity * 100);
  payload[3] = (humid >> 8) & 0xFF;
  payload[4] = humid & 0xFF;
  
  payload[5] = (data.gasRaw >> 8) & 0xFF;
  payload[6] = data.gasRaw & 0xFF;
  
  int16_t delta = (int16_t)data.gasDelta;
  payload[7] = (delta >> 8) & 0xFF;
  payload[8] = delta & 0xFF;
  
  payload[9] = (uint8_t)freshnessModel.getScore();
  
  int remainDays = freshnessModel.getRemainingDays();
  payload[10] = (uint8_t)(remainDays < 0 ? 255 : remainDays);
  
  payload[11] = (uint8_t)freshnessModel.getStage();
  
  unsigned long ageHours = millis() / 3600000;
  payload[12] = (uint8_t)(ageHours > 255 ? 255 : ageHours);
  
  modem.beginPacket();
  modem.write(payload, 13);
  int err = modem.endPacket(true);
  
  if (err > 0) {
    Serial.println("✅ Sent!");
  } else {
    Serial.print("❌ Failed: ");
    Serial.println(err);
  }
  
  ui.showUploadStatus(err > 0);
  delay(2000);
}
