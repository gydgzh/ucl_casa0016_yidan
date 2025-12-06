/*
 * Multi-Fruit Freshness Monitor
 * 多水果新鲜度监测系统 - 2按钮版本
 * 
 * 硬件配置:
 * - Arduino MKR WAN 1310
 * - DHT22 (D3) - 温湿度传感器
 * - MQ-135 (A0) - 气体传感器（需分压电路）
 * - TFT ILI9488 (软件SPI) - 显示屏
 * - 黄色按钮 (D0) - 切换水果
 * - 绿色按钮 (D1) - 确认/刷新
 * - 蓝色拨动开关 - 电源开关（硬件）
 * 
 * 功能:
 * 1. 监测4种水果（香蕉、橘子、苹果、葡萄）
 * 2. 显示剩余存储时间
 * 3. 通过气体检测水果是否变坏
 * 4. LoRaWAN数据上传
 * 
 * 参考文献:
 * - Saltveit (1999): 乙烯气体与水果成熟
 * - Kader (2002): 水果采后技术
 * - FAO/USDA: 水果存储标准
 */

#include <MKRWAN.h>
#include <DHT.h>
#include <Arduino_GFX_Library.h>

#include "secrets.h"
#include "fruit_profiles.h"
#include "sensors.h"
#include "freshness_model.h"
#include "ui_manager.h"

// ==================== 全局对象 ====================
LoRaModem modem;
Sensors sensors;
FreshnessModel freshnessModel;
UIManager ui;

// ==================== 按钮配置 ====================
#define BTN_SWITCH_FRUIT  0    // D0 - 黄色按钮：切换水果
#define BTN_CONFIRM       1    // D1 - 绿色按钮：确认/刷新

// 按钮状态
bool lastSwitchState = HIGH;
bool lastConfirmState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ==================== 系统状态 ====================
FruitType currentFruit = FRUIT_BANANA;  // 默认香蕉
bool systemReady = false;
unsigned long lastUploadTime = 0;
const unsigned long UPLOAD_INTERVAL = 300000;  // 5分钟上传一次
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 2000;  // 2秒刷新显示

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);  // 等待3秒或串口连接
  
  Serial.println("\n========================================");
  Serial.println("  Multi-Fruit Freshness Monitor");
  Serial.println("  多水果新鲜度监测系统 - v2.0");
  Serial.println("========================================\n");
  
  // 1. 初始化按钮
  Serial.println("1. Initializing buttons...");
  pinMode(BTN_SWITCH_FRUIT, INPUT_PULLUP);
  pinMode(BTN_CONFIRM, INPUT_PULLUP);
  Serial.println("   ✓ Buttons configured (INPUT_PULLUP)");
  
  // 2. 初始化TFT显示屏
  Serial.println("\n2. Initializing TFT display...");
  ui.begin();
  ui.showBootScreen();
  delay(2000);
  Serial.println("   ✓ TFT initialized");
  
  // 3. 初始化传感器
  Serial.println("\n3. Initializing sensors...");
  sensors.begin();
  Serial.println("   ✓ DHT22 initialized");
  Serial.println("   ✓ MQ-135 initialized");
  
  // 4. 校准气体传感器（10秒）
  Serial.println("\n4. Calibrating gas sensor (10 seconds)...");
  Serial.println("   ⚠ Please ensure sensor is in clean air");
  ui.showCalibrationScreen();
  
  for (int i = 0; i < 10; i++) {
    sensors.calibrateGasSensor();
    ui.updateCalibrationProgress((i + 1) * 10);
    delay(1000);
  }
  
  Serial.print("   ✓ Gas baseline: ");
  Serial.println(sensors.getGasBaseline());
  
  // 5. 初始化LoRa
  Serial.println("\n5. Initializing LoRaWAN...");
  ui.showLoRaJoiningScreen();
  
  if (!modem.begin(EU868)) {
    Serial.println("   ✗ Failed to start LoRa module");
    ui.showErrorScreen("LoRa Init Failed");
    while (1);
  }
  
  Serial.print("   Device EUI: ");
  Serial.println(modem.deviceEUI());
  
  // 连接到TTN（OTAA）
  Serial.println("   Joining The Things Network...");
  int attempts = 0;
  bool connected = false;
  
  while (!connected && attempts < 3) {
    attempts++;
    Serial.print("   Attempt ");
    Serial.print(attempts);
    Serial.println("/3...");
    
    connected = modem.joinOTAA(TTN_APP_EUI, TTN_APP_KEY);
    
    if (!connected) {
      Serial.println("   ✗ Join failed, retrying...");
      delay(5000);
    }
  }
  
  if (connected) {
    Serial.println("   ✓ Successfully joined TTN!");
  } else {
    Serial.println("   ✗ Failed to join after 3 attempts");
    Serial.println("   ⚠ System will continue in offline mode");
  }
  
  // 6. 初始化新鲜度模型
  Serial.println("\n6. Initializing freshness model...");
  freshnessModel.setFruitType(currentFruit);
  Serial.println("   ✓ Model initialized with Banana profile");
  
  // 7. 系统就绪
  Serial.println("\n========================================");
  Serial.println("  🍎 System Ready!");
  Serial.println("========================================");
  Serial.println("\nControls:");
  Serial.println("  🟡 Yellow button (D0): Switch fruit");
  Serial.println("  🟢 Green button (D1): Confirm/Refresh");
  Serial.println("\nCurrent fruit: 🍌 Banana\n");
  
  systemReady = true;
  
  // 显示初始界面
  ui.showMonitoringScreen(currentFruit, NULL, 0);
  delay(1000);
}

// ==================== Loop ====================
void loop() {
  if (!systemReady) return;
  
  // 检测按钮
  handleButtons();
  
  // 定期更新显示和上传数据
  unsigned long currentTime = millis();
  
  // 每2秒更新显示
  if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    updateSensorReadings();
    lastDisplayUpdate = currentTime;
  }
  
  // 每5分钟上传LoRa数据
  if (currentTime - lastUploadTime >= UPLOAD_INTERVAL) {
    uploadLoRaData();
    lastUploadTime = currentTime;
  }
}

// ==================== 按钮处理 ====================
void handleButtons() {
  unsigned long currentTime = millis();
  
  // 读取当前按钮状态
  bool switchState = digitalRead(BTN_SWITCH_FRUIT);
  bool confirmState = digitalRead(BTN_CONFIRM);
  
  // 防抖处理
  if ((switchState != lastSwitchState || confirmState != lastConfirmState) &&
      (currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    
    // 黄色按钮 - 切换水果（循环）
    if (switchState == LOW && lastSwitchState == HIGH) {
      switchFruit();
      lastDebounceTime = currentTime;
    }
    
    // 绿色按钮 - 确认/刷新显示
    if (confirmState == LOW && lastConfirmState == HIGH) {
      refreshDisplay();
      lastDebounceTime = currentTime;
    }
    
    lastSwitchState = switchState;
    lastConfirmState = confirmState;
  }
}

// ==================== 切换水果 ====================
void switchFruit() {
  // 循环切换: 0 → 1 → 2 → 3 → 0
  currentFruit = (FruitType)((currentFruit + 1) % 4);
  
  String fruitName = FruitDatabase::getTypeName(currentFruit);
  String fruitEmoji = FruitDatabase::getEmoji(currentFruit);
  
  Serial.print("\n🔄 Switched to: ");
  Serial.print(fruitEmoji);
  Serial.print(" ");
  Serial.println(fruitName);
  
  // 重置新鲜度模型
  freshnessModel.setFruitType(currentFruit);
  
  // 显示切换动画
  ui.showFruitSwitchAnimation(currentFruit);
  delay(1000);
  
  // 更新显示
  updateSensorReadings();
}

// ==================== 刷新显示 ====================
void refreshDisplay() {
  Serial.println("\n🔄 Refreshing display...");
  updateSensorReadings();
}

// ==================== 更新传感器读数和显示 ====================
void updateSensorReadings() {
  // 读取传感器
  SensorData data = sensors.readSensors();
  
  if (!data.valid) {
    Serial.println("⚠ Sensor reading failed");
    return;
  }
  
  // 更新新鲜度模型
  freshnessModel.updateReadings(data.temperature, data.humidity, data.gasDelta);
  
  // 获取评估结果
  float score = freshnessModel.getScore();
  int remainDays = freshnessModel.getRemainingDays();
  FreshnessStage stage = freshnessModel.getStage();
  int storageQuality = freshnessModel.calculateStorageScore(data.temperature, data.humidity);
  
  // 判断水果是否变坏（基于气体变化）
  bool isSpoiled = checkFruitSpoilage(data.gasDelta, score);
  
  // 打印数据
  printMonitoringData(data, score, remainDays, stage, storageQuality, isSpoiled);
  
  // 更新TFT显示
  ui.showMonitoringScreen(currentFruit, &data, score, remainDays, stage, storageQuality);
  
  // 如果检测到变坏，显示警告
  if (isSpoiled) {
    ui.showSpoilageWarning();
  }
}

// ==================== 判断水果是否变坏 ====================
bool checkFruitSpoilage(int gasDelta, float score) {
  /*
   * 判断标准（基于科学文献）:
   * 1. 气体浓度快速上升 (Saltveit, 1999)
   * 2. 新鲜度评分低于阈值
   * 
   * 参考: Peris & Escuder-Gilabert (2009)
   * "Electronic noses for food quality control"
   */
  
  const int GAS_SPIKE_THRESHOLD = 50;  // 气体突增阈值
  const float SCORE_THRESHOLD = 30.0;  // 评分阈值
  
  bool gasSpike = (gasDelta > GAS_SPIKE_THRESHOLD);
  bool lowScore = (score < SCORE_THRESHOLD);
  
  // 任一条件满足即判定为变坏
  return (gasSpike || lowScore);
}

// ==================== 打印监测数据 ====================
void printMonitoringData(const SensorData& data, float score, int remainDays,
                         FreshnessStage stage, int storageQuality, bool isSpoiled) {
  Serial.println("\n┌─────────────────────────────────────┐");
  Serial.print("│ Monitoring: ");
  Serial.print(FruitDatabase::getEmoji(currentFruit));
  Serial.print(" ");
  Serial.println(FruitDatabase::getTypeName(currentFruit));
  Serial.println("├─────────────────────────────────────┤");
  
  // 环境数据
  Serial.print("│ Temperature:  ");
  Serial.print(data.temperature, 1);
  Serial.println(" °C");
  
  Serial.print("│ Humidity:     ");
  Serial.print(data.humidity, 1);
  Serial.println(" %");
  
  Serial.print("│ Gas Raw:      ");
  Serial.println(data.gasRaw);
  
  Serial.print("│ Gas Delta:    ");
  if (data.gasDelta > 0) Serial.print("+");
  Serial.println(data.gasDelta);
  
  Serial.println("├─────────────────────────────────────┤");
  
  // 评估结果
  Serial.print("│ Freshness:    ");
  Serial.print(score, 1);
  Serial.println(" / 100");
  
  Serial.print("│ Stage:        ");
  switch (stage) {
    case STAGE_VERY_FRESH: Serial.println("VERY FRESH ✓"); break;
    case STAGE_GOOD:       Serial.println("GOOD ✓"); break;
    case STAGE_EAT_TODAY:  Serial.println("EAT TODAY ⚠"); break;
    case STAGE_SPOILED:    Serial.println("SPOILED ✗"); break;
  }
  
  Serial.print("│ Shelf Life:   ");
  if (remainDays >= 0) {
    Serial.print(remainDays);
    Serial.println(" days");
  } else {
    Serial.println("Expired");
  }
  
  Serial.print("│ Storage:      ");
  Serial.print(storageQuality);
  Serial.println(" / 100");
  
  Serial.println("├─────────────────────────────────────┤");
  
  // 水果状态判断
  Serial.print("│ Status:       ");
  if (isSpoiled) {
    Serial.println("🔴 SPOILED!");
  } else if (stage == STAGE_VERY_FRESH || stage == STAGE_GOOD) {
    Serial.println("🟢 FRESH");
  } else {
    Serial.println("🟡 EAT SOON");
  }
  
  Serial.println("└─────────────────────────────────────┘\n");
}

// ==================== 上传LoRa数据 ====================
void uploadLoRaData() {
  Serial.println("\n📡 Uploading data to TTN...");
  
  // 读取当前数据
  SensorData data = sensors.readSensors();
  if (!data.valid) {
    Serial.println("   ✗ Invalid sensor data, skipping upload");
    return;
  }
  
  freshnessModel.updateReadings(data.temperature, data.humidity, data.gasDelta);
  
  // 构建Payload（13字节）
  uint8_t payload[13];
  
  // Byte 0: 水果类型
  payload[0] = (uint8_t)currentFruit;
  
  // Byte 1-2: 温度 (int16, 0.01°C精度)
  int16_t temp = (int16_t)(data.temperature * 100);
  payload[1] = (temp >> 8) & 0xFF;
  payload[2] = temp & 0xFF;
  
  // Byte 3-4: 湿度 (uint16, 0.01%精度)
  uint16_t humid = (uint16_t)(data.humidity * 100);
  payload[3] = (humid >> 8) & 0xFF;
  payload[4] = humid & 0xFF;
  
  // Byte 5-6: 气体原始值
  payload[5] = (data.gasRaw >> 8) & 0xFF;
  payload[6] = data.gasRaw & 0xFF;
  
  // Byte 7-8: 气体变化量 (int16)
  int16_t delta = (int16_t)data.gasDelta;
  payload[7] = (delta >> 8) & 0xFF;
  payload[8] = delta & 0xFF;
  
  // Byte 9: 新鲜度评分
  payload[9] = (uint8_t)freshnessModel.getScore();
  
  // Byte 10: 剩余天数 (int8)
  int remainDays = freshnessModel.getRemainingDays();
  payload[10] = (uint8_t)(remainDays < 0 ? 255 : remainDays);
  
  // Byte 11: 阶段代码
  payload[11] = (uint8_t)freshnessModel.getStage();
  
  // Byte 12: 运行时长（小时）
  unsigned long ageHours = millis() / 3600000;
  payload[12] = (uint8_t)(ageHours > 255 ? 255 : ageHours);
  
  // 发送数据
  modem.beginPacket();
  modem.write(payload, 13);
  int err = modem.endPacket(true);
  
  if (err > 0) {
    Serial.println("   ✓ Data sent successfully!");
  } else {
    Serial.print("   ✗ Send failed, error: ");
    Serial.println(err);
  }
  
  // 显示上传状态
  ui.showUploadStatus(err > 0);
  delay(2000);
}

