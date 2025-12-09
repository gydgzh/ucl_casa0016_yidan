/*
 * UI Manager Implementation - v3.0 现代化设计
 * 完全保持原有逻辑，只优化视觉效果
 */

#include "ui_manager.h"

UIManager::UIManager() {
    bus = NULL;
    gfx = NULL;
}

// ==================== TFT初始化 ====================
void UIManager::begin() {
    Serial.println("   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("   TFT Initialization");
    Serial.println("   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // 硬件复位
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH);
    delay(10);
    digitalWrite(TFT_RST, LOW);
    delay(20);
    digitalWrite(TFT_RST, HIGH);
    delay(150);
    
    // 创建SPI总线
    bus = new Arduino_SWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
    
    // 创建驱动
    #if TFT_DRIVER == 1
      gfx = new Arduino_ILI9488_18bit(bus, TFT_RST, 0, false);
    #elif TFT_DRIVER == 2
      gfx = new Arduino_ILI9341(bus, TFT_RST, 0, false);
    #elif TFT_DRIVER == 3
      gfx = new Arduino_ST7796(bus, TFT_RST, 0, false);
    #endif
    
    if (!gfx->begin()) {
        Serial.println("   ✗ TFT init failed!");
        return;
    }
    
    gfx->setRotation(1);
    gfx->fillScreen(COLOR_BG_DARK);
    
    Serial.println("   ✅ TFT Ready!");
    Serial.println("   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

// ==================== TFT测试 ====================
void UIManager::testDisplay() {
    Serial.println("\n🧪 TFT DISPLAY TEST\n");
    
    gfx->fillScreen(0x0000); delay(1000);
    Serial.println("   BLACK");
    
    gfx->fillScreen(0xF800); delay(1000);
    Serial.println("   RED");
    
    gfx->fillScreen(0x07E0); delay(1000);
    Serial.println("   GREEN");
    
    gfx->fillScreen(0x001F); delay(1000);
    Serial.println("   BLUE");
    
    gfx->fillScreen(0xFFFF); delay(1000);
    Serial.println("   WHITE");
    
    gfx->fillScreen(0x0000);
    gfx->setTextSize(3);
    gfx->setTextColor(0xFFFF);
    gfx->setCursor(80, 120);
    gfx->print("TFT TEST OK!");
    
    Serial.println("\n✅ TFT works! Change TFT_TEST_MODE to false\n");
}

// ==================== 启动画面 ====================
void UIManager::showBootScreen() {
    gfx->fillScreen(COLOR_BG_DARK);
    
    // 顶部装饰条
    gfx->fillRect(0, 0, SCREEN_WIDTH, 70, COLOR_PRIMARY);
    gfx->fillRect(0, 65, SCREEN_WIDTH, 5, COLOR_ACCENT);
    
    // 标题
    gfx->setTextSize(4);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    drawCenteredText("Fruit Monitor", 20, COLOR_TEXT_PRIMARY, 4);
    
    // 副标题
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_BG_DARK);
    drawCenteredText("Env + Fruit Test Mode", 90, COLOR_TEXT_SECONDARY, 1);
    
    // 接线信息
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT_DIM);
    drawCenteredText("CS=7 DC=6 RST=4", 150, COLOR_TEXT_DIM, 1);
    
    // 初始化状态
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_ACCENT);
    drawCenteredText("Initializing...", 200, COLOR_ACCENT, 2);
    
    // 底部装饰点
    for (int i = 0; i < 5; i++) {
        int x = (SCREEN_WIDTH / 2) - 40 + (i * 20);
        gfx->fillCircle(x, 260, 4, COLOR_PRIMARY);
    }
}

// ==================== 校准画面 ====================
void UIManager::showCalibrationScreen() {
    gfx->fillScreen(COLOR_BG_DARK);
    
    // 卡片背景
    drawCard(40, 60, SCREEN_WIDTH-80, 120, COLOR_BG_CARD);
    
    // 图标
    gfx->setTextSize(5);
    gfx->setTextColor(COLOR_WARNING);
    drawCenteredText("", 80, COLOR_WARNING, 5);
    
    // 标题
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    drawCenteredText("Calibrating", 140, COLOR_TEXT_PRIMARY, 3);
    
    // 副标题
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    drawCenteredText("Gas Sensor", 175, COLOR_TEXT_SECONDARY, 2);
    
    // 提示
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT_DIM);
    drawCenteredText("Wait 10 seconds...", 220, COLOR_TEXT_DIM, 1);
}

// ==================== 校准进度 ====================
void UIManager::updateCalibrationProgress(int percent) {
    int barWidth = 360;
    int barHeight = 40;
    int barX = (SCREEN_WIDTH - barWidth) / 2;
    int barY = 250;
    
    // 进度条背景
    gfx->fillRoundRect(barX, barY, barWidth, barHeight, 20, COLOR_BG_LIGHT);
    gfx->drawRoundRect(barX, barY, barWidth, barHeight, 20, COLOR_BORDER);
    
    // 填充
    int fillWidth = (barWidth - 8) * percent / 100;
    if (fillWidth > 0) {
        uint16_t barColor = (percent < 50) ? COLOR_WARNING : COLOR_VERY_FRESH;
        gfx->fillRoundRect(barX + 4, barY + 4, fillWidth, barHeight - 8, 18, barColor);
    }
    
    // 百分比
    char buf[10];
    sprintf(buf, "%d%%", percent);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    int textWidth = strlen(buf) * 12;
    gfx->setCursor((SCREEN_WIDTH - textWidth) / 2, barY + 13);
    gfx->print(buf);
}

// ==================== LoRa连接画面 ====================
void UIManager::showLoRaJoiningScreen() {
    gfx->fillScreen(COLOR_BG_DARK);
    
    // 卡片
    drawCard(60, 80, SCREEN_WIDTH-120, 160, COLOR_BG_CARD);
    
    // 图标
    gfx->setTextSize(5);
    gfx->setTextColor(COLOR_PRIMARY);
    drawCenteredText("", 100, COLOR_PRIMARY, 5);
    
    // 标题
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    drawCenteredText("Joining", 150, COLOR_TEXT_PRIMARY, 3);
    
    // 副标题
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    drawCenteredText("LoRaWAN Network", 185, COLOR_TEXT_SECONDARY, 2);
    
    // 提示
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_ACCENT);
    drawCenteredText("Please wait...", 220, COLOR_ACCENT, 1);
}

// ==================== 错误画面 ====================
void UIManager::showErrorScreen(const char* message) {
    gfx->fillScreen(COLOR_DANGER);
    
    gfx->setTextSize(6);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    drawCenteredText("!", 80, COLOR_TEXT_PRIMARY, 6);
    
    gfx->setTextSize(4);
    drawCenteredText("ERROR", 160, COLOR_TEXT_PRIMARY, 4);
    
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_BG_DARK);
    drawCenteredText(message, 220, COLOR_BG_DARK, 2);
}

// ==================== 环境监测界面（模式A）====================
void UIManager::showMonitoringScreen(FruitType fruit) {
    Serial.println("   Drawing monitoring screen...");
    
    // 深色背景
    gfx->fillScreen(COLOR_BG_DARK);
    
    String fruitName = FruitDatabase::getTypeName(fruit);
    String fruitEmoji = FruitDatabase::getEmoji(fruit);
    
    // ===== 顶部栏卡片 =====
    drawCard(5, 5, SCREEN_WIDTH-10, 45, COLOR_BG_CARD);
    
    // 水果信息（左侧）
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_PRIMARY);
    gfx->setCursor(15, 15);
    gfx->print(fruitEmoji);
    gfx->print(" ");
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    gfx->print(fruitName);
    
    // Env Monitor标识（右侧）
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_ACCENT);
    gfx->setCursor(SCREEN_WIDTH - 85, 20);
    gfx->print("Env Monitor");
    
    // ===== 主数据区域卡片 =====
    drawCard(10, 60, SCREEN_WIDTH-20, 180, COLOR_BG_CARD);
    
    // 左侧固定标签
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    
    int labelX = 25;
    gfx->setCursor(labelX, 75);  gfx->print("Stage:");
    gfx->setCursor(labelX, 105); gfx->print("Temp :");
    gfx->setCursor(labelX, 135); gfx->print("Hum  :");
    gfx->setCursor(labelX, 165); gfx->print("Gas  :");
    gfx->setCursor(labelX, 195); gfx->print("Shelf:");
    gfx->setCursor(labelX, 225); gfx->print("Score:");
    
    // Storage标签（右下）
    gfx->setCursor(320, 225);
    gfx->print("Stor:");
    
    // ===== 底部按钮提示 =====
    // 黄色按钮提示
    gfx->fillRoundRect(10, 250, 220, 25, 5, COLOR_BG_LIGHT);
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_WARNING);
    gfx->setCursor(15, 258);
    gfx->print(" Yellow: Switch Fruit");
    
    // 绿色按钮提示
    gfx->fillRoundRect(250, 250, 220, 25, 5, COLOR_BG_LIGHT);
    gfx->setTextColor(COLOR_VERY_FRESH);
    gfx->setCursor(255, 258);
    gfx->print(" Green: Fruit Test");
    
    // 分隔线
    gfx->drawFastHLine(10, 55, SCREEN_WIDTH-20, COLOR_BORDER);
    gfx->drawFastHLine(10, 245, SCREEN_WIDTH-20, COLOR_BORDER);
    
    Serial.println("   ✓ Framework drawn");
}

// ==================== 环境数据更新（模式A）====================
void UIManager::updateMonitoringData(FruitType fruit, const SensorData* data,
                                     float score, int remainDays,
                                     FreshnessStage stage, int storageQuality) {
    if (data == NULL || !data->valid) {
        return;
    }
    
    const char* stageName = getStageName(stage);
    uint16_t stageColor = getStageColor(stage);
    int valueX = 90;  // 数值开始X坐标
    
    // ===== Stage =====
    gfx->fillRect(valueX, 72, 220, 20, COLOR_BG_CARD);
    gfx->setTextSize(2);
    gfx->setTextColor(stageColor);
    gfx->setCursor(valueX, 72);
    gfx->print(stageName);
    
    // ===== Temp =====
    gfx->fillRect(valueX, 102, 150, 20, COLOR_BG_CARD);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_PRIMARY);
    gfx->setCursor(valueX, 102);
    gfx->print(data->temperature, 1);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    gfx->print(" C");
    
    // ===== Hum =====
    gfx->fillRect(valueX, 132, 150, 20, COLOR_BG_CARD);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_PRIMARY);
    gfx->setCursor(valueX, 132);
    gfx->print(data->humidity, 1);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    gfx->print(" %");
    
    // ===== Gas Delta（带颜色） =====
    uint16_t gasColor = getGasColor(data->gasDelta);
    gfx->fillRect(valueX, 162, 150, 20, COLOR_BG_CARD);
    gfx->setTextSize(2);
    gfx->setTextColor(gasColor);
    gfx->setCursor(valueX, 162);
    if (data->gasDelta > 0) gfx->print("+");
    gfx->print(data->gasDelta);
    
    // ===== Shelf Life =====
    gfx->fillRect(valueX, 192, 150, 20, COLOR_BG_CARD);
    gfx->setTextSize(2);
    gfx->setTextColor(stageColor);
    gfx->setCursor(valueX, 192);
    if (remainDays >= 0) {
        gfx->print(remainDays);
        gfx->setTextColor(COLOR_TEXT_SECONDARY);
        gfx->print(" days");
    } else {
        gfx->print("Expired");
    }
    
    // ===== Score =====
    gfx->fillRect(valueX, 222, 150, 20, COLOR_BG_CARD);
    gfx->setTextSize(2);
    gfx->setTextColor(stageColor);
    gfx->setCursor(valueX, 222);
    gfx->print((int)score);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    gfx->print("/100");
    
    // ===== Storage（右下角）=====
    uint16_t storColor = getStorageColor(storageQuality);
    gfx->fillRect(365, 222, 100, 20, COLOR_BG_CARD);
    gfx->setTextSize(2);
    gfx->setTextColor(storColor);
    gfx->setCursor(365, 222);
    gfx->print(storageQuality);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    gfx->print("%");
}

// ==================== 水果测试界面（模式B）====================
void UIManager::showFruitTestResult(FruitType fruit, bool isSpoiled) {
    Serial.println("   Showing fruit test result...");
    
    String fruitName = FruitDatabase::getTypeName(fruit);
    String fruitEmoji = FruitDatabase::getEmoji(fruit);
    
    // 整屏背景颜色
    uint16_t bgColor = isSpoiled ? COLOR_DANGER : COLOR_VERY_FRESH;
    gfx->fillScreen(bgColor);
    
    // ===== 顶部水果卡片 =====
    drawCard(80, 30, SCREEN_WIDTH-160, 60, COLOR_BG_DARK);
    
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    gfx->setCursor(100, 45);
    gfx->print(fruitEmoji);
    gfx->print("  ");
    gfx->print(fruitName);
    
    // ===== 中央大图标 =====
    gfx->setTextSize(10);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    
    if (isSpoiled) {
        drawCenteredText("X", 110, COLOR_TEXT_PRIMARY, 10);
    } else {
        drawCenteredText("", 110, COLOR_TEXT_PRIMARY, 10);
    }
    
    // ===== 中央消息卡片 =====
    drawCard(60, 200, SCREEN_WIDTH-120, 60, COLOR_BG_DARK);
    
    gfx->setTextSize(4);
    gfx->setTextColor(bgColor);
    
    if (isSpoiled) {
        drawCenteredText("DO NOT EAT", 215, bgColor, 4);
    } else {
        drawCenteredText("OK TO EAT", 215, bgColor, 4);
    }
    
    // ===== 底部按钮提示 =====
    drawCard(30, 280, 200, 30, COLOR_BG_DARK);
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_WARNING);
    gfx->setCursor(40, 290);
    gfx->print(" Yellow: Exit Test");
    
    drawCard(250, 280, 200, 30, COLOR_BG_DARK);
    gfx->setTextColor(COLOR_VERY_FRESH);
    gfx->setCursor(260, 290);
    gfx->print(" Green: Test Again");
    
    Serial.println("   ✓ Test result shown");
}

// ==================== 返回提示界面（新增）====================
void UIManager::showReturnPrompt() {
    gfx->fillScreen(COLOR_BG_DARK);
    
    // 顶部卡片
    drawCard(40, 80, SCREEN_WIDTH-80, 160, COLOR_BG_CARD);
    
    // 图标
    gfx->setTextSize(5);
    gfx->setTextColor(COLOR_PRIMARY);
    drawCenteredText("", 100, COLOR_PRIMARY, 5);
    
    // 标题
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    drawCenteredText("Returning to", 150, COLOR_TEXT_PRIMARY, 3);
    drawCenteredText("Environment Mode", 180, COLOR_TEXT_PRIMARY, 3);
    
    // 提示信息
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    drawCenteredText("For best results:", 220, COLOR_TEXT_SECONDARY, 1);
    drawCenteredText("Wait 1-2 mins before next test", 240, COLOR_TEXT_SECONDARY, 1);
}

// ==================== 切换动画 ====================
void UIManager::showFruitSwitchAnimation(FruitType newFruit) {
    String fruitEmoji = FruitDatabase::getEmoji(newFruit);
    String fruitName = FruitDatabase::getTypeName(newFruit);
    
    gfx->fillScreen(COLOR_BG_DARK);
    
    // 顶部装饰
    gfx->fillRoundRect(0, 0, SCREEN_WIDTH, 80, 0, COLOR_PRIMARY);
    gfx->fillRoundRect(0, 75, SCREEN_WIDTH, 5, 0, COLOR_ACCENT);
    
    // emoji
    gfx->setTextSize(6);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    drawCenteredText(fruitEmoji.c_str(), 100, COLOR_TEXT_PRIMARY, 6);
    
    // 名称
    gfx->setTextSize(4);
    gfx->setTextColor(COLOR_ACCENT);
    drawCenteredText(fruitName.c_str(), 180, COLOR_ACCENT, 4);
    
    // 提示
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT_SECONDARY);
    drawCenteredText("Switching...", 240, COLOR_TEXT_SECONDARY, 1);
    
    // 装饰点
    for (int i = 0; i < 5; i++) {
        int x = (SCREEN_WIDTH / 2) - 40 + (i * 20);
        gfx->fillCircle(x, 270, 5, COLOR_PRIMARY);
    }
}

// ==================== 变坏警告（顶部红条）====================
void UIManager::showSpoilageWarning() {
    // 顶部红色警告条
    gfx->fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_DANGER);
    
    // 警告图标
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    gfx->setCursor(15, 8);
    gfx->print("!");
    
    // 警告文字
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT_PRIMARY);
    drawCenteredText("! FRUIT SPOILED !", 8, COLOR_TEXT_PRIMARY, 2);
    
    // 右侧图标
    gfx->setCursor(SCREEN_WIDTH - 35, 8);
    gfx->print("!");
}

// ==================== 上传状态 ====================
void UIManager::showUploadStatus(bool success) {
    uint16_t color = success ? COLOR_VERY_FRESH : COLOR_DANGER;
    const char* message = success ? "Upload OK" : "Failed";
    
    int boxWidth = 140;
    int boxX = (SCREEN_WIDTH - boxWidth) / 2;
    int boxY = 282;
    
    // 状态卡片
    gfx->fillRoundRect(boxX, boxY, boxWidth, 28, 5, color);
    
    // 图标
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_BG_DARK);
    gfx->setCursor(boxX + 10, boxY + 10);
    gfx->print(success ? "" : "");
    
    // 文字
    gfx->setCursor(boxX + 30, boxY + 10);
    gfx->print(message);
}

// ==================== 辅助函数 ====================

// 绘制卡片
void UIManager::drawCard(int x, int y, int w, int h, uint16_t bgColor) {
    // 阴影
    gfx->fillRoundRect(x + 2, y + 2, w, h, 6, COLOR_SHADOW);
    // 主体
    gfx->fillRoundRect(x, y, w, h, 6, bgColor);
    // 边框
    gfx->drawRoundRect(x, y, w, h, 6, COLOR_BORDER);
}

// 绘制进度条
void UIManager::drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color) {
    gfx->fillRoundRect(x, y, w, h, h/2, COLOR_BG_LIGHT);
    gfx->drawRoundRect(x, y, w, h, h/2, COLOR_BORDER);
    
    int fillWidth = (w - 4) * percent / 100;
    if (fillWidth > 0) {
        gfx->fillRoundRect(x + 2, y + 2, fillWidth, h - 4, (h-4)/2, color);
    }
}

// 绘制居中文字
void UIManager::drawCenteredText(const char* text, int y, uint16_t color, int textSize) {
    gfx->setTextSize(textSize);
    gfx->setTextColor(color);
    int textWidth = strlen(text) * 6 * textSize;
    gfx->setCursor((SCREEN_WIDTH - textWidth) / 2, y);
    gfx->print(text);
}

// 获取阶段颜色
uint16_t UIManager::getStageColor(FreshnessStage stage) {
    switch (stage) {
        case STAGE_VERY_FRESH: return COLOR_VERY_FRESH;  // 绿
        case STAGE_GOOD:       return COLOR_GOOD;        // 青
        case STAGE_EAT_TODAY:  return COLOR_WARNING;     // 橙
        case STAGE_SPOILED:    return COLOR_DANGER;      // 红
        default:               return COLOR_TEXT_SECONDARY;
    }
}

// 获取阶段名称
const char* UIManager::getStageName(FreshnessStage stage) {
    switch (stage) {
        case STAGE_VERY_FRESH: return "VERY FRESH";
        case STAGE_GOOD:       return "GOOD";
        case STAGE_EAT_TODAY:  return "EAT TODAY";
        case STAGE_SPOILED:    return "SPOILED";
        default:               return "UNKNOWN";
    }
}

// 获取Storage颜色
uint16_t UIManager::getStorageColor(int quality) {
    if (quality >= 80) return COLOR_VERY_FRESH;   // 绿
    if (quality >= 60) return COLOR_GOOD;         // 青
    if (quality >= 40) return COLOR_WARNING;      // 橙
    return COLOR_DANGER;                          // 红
}

// 获取Gas颜色
uint16_t UIManager::getGasColor(int gasDelta) {
    if (gasDelta > 50)  return COLOR_DANGER;      // 红
    if (gasDelta > 30)  return COLOR_WARNING;     // 橙
    if (gasDelta > -10) return COLOR_GOOD;        // 青/绿
    return COLOR_VERY_FRESH;                      // 更绿
}
