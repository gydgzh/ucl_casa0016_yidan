// 全局变量
let currentFruit = 0;
let allData = [];
let filteredData = [];
let freshnessChart = null;
let environmentChart = null;

// 水果配置
const fruitConfig = {
    0: { name: 'Banana', emoji: '🍌', minTemp: 18, maxTemp: 22, minHumid: 60, maxHumid: 70 },
    1: { name: 'Orange', emoji: '🍊', minTemp: 4,  maxTemp: 10, minHumid: 85, maxHumid: 90 },
    2: { name: 'Apple',  emoji: '🍎', minTemp: 0,  maxTemp: 4,  minHumid: 90, maxHumid: 95 },
    3: { name: 'Grape',  emoji: '🍇', minTemp: 0,  maxTemp: 2,  minHumid: 90, maxHumid: 95 }
};

// 页面加载时初始化
document.addEventListener('DOMContentLoaded', function () {
    console.log('Initializing Fruit Monitor Dashboard...');

    document.getElementById('deviceId').textContent = CONFIG.DEVICE_ID;
    if (document.getElementById('region') && CONFIG.TTN_REGION) {
        document.getElementById('region').textContent = CONFIG.TTN_REGION;
    }

    initCharts();
    loadData();

    if (CONFIG.AUTO_REFRESH) {
        setInterval(loadData, CONFIG.REFRESH_INTERVAL);
    }
});

// 从 TTN Storage 加载数据
async function loadData() {
    if (CONFIG.DEBUG) console.log('Loading data from TTN Storage…');

    try {
        // 只取当前应用 + 当前设备的历史 uplink
        const url =
            `${CONFIG.TTN_BASE_URL}` +
            `/api/v3/as/applications/${CONFIG.TTN_APP_ID}` +
            `/devices/${CONFIG.DEVICE_ID}/packages/storage/uplink_message` +
            `?field_mask=up.uplink_message.decoded_payload`;

        const response = await fetch(url, {
            headers: {
                'Authorization': `Bearer ${CONFIG.TTN_API_KEY}`,
                'Accept': 'text/event-stream'
            }
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const text = await response.text();
        if (CONFIG.DEBUG) console.log('Raw response text:', text);

        const lines = text.trim().split('\n').filter(line => line.length > 0);

        allData = lines.map(line => {
            try {
                // 某些环境会返回 "data: {...}"，先把前缀去掉
                const cleaned = line.startsWith('data:') ? line.substring(5).trim() : line;
                const json = JSON.parse(cleaned);
                const result = json.result || json;  // 两种格式都兼容

                return {
                    timestamp: result.received_at,
                    data: result.uplink_message.decoded_payload
                };
            } catch (e) {
                console.error('Parse error for line:', line, e);
                return null;
            }
        }).filter(item => item !== null).reverse();  // 最新的在前

        if (CONFIG.DEBUG) {
            console.log(`Loaded ${allData.length} data points`);
            if (allData[0]) console.log('Latest record:', allData[0]);
        }

        filterDataByFruit(currentFruit);
        updateUI();
        updateLastUpdate();
    } catch (error) {
        console.error('Error loading data:', error);
        showError('Failed to load data from TTN. Please check your configuration.');
    }
}

// 按水果类型过滤数据
function filterDataByFruit(fruitType) {
    if (!allData || allData.length === 0) {
        filteredData = [];
        return;
    }

    filteredData = allData.filter(item => {
        const t = item.data.fruitType;

        // 如果 payload 里有 fruitType 字段，就按字段过滤
        if (typeof t === 'number') {
            return t === fruitType;
        }

        // 如果 payload 里没有 fruitType：视为“单水果系统”
        // 默认全部是香蕉（0），所以只在 Banana 标签下显示
        if (fruitType === 0) return true;
        return false;
    });

    if (CONFIG.DEBUG) {
        console.log(`Filtered ${filteredData.length} records for fruit ${fruitType}`);
    }
}

// 顶部按钮切换水果
function selectFruit(fruitType) {
    currentFruit = fruitType;

    document.querySelectorAll('.fruit-tab').forEach(tab => {
        tab.classList.remove('active');
    });
    const activeTab = document.querySelector(`[data-fruit="${fruitType}"]`);
    if (activeTab) activeTab.classList.add('active');

    filterDataByFruit(fruitType);
    updateUI();
}

// 更新界面
function updateUI() {
    if (!filteredData || filteredData.length === 0) {
        showNoData();
        return;
    }

    const latest = filteredData[0].data;

    // 标题
    document.getElementById('currentEmoji').textContent = fruitConfig[currentFruit].emoji;
    document.getElementById('currentFruit').textContent = fruitConfig[currentFruit].name;

    // 新鲜度阶段
    updateStageBadge(latest.stage);

    // 核心指标
    document.getElementById('scoreValue').textContent = latest.score;
    document.getElementById('daysValue').textContent =
        latest.remainDays >= 0 ? latest.remainDays : 'Expired';

    const storageQuality = calculateStorageQuality(latest);
    document.getElementById('storageValue').textContent = storageQuality;
    updateStorageBar(storageQuality);

    // 环境数据
    document.getElementById('tempValue').textContent = `${latest.temperature.toFixed(1)}°C`;
    document.getElementById('humidValue').textContent = `${latest.humidity.toFixed(1)}%`;
    document.getElementById('gasValue').textContent = `${latest.gasRaw} ADC`;
    document.getElementById('runtimeValue').textContent = `${latest.ageHours}h`;

    // 最佳条件 & 提示
    updateOptimalConditions(latest);

    // 图表 / 表格 / 统计
    updateCharts();
    updateTable();
    updateSummary();
}

// 新鲜度阶段徽章
function updateStageBadge(stage) {
    const badge = document.getElementById('stageBadge');
    const text = document.getElementById('stageText');

    badge.className = 'stage-badge';

    switch (stage) {
        case 'VERY_FRESH':
            badge.classList.add('very-fresh');
            text.textContent = 'VERY FRESH ✓';
            break;
        case 'GOOD':
            badge.classList.add('good');
            text.textContent = 'GOOD ✓';
            break;
        case 'EAT_TODAY':
            badge.classList.add('eat-today');
            text.textContent = 'EAT TODAY ⚠';
            break;
        case 'SPOILED':
            badge.classList.add('spoiled');
            text.textContent = 'SPOILED ✗';
            break;
        default:
            text.textContent = 'UNKNOWN';
    }
}

// 计算存储质量（前端）
function calculateStorageQuality(data) {
    const config = fruitConfig[currentFruit];
    let score = 100;

    const optimalTemp = (config.minTemp + config.maxTemp) / 2;
    const tempDeviation = Math.abs(data.temperature - optimalTemp);
    score -= tempDeviation * 5;

    const optimalHumid = (config.minHumid + config.maxHumid) / 2;
    const humidDeviation = Math.abs(data.humidity - optimalHumid);
    score -= humidDeviation * 2;

    return Math.max(0, Math.min(100, Math.round(score)));
}

// 存储质量进度条
function updateStorageBar(quality) {
    const bar = document.getElementById('storageBar');
    bar.style.width = `${quality}%`;
    bar.textContent = `${quality}%`;

    bar.className = 'storage-bar';
    if (quality >= 80)       bar.classList.add('excellent');
    else if (quality >= 60)  bar.classList.add('good');
    else if (quality >= 40)  bar.classList.add('fair');
    else                     bar.classList.add('poor');
}

// 最佳条件 & 提示文字
function updateOptimalConditions(data) {
    const config = fruitConfig[currentFruit];

    document.getElementById('optimalTemp').textContent =
        `${config.minTemp}-${config.maxTemp}°C`;
    document.getElementById('optimalHumid').textContent =
        `${config.minHumid}-${config.maxHumid}%`;

    const storageQuality = calculateStorageQuality(data);
    const tip = document.getElementById('storageTip');
    const tipText = document.getElementById('tipText');

    tip.className = 'optimal-tip';

    if (storageQuality >= 80) {
        tipText.textContent = 'Excellent storage conditions! Keep it up.';
    } else if (storageQuality >= 60) {
        tipText.textContent = 'Good conditions, but can be improved. Adjust temperature or humidity.';
    } else if (storageQuality >= 40) {
        tip.classList.add('warning');
        tipText.textContent = '⚠️ Not optimal. Consider adjusting temperature and humidity. ';
        if (data.temperature > config.maxTemp) {
            tipText.textContent += 'Refrigerate to extend shelf life.';
        } else if (data.temperature < config.minTemp) {
            tipText.textContent += 'Increase temperature slightly.';
        }
    } else {
        tip.classList.add('danger');
        tipText.textContent = '❌ Poor conditions! Fruit will spoil quickly. Immediate action needed.';
    }
}

// 初始化图表
function initCharts() {
    const freshnessCtx = document.getElementById('freshnessChart').getContext('2d');
    freshnessChart = new Chart(freshnessCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Freshness Score',
                data: [],
                borderColor: '#667eea',
                backgroundColor: 'rgba(102, 126, 234, 0.1)',
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            plugins: { legend: { display: true } },
            scales: { y: { beginAtZero: true, max: 100 } }
        }
    });

    const environmentCtx = document.getElementById('environmentChart').getContext('2d');
    environmentChart = new Chart(environmentCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Temperature (°C)',
                    data: [],
                    borderColor: '#eb3349',
                    backgroundColor: 'rgba(235, 51, 73, 0.1)',
                    yAxisID: 'y',
                    tension: 0.4
                },
                {
                    label: 'Humidity (%)',
                    data: [],
                    borderColor: '#4facfe',
                    backgroundColor: 'rgba(79, 172, 254, 0.1)',
                    yAxisID: 'y1',
                    tension: 0.4
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            plugins: { legend: { display: true } },
            scales: {
                y: {
                    type: 'linear',
                    position: 'left',
                    title: { display: true, text: 'Temperature (°C)' }
                },
                y1: {
                    type: 'linear',
                    position: 'right',
                    title: { display: true, text: 'Humidity (%)' },
                    grid: { drawOnChartArea: false }
                }
            }
        }
    });
}

// 更新图表
function updateCharts() {
    if (!filteredData || filteredData.length === 0) return;

    const data = filteredData.slice(0, CONFIG.MAX_HISTORY_POINTS).reverse();
    const labels = data.map(item => {
        const date = new Date(item.timestamp);
        return date.toLocaleTimeString();
    });

    freshnessChart.data.labels = labels;
    freshnessChart.data.datasets[0].data = data.map(item => item.data.score);
    freshnessChart.update();

    environmentChart.data.labels = labels;
    environmentChart.data.datasets[0].data = data.map(item => item.data.temperature);
    environmentChart.data.datasets[1].data = data.map(item => item.data.humidity);
    environmentChart.update();
}

// 更新表格
function updateTable() {
    const tbody = document.getElementById('tableBody');
    tbody.innerHTML = '';

    const recentData = filteredData.slice(0, 10);
    if (recentData.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" class="loading">No data available</td></tr>';
        return;
    }

    recentData.forEach(item => {
        const row = document.createElement('tr');
        const date = new Date(item.timestamp);

        row.innerHTML = `
            <td>${date.toLocaleString()}</td>
            <td>${item.data.score}</td>
            <td>${item.data.remainDays >= 0 ? item.data.remainDays : 'Expired'}</td>
            <td>${item.data.temperature.toFixed(1)}°C</td>
            <td>${item.data.humidity.toFixed(1)}%</td>
            <td>${item.data.gasRaw}</td>
            <td>${item.data.stage}</td>
        `;
        tbody.appendChild(row);
    });
}

// 更新统计摘要
function updateSummary() {
    if (!filteredData || filteredData.length === 0) return;

    document.getElementById('totalReadings').textContent = allData.length;

    const avgScore =
        filteredData.reduce((sum, item) => sum + item.data.score, 0) /
        filteredData.length;
    document.getElementById('avgScore').textContent = avgScore.toFixed(1);

    const latestAge = filteredData[0].data.ageHours;
    document.getElementById('monitorTime').textContent = `${latestAge}h`;

    document.getElementById('dataPoints').textContent = filteredData.length;
}

// 更新时间
function updateLastUpdate() {
    const now = new Date();
    document.getElementById('lastUpdate').textContent = now.toLocaleString();
}

// 显示无数据
function showNoData() {
    document.getElementById('scoreValue').textContent = '--';
    document.getElementById('daysValue').textContent = '--';
    document.getElementById('storageValue').textContent = '--';
    document.getElementById('tempValue').textContent = '--';
    document.getElementById('humidValue').textContent = '--';
    document.getElementById('gasValue').textContent = '--';
    document.getElementById('runtimeValue').textContent = '--';

    document.getElementById('stageText').textContent = 'NO DATA';
    document.getElementById('storageBar').style.width = '0%';

    const tbody = document.getElementById('tableBody');
    tbody.innerHTML =
        '<tr><td colspan="7" class="loading">No data available for this fruit</td></tr>';
}

// 错误提示
function showError(message) {
    console.error(message);
    alert(message);
}
