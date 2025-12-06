/*
 * UI Manager - 2按钮版本
 * 简化界面：只有监测界面，无选择界面
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "fruit_profiles.h"
#include "sensors.h"
#include "freshness_model.h"

// TFT引脚定义（软件SPI）
#define TFT_CS      7
#define TFT_DC      6
#define TFT_RST     4
#define TFT_MOSI    8
#define TFT_SCK     9
#define TFT_MISO    10

// 颜色定义
#define COLOR_BG        0x0000  // 黑色背景
#define COLOR_TEXT      0xFFFF  // 白色文字
#define COLOR_TITLE     0x07FF  // 青色标题
#define COLOR_FRESH     0x07E0  // 绿色 (VERY FRESH)
#define COLOR_GOOD      0x07FF  // 青色 (GOOD)
#define COLOR_WARNING   0xFD20  // 橙色 (EAT TODAY)
#define COLOR_DANGER    0xF800  // 红色 (SPOILED)

class UIManager {
public:
    UIManager();
    
    void begin();
    void showBootScreen();
    void showCalibrationScreen();
    void updateCalibrationProgress(int percent);
    void showLoRaJoiningScreen();
    void showErrorScreen(const char* message);
    
    // 显示监测界面
    void showMonitoringScreen(FruitType fruit, const SensorData* data, 
                             float score = 0, int remainDays = 0,
                             FreshnessStage stage = STAGE_GOOD, 
                             int storageQuality = 0);
    
    // 显示水果切换动画
    void showFruitSwitchAnimation(FruitType newFruit);
    
    // 显示变坏警告
    void showSpoilageWarning();
    
    // 显示上传状态
    void showUploadStatus(bool success);
    
private:
    Arduino_DataBus *bus;
    Arduino_GFX *gfx;
    
    void drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color);
    void drawCenteredText(const char* text, int y, uint16_t color, int textSize);
    uint16_t getStageColor(FreshnessStage stage);
    const char* getStageName(FreshnessStage stage);
};

#endif
