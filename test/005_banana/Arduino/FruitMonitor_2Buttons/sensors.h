/*
 * Sensors Module - 传感器模块
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <DHT.h>

// DHT22 配置
#define DHT_PIN     3       // D3 - ⚠️ 注意是D3不是D2
#define DHT_TYPE    DHT22

// MQ-135 配置
#define MQ_PIN      A0      // 模拟输入（需分压电路）

// 传感器数据结构
struct SensorData {
    float temperature;      // 温度 (°C)
    float humidity;         // 湿度 (%)
    int gasRaw;            // 气体原始值 (0-1023)
    int gasBaseline;       // 气体基准值
    int gasDelta;          // 气体变化量
    bool valid;            // 数据有效性
};

// 传感器类
class Sensors {
public:
    Sensors();
    void begin();
    SensorData readSensors();
    void calibrateGasSensor();
    int getGasBaseline();
    
private:
    DHT dht;
    int gasBaseline;
    int calibrationSamples;
    long calibrationSum;
};

#endif
