# ucl_casa0016_yidan
# 🍌 Fruit – Fruit Freshness Monitoring System

* by Yidan Gao

##  Overview

**Fruit** is a real-time monitoring system that estimates:

- How many days fresh fruit can be stored under current environmental conditions
- The freshness level of a specific piece of fruit

The system supports multiple fruit types (e.g., banana and orange) and includes two modules:

1. **Catbus Fruit Detector**

<img width="3072" height="2582" alt="主题" src="https://github.com/user-attachments/assets/ad4e9826-1072-49ba-8dc8-df02dc59a206" />


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

  ![IMG_6391](https://github.com/user-attachments/assets/a7c7b626-8b2f-4349-a3fe-43a51c2a18d5)


------

## Startup Process

When powered on, the device:

1. Calibrates the gas sensor (≈10 seconds)

2. Joins the LoRaWAN network

3. Enters Environment Monitoring Mode

![未命名作品 4](https://github.com/user-attachments/assets/df3eb18c-4fc9-4ffc-b9f4-09a92e8afa31)


------

## Environment Monitoring Mode

The device continuously:

- Reads **temperature**, **humidity**, and **air quality**

- Estimates **how many days fruit can stay fresh**

- Computes a **storage quality score** (fridges usually score higher than rooms)
 <img width="820" height="254" alt="image" src="https://github.com/user-attachments/assets/bcdf3428-ae31-49b6-b1a6-a5a4de2de06b" />
![IMG_0827](https://github.com/user-attachments/assets/b129566c-d23d-438f-9ac7-2ebd10a62fe7)


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
<img width="1090" height="720" alt="image" src="https://github.com/user-attachments/assets/ac088308-32df-4f17-94dd-32e14328cb2a" />

  
- **Red screen** → Bad fruit detected based on gas change
<img width="1073" height="720" alt="image" src="https://github.com/user-attachments/assets/a41a4475-908e-4a24-852b-cc6be188774a" />


Press the **yellow button** again to return to Environment Monitoring Mode.
 The device will recalibrate the gas sensor before continuing.

<img width="1156" height="720" alt="image" src="https://github.com/user-attachments/assets/13266d6c-0dab-4bbb-abb4-05d5397f9a15" />

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

<img width="855" height="633" alt="image" src="https://github.com/user-attachments/assets/aeb97ce2-0346-452c-9318-17b1132d1a0b" />

<img width="851" height="442" alt="image" src="https://github.com/user-attachments/assets/4ec1585b-6fb2-466a-8034-842862c09e10" />

### **2. Gas Composition Estimation**

Explains gases related to spoilage (NH₃, NOₓ, alcohol, benzene, CO₂, smoke).
 Inclu des:

- Radar chart of estimated gas composition

- Gas history timeline
 <img width="816" height="561" alt="image" src="https://github.com/user-attachments/assets/865482f0-f4cc-466b-be65-8337a65d0c5d" />

### **3. Historical Data Line Chart**

Visualizes:

- Temperature

- Humidity

- Gas data

- Freshness score trends
 <img width="856" height="390" alt="image" src="https://github.com/user-attachments/assets/1125e03c-bc0d-42f4-8b32-a329382a503a" />
 <img width="853" height="378" alt="image" src="https://github.com/user-attachments/assets/c1e2f696-aec0-4ff4-9e5b-7983933ce2f5" />


Helps users observe long-term patterns of fruit ripening and spoilage.

<img width="854" height="487" alt="image" src="https://github.com/user-attachments/assets/99b63f58-34dc-4c82-8b7f-83806ca4c48a" />


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

