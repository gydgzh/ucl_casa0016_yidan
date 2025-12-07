/*
 * UI Manager Header - v3.0 现代化设计
 * 
 * 设计原则：
 * - 完全保持原有逻辑不变
 * - 只优化视觉效果和布局
 * - 现代化配色和卡片设计
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "fruit_profiles.h"
#include "sensors.h"
#include "freshness_model.h"

// ==================== TFT引脚配置 ====================
#define TFT_CS    7
#define TFT_DC    6
#define TFT_RST   4
#define TFT_MOSI  8
#define TFT_SCK   9
#define TFT_MISO  10

// ==================== TFT分辨率配置 ====================
#ifndef TFT_DRIVER
#define TFT_DRIVER 1
#endif

#if TFT_DRIVER == 1
  #define SCREEN_WIDTH  480
  #define SCREEN_HEIGHT 320
#elif TFT_DRIVER == 2
  #define SCREEN_WIDTH  320
  #define SCREEN_HEIGHT 240
#elif TFT_DRIVER == 3
  #define SCREEN_WIDTH  480
  #define SCREEN_HEIGHT 320
#endif

// ==================== 现代化配色方案 ====================
// 深色主题
#define COLOR_BG_DARK      0x1082  // 深蓝灰 #102030
#define COLOR_BG_CARD      0x2124  // 卡片背景 #202040
#define COLOR_BG_LIGHT     0x2945  // 浅背景 #293050

// 主色调
#define COLOR_PRIMARY      0x4A9F  // 现代蓝 #4080FF
#define COLOR_ACCENT       0x07FF  // 青色强调

// 文字颜色
#define COLOR_TEXT_PRIMARY    0xFFFF  // 白色
#define COLOR_TEXT_SECONDARY  0xAD55  // 灰色
#define COLOR_TEXT_DIM        0x7BEF  // 暗灰

// 状态颜色（柔和色调）
#define COLOR_VERY_FRESH   0x07E8  // 柔和绿
#define COLOR_GOOD         0x05FF  // 青绿
#define COLOR_WARNING      0xFD60  // 柔和橙
#define COLOR_DANGER       0xF9A6  // 柔和红

// 边框和阴影
#define COLOR_BORDER       0x4208
#define COLOR_SHADOW       0x0841

// ==================== UIManager类 ====================
class UIManager {
public:
    UIManager();
    
    void begin();
    void testDisplay();
    
    // 启动流程界面
    void showBootScreen();
    void showCalibrationScreen();
    void updateCalibrationProgress(int percent);
    void showLoRaJoiningScreen();
    void showErrorScreen(const char* message);
    
    // 环境监测界面（模式A）
    void showMonitoringScreen(FruitType fruit);
    void updateMonitoringData(FruitType fruit, const SensorData* data,
                             float score, int remainDays,
                             FreshnessStage stage, int storageQuality);
    
    // 水果测试界面（模式B）
    void showFruitTestResult(FruitType fruit, bool isSpoiled);
    
    // 返回提示界面（新增）
    void showReturnPrompt();
    
    // 动画和状态提示
    void showFruitSwitchAnimation(FruitType newFruit);
    void showSpoilageWarning();
    void showUploadStatus(bool success);

private:
    Arduino_DataBus* bus;
    Arduino_GFX* gfx;
    
    // 辅助绘图函数
    void drawProgressBar(int x, int y, int w, int h, int percent, uint16_t color);
    void drawCenteredText(const char* text, int y, uint16_t color, int textSize);
    void drawCard(int x, int y, int w, int h, uint16_t bgColor);
    
    // 颜色获取
    uint16_t getStageColor(FreshnessStage stage);
    const char* getStageName(FreshnessStage stage);
    uint16_t getStorageColor(int quality);
    uint16_t getGasColor(int gasDelta);
};

#endif
