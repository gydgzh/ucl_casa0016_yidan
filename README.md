# ucl_casa0016_yidan
# 🍌 Fruit – Fruit Freshness Monitoring System

* by Yidan Gao

##  Overview

**Fruit** is a real-time monitoring system that estimates:

- How many days fresh fruit can be stored under current environmental conditions
- The freshness level of a specific piece of fruit

The system supports multiple fruit types (e.g., banana and orange) and includes two modules:

1. **Catbus Fruit Detector**

   ![image-20251210142315668](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210142315668.png)

2. **Web Monitoring System**

------

# 1. Catbus Fruit Detector

A physical LoRa-enabled device used for fruit and environment monitoring.

## Hardware Components

- DHT22 – temperature & humidity sensor
- MQ-135 – gas sensor for spoilage-related gases
- TFT Display – visual interface
- Two buttons – yellow (fruit type) and green (fruit testing)
- MKR WAN 1310 – LoRaWAN communication

------

## Startup Process

When powered on, the device:

1. Calibrates the gas sensor (≈10 seconds)

2. Joins the LoRaWAN network

3. Enters Environment Monitoring Mode

   ![未命名作品 4](/Users/yimisheng/Downloads/未命名作品 4.jpg)

------

## Environment Monitoring Mode

The device continuously:

- Reads **temperature**, **humidity**, and **air quality**

- Estimates **how many days fruit can stay fresh**

- Computes a **storage quality score** (fridges usually score higher than rooms)

  ![image-20251210142803127](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210142803127.png)

  ![IMG_0827](/Users/yimisheng/Downloads/IMG_0827.jpg)

### 🟡 Yellow Button

Switches fruit type (e.g., banana ↔ orange)

### Data Update Cycle

- Local display refresh: every 2 seconds
- LoRaWAN upload to TTN: every 5 minutes

------

## Fruit Testing Mode

### 🟢 Green Button

Activated by pressing the **green button**.

Used to check the freshness of **one single fruit** of the selected type.

### Display Logic

- **Green screen** → No fruit nearby or the fruit is healthy

  ![image-20251210142348108](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210142348108.png)

- **Red screen** → Bad fruit detected based on gas change

  ![image-20251210142358478](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210142358478.png)

Press the **yellow button** again to return to Environment Monitoring Mode.
 The device will recalibrate the gas sensor before continuing.

![image-20251210142433597](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210142433597.png)

------

# 2. Web Monitoring System

A webpage used by TTN Storage and TTN API.

## Data Flow

```
Device → TTN (every 5 min)
TTN Storage → Stores latest 100 records
Web Dashboard → Fetches data via TTN API
```

------

# Fruit Freshness Monitor

### **1. Freshness Score**

Shows:

- Freshness stage (Very Fresh / Good / Eat Today / Spoiled)

- Storage quality

- Predicted shelf life

  ![image-20251210143021752](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210143021752.png)

![image-20251210143209557](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210143209557.png)

### **2. Gas Composition Estimation**

Explains gases related to spoilage (NH₃, NOₓ, alcohol, benzene, CO₂, smoke).
 Inclu des:

- Radar chart of estimated gas composition

- Gas history timeline

  ![image-20251210143116996](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210143116996.png)

### **3. Historical Data Line Chart**

Visualizes:

- Temperature

- Humidity

- Gas data

- Freshness score trends

  ![image-20251210143320470](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210143320470.png)

Helps users observe long-term patterns of fruit ripening and spoilage.

![image-20251210143349955](/Users/yimisheng/Library/Application Support/typora-user-images/image-20251210143349955.png)

------

# System Summary

- Full IoT workflow: sensors → LoRaWAN → cloud → dashboard
- Estimates both **storage suitability** and **freshness**
- Supports **multi-fruit operation**
- Includes interactive TFT UI and data analytics
- Designed for simple everyday use or research on fruit storage behavior

------

# Author

**Yidan Gao**
 GitHub Repository: https://github.com/gydgzh/ucl_casa0016_yidan 

