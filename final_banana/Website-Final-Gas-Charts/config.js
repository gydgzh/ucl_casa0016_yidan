/*
 * =============================================================================
 * TTN Configuration for Fruit Freshness Monitor v3.5
 * =============================================================================
 * 
 * 填写你的 TTN 信息
 */

const CONFIG = {
    // TTN Base URL（根据你的region）
    // 例如：https://gydgzh2025.eu2.cloud.thethings.industries
    TTN_BASE_URL: 'https://gydgzh2025.eu2.cloud.thethings.industries',

    // Application ID
    // 在TTN Console里可以看到，例如：banana-freshness-monitor
    TTN_APP_ID: 'banana-freshness-monitor',

    // API Key（需要 RIGHT_APPLICATION_TRAFFIC_READ 权限）
    // 在TTN Console → Applications → API Keys 里生成
    TTN_API_KEY: 'NNSXS.FJSOI5WO52NJOEU5DTRTRIKZQOVOAPIENOZYLWQ.M4CDY3OEEXZ6WTFATZJQ2E3IDZZJFVH4XQNNHC7XQYXXWYNCS2JQ',

    // Region（仅用于显示）
    TTN_REGION: 'eu2',

    // Device ID（实际使用的设备）
    DEVICE_ID: 'banana2',

    // 图表历史点数（最多显示多少个数据点）
    MAX_HISTORY_POINTS: 100,

    // 自动刷新设置
    AUTO_REFRESH: true,
    REFRESH_INTERVAL: 60000, // 60秒刷新一次

    // 调试模式（会在浏览器Console打印详细日志）
    DEBUG: true
};

// =============================================================================
// 配置说明
// =============================================================================
/*
 * 如何获取API Key:
 * 
 * 1. 登录 TTN Console
 * 2. 进入你的 Application
 * 3. 左侧菜单 → API Keys
 * 4. 点击 "Add API Key"
 * 5. 名称随意，例如：website-access
 * 6. Rights 选择：Grant individual rights
 * 7. 勾选：Read application traffic (uplink and downlink)
 * 8. 点击 Create API Key
 * 9. 复制生成的 Key（NNSXS开头）
 * 10. 粘贴到上面的 TTN_API_KEY
 * 
 * ⚠️ 重要：API Key只显示一次，请妥善保存！
 * 
 * 如何找到 Device ID:
 * 
 * 1. TTN Console → Applications → 你的应用
 * 2. 点击 End Devices
 * 3. 看到的设备名称就是 Device ID
 *    例如：banana2, banana-monitor-001 等
 * 
 * 如何确定 Base URL:
 * 
 * 1. 看浏览器地址栏
 * 2. 例如：https://gydgzh2025.eu2.cloud.thethings.industries/console/...
 * 3. 取前面部分：https://gydgzh2025.eu2.cloud.thethings.industries
 * 4. Region 就是 eu2（或 nam1, au1 等）
 * 
 * 数据格式（Arduino发送的13字节payload）:
 * 
 * Byte 0:      fruitType (0=Banana, 1=Orange)
 * Byte 1-2:    temperature (int16, ×100)
 * Byte 3-4:    humidity (uint16, ×100)
 * Byte 5-6:    gasRaw (uint16)
 * Byte 7-8:    gasDelta (int16)
 * Byte 9:      score (uint8)
 * Byte 10:     remainingDays (uint8)
 * Byte 11:     stage (uint8)
 * Byte 12:     runtime (uint8, hours)
 * 
 * TTN会自动解码这些数据，网页直接读取decoded_payload即可。
 */
