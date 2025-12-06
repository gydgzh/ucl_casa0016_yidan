/*
 * Sensors Implementation
 */

#include "sensors.h"

// 构造函数
Sensors::Sensors() : dht(DHT_PIN, DHT_TYPE) {
    gasBaseline = 0;
    calibrationSamples = 0;
    calibrationSum = 0;
}

// 初始化传感器
void Sensors::begin() {
    dht.begin();
    pinMode(MQ_PIN, INPUT);
}

// 读取所有传感器
SensorData Sensors::readSensors() {
    SensorData data;
    
    // 读取DHT22
    data.temperature = dht.readTemperature();
    data.humidity = dht.readHumidity();
    
    // 读取MQ-135
    data.gasRaw = analogRead(MQ_PIN);
    data.gasBaseline = gasBaseline;
    data.gasDelta = data.gasRaw - gasBaseline;
    
    // 检查数据有效性
    data.valid = !isnan(data.temperature) && !isnan(data.humidity);
    
    return data;
}

// 校准气体传感器
void Sensors::calibrateGasSensor() {
    int reading = analogRead(MQ_PIN);
    calibrationSum += reading;
    calibrationSamples++;
    
    // 计算平均值作为基准
    gasBaseline = calibrationSum / calibrationSamples;
}

// 获取气体基准值
int Sensors::getGasBaseline() {
    return gasBaseline;
}
