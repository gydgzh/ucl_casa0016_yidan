/*
 * Freshness Model - 新鲜度评估模型
 */

#ifndef FRESHNESS_MODEL_H
#define FRESHNESS_MODEL_H

#include <Arduino.h>
#include "fruit_profiles.h"

// 新鲜度阶段
enum FreshnessStage {
    STAGE_VERY_FRESH = 0,   // 80-100分
    STAGE_GOOD = 1,         // 60-79分
    STAGE_EAT_TODAY = 2,    // 40-59分
    STAGE_SPOILED = 3       // <40分
};

// 新鲜度模型类
class FreshnessModel {
public:
    FreshnessModel();
    
    void setFruitType(FruitType type);
    void updateReadings(float temperature, float humidity, int gasDelta);
    
    float getScore();
    int getRemainingDays();
    FreshnessStage getStage();
    int calculateStorageScore(float temperature, float humidity);
    
private:
    FruitType currentFruit;
    const FruitProfile* profile;
    
    float currentScore;
    unsigned long startTime;
    
    float calculateScore(float temperature, float humidity, int gasDelta);
};

#endif
