/*
 * Fruit Profiles Implementation
 */

#include "fruit_profiles.h"

// 水果配置数据库 (基于科学文献)
const FruitProfile FruitDatabase::profiles[4] = {
    // 香蕉 (Banana) - Kader (2002)
    {
        .name = "Banana",
        .emoji = "🍌",
        .minTemp = 18.0,
        .maxTemp = 22.0,
        .minHumidity = 60.0,
        .maxHumidity = 70.0,
        .gasThreshold = 50.0,
        .tempDecayCoeff = 3.0,
        .humidDecayCoeff = 2.0,
        .gasDecayCoeff = 0.15,
        .timeDecayCoeff = 0.6,    // 每小时0.6分
        .initialScore = 100.0,
        .expectedLifeDays = 7
    },
    
    // 橘子 (Orange) - Kader (2002)
    {
        .name = "Orange",
        .emoji = "🍊",
        .minTemp = 4.0,
        .maxTemp = 10.0,
        .minHumidity = 85.0,
        .maxHumidity = 90.0,
        .gasThreshold = 80.0,
        .tempDecayCoeff = 2.5,
        .humidDecayCoeff = 1.5,
        .gasDecayCoeff = 0.08,
        .timeDecayCoeff = 0.3,    // 每小时0.3分
        .initialScore = 100.0,
        .expectedLifeDays = 14
    },
    
    // 苹果 (Apple) - Kader (2002)
    {
        .name = "Apple",
        .emoji = "🍎",
        .minTemp = 0.0,
        .maxTemp = 4.0,
        .minHumidity = 90.0,
        .maxHumidity = 95.0,
        .gasThreshold = 60.0,
        .tempDecayCoeff = 2.0,
        .humidDecayCoeff = 1.5,
        .gasDecayCoeff = 0.10,
        .timeDecayCoeff = 0.2,    // 每小时0.2分
        .initialScore = 100.0,
        .expectedLifeDays = 30
    },
    
    // 葡萄 (Grape) - Kader (2002)
    {
        .name = "Grape",
        .emoji = "🍇",
        .minTemp = 0.0,
        .maxTemp = 2.0,
        .minHumidity = 90.0,
        .maxHumidity = 95.0,
        .gasThreshold = 70.0,
        .tempDecayCoeff = 2.5,
        .humidDecayCoeff = 2.0,
        .gasDecayCoeff = 0.12,
        .timeDecayCoeff = 0.8,    // 每小时0.8分
        .initialScore = 100.0,
        .expectedLifeDays = 10
    }
};

// 获取水果配置
const FruitProfile& FruitDatabase::getProfile(FruitType type) {
    return profiles[type];
}

// 获取水果名称
String FruitDatabase::getTypeName(FruitType type) {
    return String(profiles[type].name);
}

// 获取水果表情
String FruitDatabase::getEmoji(FruitType type) {
    return String(profiles[type].emoji);
}
