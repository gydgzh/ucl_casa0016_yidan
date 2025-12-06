/*
 * Fruit Profiles - 水果参数配置
 * 基于科学文献的水果存储标准
 */

#ifndef FRUIT_PROFILES_H
#define FRUIT_PROFILES_H

#include <Arduino.h>

// 水果类型枚举
enum FruitType {
    FRUIT_BANANA = 0,   // 香蕉
    FRUIT_ORANGE = 1,   // 橘子
    FRUIT_APPLE = 2,    // 苹果
    FRUIT_GRAPE = 3     // 葡萄
};

// 水果配置参数结构
struct FruitProfile {
    const char* name;           // 水果名称
    const char* emoji;          // 表情符号
    
    // 最佳存储条件 (Kader, 2002)
    float minTemp;              // 最低温度 (°C)
    float maxTemp;              // 最高温度 (°C)
    float minHumidity;          // 最低湿度 (%)
    float maxHumidity;          // 最高湿度 (%)
    
    // 气体敏感度 (Saltveit, 1999)
    float gasThreshold;         // 气体阈值系数
    
    // 新鲜度评分系数
    float tempDecayCoeff;       // 温度衰减系数
    float humidDecayCoeff;      // 湿度衰减系数
    float gasDecayCoeff;        // 气体衰减系数
    float timeDecayCoeff;       // 时间衰减系数 (分/小时)
    
    // 初始评分和预期寿命
    float initialScore;         // 初始评分
    int expectedLifeDays;       // 预期寿命 (天)
};

// 水果数据库类
class FruitDatabase {
public:
    static const FruitProfile& getProfile(FruitType type);
    static String getTypeName(FruitType type);
    static String getEmoji(FruitType type);
    
private:
    static const FruitProfile profiles[4];
};

#endif
