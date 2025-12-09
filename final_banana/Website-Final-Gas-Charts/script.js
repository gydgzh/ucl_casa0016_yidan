// =============================================================================
// Fruit Freshness Monitor v3.5 - JavaScript
// With Q₁₀-based shelf life calculation
// =============================================================================

// 全局变量
let currentFruit = 0;
let allData = [];
let filteredData = [];
let freshnessChart = null;
let environmentChart = null;
let gasChart = null;
let gasCompositionChart = null;  // 🆕 气体成分雷达图
let gasHistoryChart = null;      // 🆕 气体历史堆叠图

// 水果配置（更新为v3.5阈值）
const fruitConfig = {
    0: { 
        name: 'Banana', 
        emoji: '🍌', 
        minTemp: 18, maxTemp: 22,  // 家庭存储
        minHumid: 50, maxHumid: 60,
        optimalTemp: 13, maxOptimalTemp: 15,  // 专业冷藏
        optimalHumid: 90, maxOptimalHumid: 95,
        baseShelfLife: 7.0,  // 20°C基准
        gasDeltaThreshold: 10,
        scoreThreshold: 38  // v3.5降低阈值
    },
    1: { 
        name: 'Orange', 
        emoji: '🍊', 
        minTemp: 4, maxTemp: 10, 
        minHumid: 85, maxHumid: 90,
        optimalTemp: 4, maxOptimalTemp: 10,
        optimalHumid: 85, maxOptimalHumid: 90,
        baseShelfLife: 14.0,
        gasDeltaThreshold: 15,
        scoreThreshold: 45
    }
};

// Q₁₀常数和参考温度
const Q10 = 2.5;
const REFERENCE_TEMP = 20.0;  // °C

// 页面加载时初始化
document.addEventListener('DOMContentLoaded', function () {
    console.log('Initializing Fruit Monitor Dashboard v3.5...');

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

// =============================================================================
// Q₁₀-Based Shelf Life Calculation (Scientific)
// =============================================================================

function calculateQ10ShelfLife(temp, humidity, score) {
    const config = fruitConfig[currentFruit];
    
    // 1. 温度因子 (Q₁₀法则)
    const tempFactor = Math.pow(Q10, (temp - REFERENCE_TEMP) / 10.0);
    
    // 2. 湿度因子
    let humidityFactor;
    if (humidity >= 90) humidityFactor = 1.3;      // 理想高湿度
    else if (humidity >= 70) humidityFactor = 1.1; // 良好
    else if (humidity >= 50) humidityFactor = 1.0; // 可接受
    else if (humidity >= 40) humidityFactor = 0.85; // 偏低
    else humidityFactor = 0.7;  // 很低
    
    // 3. 气体质量因子
    let gasFactor = score / 60.0;
    if (gasFactor > 1.2) gasFactor = 1.2;  // 上限
    if (gasFactor < 0.5) gasFactor = 0.5;  // 下限
    
    // 4. 综合计算
    let shelfDays = config.baseShelfLife * humidityFactor * gasFactor / tempFactor;
    
    // 5. 合理范围
    if (shelfDays < 1) shelfDays = 1;
    if (shelfDays > 40) shelfDays = 40;
    
    return {
        days: shelfDays,
        tempFactor: tempFactor,
        humidityFactor: humidityFactor,
        gasFactor: gasFactor
    };
}

// =============================================================================
// 数据加载和解析
// =============================================================================

async function loadData() {
    if (CONFIG.DEBUG) console.log('Loading data from TTN Storage…');

    try {
        const url =
            `${CONFIG.TTN_BASE_URL}` +
            `/api/v3/as/applications/${CONFIG.TTN_APP_ID}` +
            `/devices/${CONFIG.DEVICE_ID}/packages/storage/uplink_message` +
            `?field_mask=up.uplink_message.decoded_payload,up.uplink_message.received_at`;

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
        if (CONFIG.DEBUG) console.log('Raw response text (first 500 chars):', text.substring(0, 500));

        const lines = text.trim().split('\n').filter(line => line.length > 0);

        allData = lines.map(line => {
            try {
                const cleaned = line.startsWith('data:') ? line.substring(5).trim() : line;
                const json = JSON.parse(cleaned);
                const result = json.result || json;

                // 解析Arduino的13字节payload
                const payload = result.uplink_message.decoded_payload;
                
                return {
                    timestamp: result.uplink_message.received_at,
                    data: {
                        fruitType: payload.fruitType || 0,
                        temperature: payload.temperature || 0,
                        humidity: payload.humidity || 0,
                        gasRaw: payload.gasRaw || 0,
                        gasDelta: payload.gasDelta || 0,
                        score: payload.score || 0,
                        // 兼容新旧字段名
                        remainingDays: payload.remainingDays || payload.remainDays || 0,
                        stage: payload.stage || 0,
                        runtime: payload.runtime || payload.ageHours || 0
                    }
                };
            } catch (e) {
                console.error('Parse error for line:', line, e);
                return null;
            }
        }).filter(item => item !== null).reverse();

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
        if (typeof t === 'number') {
            return t === fruitType;
        }
        // 默认香蕉
        if (fruitType === 0) return true;
        return false;
    });

    if (CONFIG.DEBUG) {
        console.log(`Filtered ${filteredData.length} records for fruit ${fruitType}`);
    }
}

// 切换水果
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

// =============================================================================
// UI更新
// =============================================================================

function updateUI() {
    if (!filteredData || filteredData.length === 0) {
        showNoData();
        return;
    }

    const latest = filteredData[0].data;
    const config = fruitConfig[currentFruit];

    // 标题
    document.getElementById('currentEmoji').textContent = config.emoji;
    document.getElementById('currentFruit').textContent = config.name;

    // 新鲜度阶段
    updateStageBadge(latest.stage);

    // 计算Q₁₀-based shelf life
    const q10Result = calculateQ10ShelfLife(
        latest.temperature, 
        latest.humidity, 
        latest.score
    );

    // 核心指标
    document.getElementById('scoreValue').textContent = latest.score;
    document.getElementById('daysValue').textContent = 
        (latest.remainingDays >= 0 ? latest.remainingDays : 'Exp');
    document.getElementById('storageValue').textContent = calculateStorageQuality(latest);

    // 存储质量进度条
    const storageQuality = calculateStorageQuality(latest);
    updateStorageBar(storageQuality);

    // 环境数据
    updateEnvironmentData(latest);

    // Q₁₀模型详情
    updateQ10Details(q10Result, latest.temperature);

    // 最佳条件对比
    updateOptimalComparison(latest, config);

    // 更新图表
    updateCharts();

    // 更新表格
    updateTable();

    // 更新统计
    updateStatistics();
}

function updateEnvironmentData(data) {
    const config = fruitConfig[currentFruit];
    
    // 温度
    document.getElementById('tempValue').textContent = `${data.temperature.toFixed(1)}°C`;
    document.getElementById('tempQuality').textContent = evaluateTemp(data.temperature, config);
    
    // 湿度
    document.getElementById('humidValue').textContent = `${data.humidity.toFixed(1)}%`;
    document.getElementById('humidQuality').textContent = evaluateHumidity(data.humidity, config);
    
    // Gas Raw
    document.getElementById('gasRawValue').textContent = `${data.gasRaw} ADC`;
    document.getElementById('gasQuality').textContent = evaluateGasRaw(data.gasRaw);
    
    // Gas Delta
    const deltaSign = data.gasDelta > 0 ? '+' : '';
    document.getElementById('gasDeltaValue').textContent = `${deltaSign}${data.gasDelta} ADC`;
    document.getElementById('gasDeltaQuality').textContent = evaluateGasDelta(data.gasDelta, config);
}

function evaluateTemp(temp, config) {
    if (temp <= config.maxOptimalTemp && temp >= config.optimalTemp) {
        return '✅ Optimal';
    } else if (temp <= config.maxTemp && temp >= config.minTemp) {
        return '🟡 Acceptable';
    } else if (temp > config.maxTemp) {
        return `🔴 Too High (+${(temp - config.maxTemp).toFixed(1)}°C)`;
    } else {
        return `🔵 Too Low (${(config.minTemp - temp).toFixed(1)}°C)`;
    }
}

function evaluateHumidity(humid, config) {
    if (humid >= config.optimalHumid && humid <= config.maxOptimalHumid) {
        return '✅ Optimal';
    } else if (humid >= config.minHumid && humid <= config.maxHumid) {
        return '🟡 Acceptable';
    } else if (humid < config.minHumid) {
        return `🔴 Too Low (-${(config.minHumid - humid).toFixed(1)}%)`;
    } else {
        return `🔵 Too High (+${(humid - config.maxHumid).toFixed(1)}%)`;
    }
}

function evaluateGasRaw(gasRaw) {
    if (gasRaw < 150) {
        return '✅ Very Clean';
    } else if (gasRaw < 200) {
        return '🟢 Clean';
    } else if (gasRaw < 300) {
        return '🟡 Moderate';
    } else {
        return '🔴 High';
    }
}

function evaluateGasDelta(gasDelta, config) {
    if (Math.abs(gasDelta) <= 5) {
        return '✅ Stable';
    } else if (Math.abs(gasDelta) <= config.gasDeltaThreshold) {
        return '🟡 Minor Change';
    } else {
        return '🔴 Significant Change';
    }
}

function updateQ10Details(q10Result, currentTemp) {
    const config = fruitConfig[currentFruit];
    
    document.getElementById('baseShelfLife').textContent = `${config.baseShelfLife} days`;
    document.getElementById('tempFactor').textContent = q10Result.tempFactor.toFixed(2);
    document.getElementById('humidFactor').textContent = q10Result.humidityFactor.toFixed(2);
    document.getElementById('gasFactor').textContent = q10Result.gasFactor.toFixed(2);
    document.getElementById('calculatedDays').textContent = `${q10Result.days.toFixed(1)} days`;
}

function updateOptimalComparison(data, config) {
    document.getElementById('currentTempComp').textContent = `${data.temperature.toFixed(1)}°C`;
    document.getElementById('currentHumidComp').textContent = `${data.humidity.toFixed(1)}%`;
    document.getElementById('currentGasComp').textContent = `${data.gasDelta >= 0 ? '+' : ''}${data.gasDelta}`;
    
    document.getElementById('optimalTemp').textContent = `${config.minTemp}-${config.maxTemp}°C`;
    document.getElementById('optimalHumid').textContent = `${config.minHumid}-${config.maxHumid}%`;
    
    // 生成建议
    const tips = [];
    if (data.temperature > config.maxTemp) {
        tips.push(`Lower temperature by ${(data.temperature - config.maxTemp).toFixed(1)}°C`);
    }
    if (data.humidity < config.minHumid) {
        tips.push(`Increase humidity by ${(config.minHumid - data.humidity).toFixed(1)}%`);
    }
    if (Math.abs(data.gasDelta) > config.gasDeltaThreshold) {
        tips.push('Wait 1-2 minutes for sensor recovery');
    }
    
    const tipText = tips.length > 0 ? 
        tips.join(' • ') : 
        'Current conditions are acceptable for storage.';
    
    document.getElementById('tipText').textContent = tipText;
}

function calculateStorageQuality(data) {
    const config = fruitConfig[currentFruit];
    
    // 温度评分 (30%)
    let tempScore = 100;
    if (data.temperature > config.maxTemp) {
        tempScore = Math.max(0, 100 - (data.temperature - config.maxTemp) * 10);
    } else if (data.temperature < config.minTemp) {
        tempScore = Math.max(0, 100 - (config.minTemp - data.temperature) * 10);
    }
    
    // 湿度评分 (30%)
    let humidScore = 100;
    if (data.humidity < config.minHumid) {
        humidScore = Math.max(0, 100 - (config.minHumid - data.humidity) * 2);
    } else if (data.humidity > config.maxHumid) {
        humidScore = Math.max(0, 100 - (data.humidity - config.maxHumid) * 2);
    }
    
    // 气体评分 (40%)
    let gasScore = 100;
    if (Math.abs(data.gasDelta) > config.gasDeltaThreshold) {
        gasScore = Math.max(0, 100 - Math.abs(data.gasDelta - config.gasDeltaThreshold) * 5);
    }
    
    return Math.round(tempScore * 0.3 + humidScore * 0.3 + gasScore * 0.4);
}

function updateStorageBar(quality) {
    const bar = document.getElementById('storageBar');
    bar.style.width = `${quality}%`;
    
    if (quality >= 80) {
        bar.style.background = 'linear-gradient(90deg, #22c55e, #10b981)';
    } else if (quality >= 60) {
        bar.style.background = 'linear-gradient(90deg, #eab308, #f59e0b)';
    } else {
        bar.style.background = 'linear-gradient(90deg, #ef4444, #dc2626)';
    }
}

function updateStageBadge(stage) {
    const badge = document.getElementById('stageBadge');
    const text = document.getElementById('stageText');
    
    const stages = ['VERY FRESH', 'GOOD', 'EAT TODAY', 'SPOILED'];
    const colors = ['#22c55e', '#3b82f6', '#f59e0b', '#ef4444'];
    
    text.textContent = stages[stage] || 'UNKNOWN';
    badge.style.background = colors[stage] || '#6b7280';
}

// =============================================================================
// 图表初始化和更新
// =============================================================================

function initCharts() {
    // Freshness Chart
    const freshnessCtx = document.getElementById('freshnessChart');
    if (freshnessCtx) {
        freshnessChart = new Chart(freshnessCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Freshness Score',
                        data: [],
                        borderColor: '#3b82f6',
                        backgroundColor: 'rgba(59, 130, 246, 0.1)',
                        tension: 0.4,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Shelf Life (days)',
                        data: [],
                        borderColor: '#22c55e',
                        backgroundColor: 'rgba(34, 197, 94, 0.1)',
                        tension: 0.4,
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: { intersect: false, mode: 'index' },
                scales: {
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: { display: true, text: 'Score (0-100)' },
                        min: 0,
                        max: 100
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: { display: true, text: 'Days' },
                        min: 0,
                        grid: { drawOnChartArea: false }
                    }
                }
            }
        });
    }

    // Environment Chart
    const envCtx = document.getElementById('environmentChart');
    if (envCtx) {
        environmentChart = new Chart(envCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Temperature (°C)',
                        data: [],
                        borderColor: '#ef4444',
                        backgroundColor: 'rgba(239, 68, 68, 0.1)',
                        tension: 0.4,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Humidity (%)',
                        data: [],
                        borderColor: '#3b82f6',
                        backgroundColor: 'rgba(59, 130, 246, 0.1)',
                        tension: 0.4,
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: { intersect: false, mode: 'index' },
                scales: {
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: { display: true, text: 'Temperature (°C)' }
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: { display: true, text: 'Humidity (%)' },
                        grid: { drawOnChartArea: false }
                    }
                }
            }
        });
    }

    // Gas Chart
    const gasCtx = document.getElementById('gasChart');
    if (gasCtx) {
        gasChart = new Chart(gasCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Gas Raw (ADC)',
                        data: [],
                        borderColor: '#8b5cf6',
                        backgroundColor: 'rgba(139, 92, 246, 0.1)',
                        tension: 0.4,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Gas Delta (ADC)',
                        data: [],
                        borderColor: '#f59e0b',
                        backgroundColor: 'rgba(245, 158, 11, 0.1)',
                        tension: 0.4,
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: { intersect: false, mode: 'index' },
                scales: {
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: { display: true, text: 'Gas Raw (ADC)' }
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: { display: true, text: 'Gas Delta (ADC)' },
                        grid: { drawOnChartArea: false }
                    }
                }
            }
        });
    }

    // 🆕 Gas Composition Radar Chart
    const gasCompCtx = document.getElementById('gasCompositionChart');
    if (gasCompCtx) {
        gasCompositionChart = new Chart(gasCompCtx, {
            type: 'radar',
            data: {
                labels: ['NH₃', 'Alcohol', 'CO₂', 'NOₓ', 'Benzene', 'Smoke'],
                datasets: [{
                    label: 'Estimated Gas Composition',
                    data: [0, 0, 0, 0, 0, 0],
                    borderColor: '#0369a1',
                    backgroundColor: 'rgba(3, 105, 161, 0.2)',
                    pointBackgroundColor: '#0369a1',
                    pointBorderColor: '#fff',
                    pointHoverBackgroundColor: '#fff',
                    pointHoverBorderColor: '#0369a1',
                    pointRadius: 5,
                    pointHoverRadius: 7
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    r: {
                        beginAtZero: true,
                        max: 100,
                        ticks: {
                            stepSize: 20,
                            font: { size: 11 }
                        },
                        pointLabels: {
                            font: { size: 13, weight: 'bold' }
                        }
                    }
                },
                plugins: {
                    legend: {
                        display: false
                    },
                    tooltip: {
                        callbacks: {
                            label: function(context) {
                                return context.label + ': ' + context.parsed.r.toFixed(1) + '%';
                            }
                        }
                    }
                }
            }
        });
    }

    // 🆕 Gas History Stacked Area Chart
    const gasHistCtx = document.getElementById('gasHistoryChart');
    if (gasHistCtx) {
        gasHistoryChart = new Chart(gasHistCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'NH₃',
                        data: [],
                        borderColor: '#ef4444',
                        backgroundColor: 'rgba(239, 68, 68, 0.4)',
                        fill: true,
                        tension: 0.4
                    },
                    {
                        label: 'Alcohol',
                        data: [],
                        borderColor: '#f97316',
                        backgroundColor: 'rgba(249, 115, 22, 0.4)',
                        fill: true,
                        tension: 0.4
                    },
                    {
                        label: 'CO₂',
                        data: [],
                        borderColor: '#3b82f6',
                        backgroundColor: 'rgba(59, 130, 246, 0.4)',
                        fill: true,
                        tension: 0.4
                    },
                    {
                        label: 'NOₓ',
                        data: [],
                        borderColor: '#8b5cf6',
                        backgroundColor: 'rgba(139, 92, 246, 0.4)',
                        fill: true,
                        tension: 0.4
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: { intersect: false, mode: 'index' },
                scales: {
                    y: {
                        stacked: true,
                        beginAtZero: true,
                        max: 100,
                        title: { display: true, text: 'Estimated Concentration (%)' }
                    },
                    x: {
                        stacked: true
                    }
                },
                plugins: {
                    legend: {
                        display: false
                    },
                    tooltip: {
                        mode: 'index',
                        callbacks: {
                            label: function(context) {
                                return context.dataset.label + ': ' + context.parsed.y.toFixed(1) + '%';
                            }
                        }
                    }
                }
            }
        });
    }
}

// =============================================================================
// 🆕 气体成分估算算法
// =============================================================================

/**
 * 基于新鲜度阶段和Gas Delta估算各气体的相对浓度
 * 根据科学文献中的典型水果腐败模式
 */
function estimateGasComposition(stage, gasDelta) {
    // 基础模式（基于阶段）
    let basePattern = {
        nh3: 0,      // 氨气（蛋白质分解）
        alcohol: 0,  // 酒精（厌氧发酵）
        co2: 0,      // 二氧化碳（呼吸）
        nox: 0,      // 氮氧化物（氧化）
        benzene: 0,  // 苯（有机分解）
        smoke: 0     // 烟雾（环境）
    };

    // Stage 0: VERY FRESH (80-100分)
    if (stage === 0) {
        basePattern.co2 = 30;      // 正常呼吸
        basePattern.smoke = 10;    // 环境背景
        basePattern.alcohol = 5;   // 微量
        basePattern.nh3 = 2;
        basePattern.nox = 3;
        basePattern.benzene = 2;
    }
    // Stage 1: GOOD (60-79分)
    else if (stage === 1) {
        basePattern.co2 = 45;      // 呼吸加速
        basePattern.alcohol = 20;  // 开始发酵
        basePattern.smoke = 12;
        basePattern.nh3 = 8;       // 轻微分解
        basePattern.nox = 10;
        basePattern.benzene = 8;
    }
    // Stage 2: EAT TODAY (40-59分)
    else if (stage === 2) {
        basePattern.alcohol = 35;  // 明显发酵
        basePattern.co2 = 40;
        basePattern.nh3 = 25;      // 蛋白质分解加速
        basePattern.nox = 20;
        basePattern.benzene = 18;
        basePattern.smoke = 15;
    }
    // Stage 3: SPOILED (<40分)
    else if (stage === 3) {
        basePattern.nh3 = 50;      // 严重腐败
        basePattern.alcohol = 40;
        basePattern.nox = 35;
        basePattern.benzene = 30;
        basePattern.co2 = 35;
        basePattern.smoke = 20;
    }

    // Gas Delta修正因子
    // gasDelta越大，所有气体浓度越高
    let deltaFactor = 1.0;
    if (gasDelta > 50) deltaFactor = 2.0;
    else if (gasDelta > 30) deltaFactor = 1.5;
    else if (gasDelta > 10) deltaFactor = 1.2;
    else if (gasDelta > 0) deltaFactor = 1.0;
    else if (gasDelta > -10) deltaFactor = 0.8;
    else deltaFactor = 0.6;

    // 应用修正因子
    for (let key in basePattern) {
        basePattern[key] = Math.min(100, basePattern[key] * deltaFactor);
    }

    return basePattern;
}

function updateCharts() {
    if (!filteredData || filteredData.length === 0) return;

    const maxPoints = CONFIG.MAX_HISTORY_POINTS || 50;
    const data = filteredData.slice(0, maxPoints).reverse();

    const labels = data.map(item => {
        const date = new Date(item.timestamp);
        return `${date.getHours()}:${String(date.getMinutes()).padStart(2, '0')}`;
    });

    // Freshness Chart
    if (freshnessChart) {
        freshnessChart.data.labels = labels;
        freshnessChart.data.datasets[0].data = data.map(item => item.data.score);
        freshnessChart.data.datasets[1].data = data.map(item => item.data.remainingDays);
        freshnessChart.update();
    }

    // Environment Chart
    if (environmentChart) {
        environmentChart.data.labels = labels;
        environmentChart.data.datasets[0].data = data.map(item => item.data.temperature);
        environmentChart.data.datasets[1].data = data.map(item => item.data.humidity);
        environmentChart.update();
    }

    // Gas Chart
    if (gasChart) {
        gasChart.data.labels = labels;
        gasChart.data.datasets[0].data = data.map(item => item.data.gasRaw);
        gasChart.data.datasets[1].data = data.map(item => item.data.gasDelta);
        gasChart.update();
    }

    // 🆕 Update Gas Composition Charts
    updateGasCompositionCharts(data);
}

/**
 * 更新气体成分估算图表
 */
function updateGasCompositionCharts(data) {
    if (!data || data.length === 0) return;

    // 获取最新数据
    const latest = data[data.length - 1].data;
    const composition = estimateGasComposition(latest.stage, latest.gasDelta);

    // 更新雷达图（当前状态）
    if (gasCompositionChart) {
        gasCompositionChart.data.datasets[0].data = [
            composition.nh3,
            composition.alcohol,
            composition.co2,
            composition.nox,
            composition.benzene,
            composition.smoke
        ];
        gasCompositionChart.update();
    }

    // 更新阶段指示器
    const stageNames = ['VERY FRESH', 'GOOD', 'EAT TODAY', 'SPOILED'];
    const stageColors = ['#22c55e', '#3b82f6', '#f59e0b', '#ef4444'];
    
    const stageElement = document.getElementById('compositionStage');
    const gasDeltaElement = document.getElementById('compositionGasDelta');
    const indicatorElement = document.getElementById('currentStageIndicator');
    
    if (stageElement) {
        stageElement.textContent = stageNames[latest.stage] || 'UNKNOWN';
    }
    if (gasDeltaElement) {
        const sign = latest.gasDelta >= 0 ? '+' : '';
        gasDeltaElement.textContent = `${sign}${latest.gasDelta} ADC`;
    }
    if (indicatorElement) {
        indicatorElement.style.background = `linear-gradient(135deg, ${stageColors[latest.stage]}, ${stageColors[latest.stage]}dd)`;
    }

    // 更新历史堆叠图（最近20个点）
    if (gasHistoryChart) {
        const historyPoints = Math.min(20, data.length);
        const historyData = data.slice(-historyPoints);
        
        const historyLabels = historyData.map(item => {
            const date = new Date(item.timestamp);
            return `${date.getHours()}:${String(date.getMinutes()).padStart(2, '0')}`;
        });

        // 计算每个时间点的气体成分
        const nh3Data = [];
        const alcoholData = [];
        const co2Data = [];
        const noxData = [];

        historyData.forEach(item => {
            const comp = estimateGasComposition(item.data.stage, item.data.gasDelta);
            nh3Data.push(comp.nh3);
            alcoholData.push(comp.alcohol);
            co2Data.push(comp.co2);
            noxData.push(comp.nox);
        });

        gasHistoryChart.data.labels = historyLabels;
        gasHistoryChart.data.datasets[0].data = nh3Data;
        gasHistoryChart.data.datasets[1].data = alcoholData;
        gasHistoryChart.data.datasets[2].data = co2Data;
        gasHistoryChart.data.datasets[3].data = noxData;
        gasHistoryChart.update();
    }
}

// =============================================================================
// 表格和统计
// =============================================================================

function updateTable() {
    const tbody = document.getElementById('tableBody');
    if (!tbody) return;

    tbody.innerHTML = '';

    const displayData = filteredData.slice(0, 10);
    const stages = ['VERY FRESH', 'GOOD', 'EAT TODAY', 'SPOILED'];

    displayData.forEach(item => {
        const row = tbody.insertRow();
        const date = new Date(item.timestamp);
        const timeStr = `${date.getMonth() + 1}/${date.getDate()} ${date.getHours()}:${String(date.getMinutes()).padStart(2, '0')}`;

        row.innerHTML = `
            <td>${timeStr}</td>
            <td>${item.data.score}</td>
            <td>${item.data.remainingDays >= 0 ? item.data.remainingDays : 'Exp'}</td>
            <td>${item.data.temperature.toFixed(1)}</td>
            <td>${item.data.humidity.toFixed(1)}</td>
            <td>${item.data.gasRaw}</td>
            <td>${item.data.gasDelta >= 0 ? '+' : ''}${item.data.gasDelta}</td>
            <td><span class="stage-${item.data.stage}">${stages[item.data.stage] || 'N/A'}</span></td>
        `;
    });
}

function updateStatistics() {
    if (!filteredData || filteredData.length === 0) return;

    document.getElementById('totalReadings').textContent = filteredData.length;
    document.getElementById('dataPoints').textContent = filteredData.length;

    const avgScore = filteredData.reduce((sum, item) => sum + item.data.score, 0) / filteredData.length;
    document.getElementById('avgScore').textContent = avgScore.toFixed(1);

    const validDays = filteredData.filter(item => item.data.remainingDays >= 0);
    if (validDays.length > 0) {
        const avgDays = validDays.reduce((sum, item) => sum + item.data.remainingDays, 0) / validDays.length;
        document.getElementById('avgShelfLife').textContent = `${avgDays.toFixed(1)} days`;
    } else {
        document.getElementById('avgShelfLife').textContent = 'N/A';
    }
}

// =============================================================================
// 辅助函数
// =============================================================================

function updateLastUpdate() {
    const now = new Date();
    document.getElementById('lastUpdate').textContent = 
        `${now.getHours()}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
}

function showNoData() {
    document.getElementById('scoreValue').textContent = '--';
    document.getElementById('daysValue').textContent = '--';
    document.getElementById('storageValue').textContent = '--';
    document.getElementById('tempValue').textContent = '--';
    document.getElementById('humidValue').textContent = '--';
    document.getElementById('gasRawValue').textContent = '--';
    document.getElementById('gasDeltaValue').textContent = '--';
    
    const tbody = document.getElementById('tableBody');
    if (tbody) {
        tbody.innerHTML = '<tr><td colspan="8" class="loading">No data available for this fruit type</td></tr>';
    }
}

function showError(message) {
    console.error(message);
    const tbody = document.getElementById('tableBody');
    if (tbody) {
        tbody.innerHTML = `<tr><td colspan="8" class="error">${message}</td></tr>`;
    }
}
