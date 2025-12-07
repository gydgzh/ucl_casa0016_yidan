/*
 * TTN Configuration
 * 填写你的 TTN 信息
 */

const CONFIG = {
    // 你的 tenant 域名（看浏览器地址栏）
    // 现在是：https://gydgzh2025.eu2.cloud.thethings.industries/console/...
    TTN_BASE_URL: 'https://gydgzh2025.eu2.cloud.thethings.industries',

    // Application ID（Console 里看到的那个：banana-freshness-monitor）
    TTN_APP_ID: 'banana-freshness-monitor',

    // 有 RIGHT_APPLICATION_TRAFFIC_READ 权限的 API Key
    //（就是你现在在 Console 里那条很长的 NNSXS...）
    TTN_API_KEY: 'NNSXS.FJSOI5WO52NJOEU5DTRTRIKZQOVOAPIENOZYLWQ.M4CDY3OEEXZ6WTFATZJQ2E3IDZZJFVH4XQNNHC7XQYXXWYNCS2JQ',

    // 只是用来在网页底部显示，写 eu2 就行
    TTN_REGION: 'eu2',

    // 现在实际在用的设备 ID（你刚注册的那个）
    DEVICE_ID: 'banana2',

    // 图表最多显示多少个历史点
    MAX_HISTORY_POINTS: 100,

    // 自动刷新设置
    AUTO_REFRESH: true,
    REFRESH_INTERVAL: 60000, // 每 60 秒刷新一次

    // 调试模式：true 时会在浏览器 Console 打印很多日志
    DEBUG: true
};

