/*
 * Freshness Model Implementation
 */

#include "freshness_model.h"

// 构造函数
FreshnessModel::FreshnessModel() {
    currentFruit = FRUIT_BANANA;
    profile = &FruitDatabase::getProfile(FRUIT_BANANA);
    currentScore = 100.0;
    startTime = millis();
}

// 设置水果类型
void FreshnessModel::setFruitType(FruitType type) {
    currentFruit = type;
    profile = &FruitDatabase::getProfile(type);
    currentScore = profile->initialScore;
    startTime = millis();  // 重置开始时间
}

// 更新读数并计算评分
void FreshnessModel::updateReadings(float temperature, float humidity, int gasDelta) {
    currentScore = calculateScore(temperature, humidity, gasDelta);
}

// 计算新鲜度评分
float FreshnessModel::calculateScore(float temperature, float humidity, int gasDelta) {
    float score = profile->initialScore;
    
    // 1. 温度影响
    float optimalTemp = (profile->minTemp + profile->maxTemp) / 2.0;
    float tempDeviation = abs(temperature - optimalTemp);
    score -= tempDeviation * profile->tempDecayCoeff;
    
    // 2. 湿度影响
    float optimalHumidity = (profile->minHumidity + profile->maxHumidity) / 2.0;
    float humidDeviation = abs(humidity - optimalHumidity);
    score -= humidDeviation * profile->humidDecayCoeff;
    
    // 3. 气体影响 (只考虑正值变化)
    if (gasDelta > 0) {
        score -= gasDelta * profile->gasDecayCoeff;
    }
    
    // 4. 时间衰减
    float ageHours = (millis() - startTime) / 3600000.0;
    score -= ageHours * profile->timeDecayCoeff;
    
    // 限制范围 0-100
    return max(0.0f, min(100.0f, score));
}

// 获取当前评分
float FreshnessModel::getScore() {
    return currentScore;
}

// 计算剩余天数
int FreshnessModel::getRemainingDays() {
    if (currentScore <= 0) return -1;
    
    // 基于当前评分和预期寿命的比例
    float ratio = currentScore / 100.0;
    int days = (int)(ratio * profile->expectedLifeDays);
    
    return days;
}

// 获取新鲜度阶段
FreshnessStage FreshnessModel::getStage() {
    if (currentScore >= 80.0) return STAGE_VERY_FRESH;
    if (currentScore >= 60.0) return STAGE_GOOD;
    if (currentScore >= 40.0) return STAGE_EAT_TODAY;
    return STAGE_SPOILED;
}

// 计算存储环境质量评分 (⭐ 创新功能)
int FreshnessModel::calculateStorageScore(float temperature, float humidity) {
    int score = 100;
    
    // 温度偏离评分
    if (temperature < profile->minTemp) {
        score -= (profile->minTemp - temperature) * 5;
    } else if (temperature > profile->maxTemp) {
        score -= (temperature - profile->maxTemp) * 5;
    }
    
    // 湿度偏离评分
    if (humidity < profile->minHumidity) {
        score -= (profile->minHumidity - humidity) * 2;
    } else if (humidity > profile->maxHumidity) {
        score -= (humidity - profile->maxHumidity) * 2;
    }
    
    // 限制范围 0-100
    return max(0, min(100, score));
}
