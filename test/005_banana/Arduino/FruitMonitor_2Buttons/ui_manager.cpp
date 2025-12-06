/*
 * UI Manager Implementation - 2按钮版本
 */

#include "ui_manager.h"

// 构造函数
UIManager::UIManager() {
    bus = NULL;
    gfx = NULL;
}

// 初始化TFT显示
void UIManager::begin() {
    // 创建软件SPI总线
    bus = new Arduino_SWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
    
    // 创建ILI9488驱动（18bit颜色）
    gfx = new Arduino_ILI9488_18bit(bus, TFT_RST);
    
    // 初始化显示
    gfx->begin();
    gfx->setRotation(1);  // 横屏
    gfx->fillScreen(COLOR_BG);
}

// 显示启动画面
void UIManager::showBootScreen() {
    gfx->fillScreen(COLOR_BG);
    gfx->setTextColor(COLOR_TITLE);
    gfx->setTextSize(3);
    
    int centerY = 120;
    drawCenteredText("Multi-Fruit", centerY - 40, COLOR_TITLE, 3);
    drawCenteredText("Freshness Monitor", centerY, COLOR_TEXT, 2);
    drawCenteredText("v2.0", centerY + 40, COLOR_GOOD, 2);
    
    gfx->setTextSize(1);
    drawCenteredText("Initializing...", centerY + 80, COLOR_TEXT, 1);
}

// 显示校准画面
void UIManager::showCalibrationScreen() {
    gfx->fillScreen(COLOR_BG);
    gfx->setTextColor(COLOR_WARNING);
    gfx->setTextSize(2);
    
    drawCenteredText("Calibrating", 80, COLOR_WARNING, 2);
    drawCenteredText("Gas Sensor", 110, COLOR_WARNING, 2);
    
    gfx->setTextSize(1);
    drawCenteredText("Please wait 10 seconds...", 160, COLOR_TEXT, 1);
}

// 更新校准进度
void UIManager::updateCalibrationProgress(int percent) {
    int barWidth = 300;
    int barHeight = 30;
    int barX = (480 - barWidth) / 2;
    int barY = 200;
    
    drawProgressBar(barX, barY, barWidth, barHeight, percent, COLOR_GOOD);
    
    // 显示百分比
    char buf[10];
    sprintf(buf, "%d%%", percent);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TEXT, COLOR_BG);
    int textWidth = strlen(buf) * 12;
    gfx->setCursor((480 - textWidth) / 2, barY + 50);
    gfx->print(buf);
}

// 显示LoRa连接画面
void UIManager::showLoRaJoiningScreen() {
    gfx->fillScreen(COLOR_BG);
    gfx->setTextSize(2);
    
    drawCenteredText("Connecting to", 100, COLOR_TITLE, 2);
    drawCenteredText("The Things Network", 130, COLOR_TITLE, 2);
    
    gfx->setTextSize(1);
    drawCenteredText("This may take a minute...", 180, COLOR_TEXT, 1);
}

// 显示错误画面
void UIManager::showErrorScreen(const char* message) {
    gfx->fillScreen(COLOR_BG);
    gfx->setTextSize(3);
    
    drawCenteredText("ERROR", 100, COLOR_DANGER, 3);
    
    gfx->setTextSize(2);
    drawCenteredText(message, 150, COLOR_TEXT, 2);
}

// 显示监测界面（主界面）
void UIManager::showMonitoringScreen(FruitType fruit, const SensorData* data,
                                     float score, int remainDays,
                                     FreshnessStage stage, int storageQuality) {
    gfx->fillScreen(COLOR_BG);
    
    // 获取水果信息
    String fruitName = FruitDatabase::getTypeName(fruit);
    String fruitEmoji = FruitDatabase::getEmoji(fruit);
    
    // 标题栏
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TITLE);
    gfx->setCursor(10, 10);
    gfx->print(fruitEmoji);
    gfx->print(" ");
    gfx->print(fruitName);
    gfx->print(" Monitor");
    
    // 分隔线
    gfx->drawFastHLine(10, 40, 460, COLOR_TITLE);
    
    if (data == NULL || !data->valid) {
        // 无数据时显示等待
        gfx->setTextSize(2);
        drawCenteredText("Waiting for data...", 160, COLOR_TEXT, 2);
        return;
    }
    
    // 新鲜度阶段（大号显示）
    const char* stageName = getStageName(stage);
    uint16_t stageColor = getStageColor(stage);
    
    gfx->fillRoundRect(90, 60, 300, 50, 10, stageColor);
    gfx->setTextSize(3);
    gfx->setTextColor(COLOR_BG);
    int textWidth = strlen(stageName) * 18;
    gfx->setCursor((480 - textWidth) / 2, 75);
    gfx->print(stageName);
    
    // 核心指标（3列）
    int col1X = 40;
    int col2X = 200;
    int col3X = 360;
    int rowY = 130;
    
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT);
    
    // 评分
    gfx->setCursor(col1X, rowY);
    gfx->print("Score");
    gfx->setTextSize(3);
    gfx->setTextColor(stageColor);
    gfx->setCursor(col1X, rowY + 20);
    gfx->print((int)score);
    gfx->setTextSize(2);
    gfx->print("/100");
    
    // 剩余天数
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(col2X, rowY);
    gfx->print("Days Left");
    gfx->setTextSize(3);
    gfx->setTextColor(stageColor);
    gfx->setCursor(col2X, rowY + 20);
    if (remainDays >= 0) {
        gfx->print(remainDays);
    } else {
        gfx->setTextSize(2);
        gfx->print("Expired");
    }
    
    // 存储质量
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(col3X, rowY);
    gfx->print("Storage");
    gfx->setTextSize(3);
    uint16_t qualityColor = storageQuality >= 80 ? COLOR_FRESH :
                           storageQuality >= 60 ? COLOR_GOOD :
                           storageQuality >= 40 ? COLOR_WARNING : COLOR_DANGER;
    gfx->setTextColor(qualityColor);
    gfx->setCursor(col3X, rowY + 20);
    gfx->print(storageQuality);
    gfx->setTextSize(2);
    gfx->print("/100");
    
    // 分隔线
    gfx->drawFastHLine(10, 180, 460, COLOR_TITLE);
    
    // 环境数据（2行x2列）
    int env1X = 40;
    int env2X = 260;
    int env1Y = 200;
    int env2Y = 250;
    
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT);
    
    // 温度
    gfx->setCursor(env1X, env1Y);
    gfx->print("Temperature:");
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TITLE);
    gfx->setCursor(env1X, env1Y + 18);
    gfx->print(data->temperature, 1);
    gfx->print(" C");
    
    // 湿度
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(env2X, env1Y);
    gfx->print("Humidity:");
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TITLE);
    gfx->setCursor(env2X, env1Y + 18);
    gfx->print(data->humidity, 1);
    gfx->print(" %");
    
    // 气体
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(env1X, env2Y);
    gfx->print("Gas Level:");
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_TITLE);
    gfx->setCursor(env1X, env2Y + 18);
    gfx->print(data->gasRaw);
    gfx->print(" ADC");
    
    // 气体变化
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(env2X, env2Y);
    gfx->print("Gas Change:");
    gfx->setTextSize(2);
    uint16_t deltaColor = data->gasDelta > 0 ? COLOR_DANGER : COLOR_GOOD;
    gfx->setTextColor(deltaColor);
    gfx->setCursor(env2X, env2Y + 18);
    if (data->gasDelta > 0) gfx->print("+");
    gfx->print(data->gasDelta);
    
    // 底部提示
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(10, 305);
    gfx->print("Yellow: Switch | Green: Refresh");
}

// 显示水果切换动画
void UIManager::showFruitSwitchAnimation(FruitType newFruit) {
    String fruitEmoji = FruitDatabase::getEmoji(newFruit);
    String fruitName = FruitDatabase::getTypeName(newFruit);
    
    gfx->fillScreen(COLOR_BG);
    
    gfx->setTextSize(4);
    gfx->setTextColor(COLOR_TITLE);
    int emojiWidth = fruitEmoji.length() * 24;
    gfx->setCursor((480 - emojiWidth) / 2, 100);
    gfx->print(fruitEmoji);
    
    gfx->setTextSize(3);
    drawCenteredText(fruitName.c_str(), 160, COLOR_TEXT, 3);
    
    gfx->setTextSize(1);
    drawCenteredText("Loading...", 220, COLOR_GOOD, 1);
}

// 显示变坏警告
void UIManager::showSpoilageWarning() {
    // 在屏幕顶部显示红色警告条
    gfx->fillRect(0, 0, 480, 30, COLOR_DANGER);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_BG);
    drawCenteredText("! FRUIT SPOILED !", 8, COLOR_BG, 2);
    
    // ⭐ 关键修改：去掉 delay(2000)，让程序不阻塞
    // delay(2000);  // ← 注释掉这一行
}

// 显示上传状态
void UIManager::showUploadStatus(bool success) {
    uint16_t color = success ? COLOR_GOOD : COLOR_DANGER;
    const char* message = success ? "Data Uploaded!" : "Upload Failed";
    
    gfx->fillRect(120, 280, 240, 35, color);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_BG);
    int textWidth = strlen(message) * 12;
    gfx->setCursor((480 - textWidth) / 2, 290);
    gfx->print(message);
}

// ==================== 辅助函数 ====================

// 绘制进度条
void UIManager::drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color) {
    // 边框
    gfx->drawRect(x, y, w, h, COLOR_TEXT);
    
    // 填充
    int fillWidth = (w - 4) * percent / 100;
    if (fillWidth > 0) {
        gfx->fillRect(x + 2, y + 2, fillWidth, h - 4, color);
    }
}

// 绘制居中文字
void UIManager::drawCenteredText(const char* text, int y, uint16_t color, int textSize) {
    gfx->setTextSize(textSize);
    gfx->setTextColor(color, COLOR_BG);
    int textWidth = strlen(text) * 6 * textSize;
    gfx->setCursor((480 - textWidth) / 2, y);
    gfx->print(text);
}

// 获取阶段颜色
uint16_t UIManager::getStageColor(FreshnessStage stage) {
    switch (stage) {
        case STAGE_VERY_FRESH: return COLOR_FRESH;
        case STAGE_GOOD:       return COLOR_GOOD;
        case STAGE_EAT_TODAY:  return COLOR_WARNING;
        case STAGE_SPOILED:    return COLOR_DANGER;
        default:               return COLOR_TEXT;
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
