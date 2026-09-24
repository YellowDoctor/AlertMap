document.addEventListener('DOMContentLoaded', () => {
    initApp();
});

let isOnline = false;
let pollingIntervals = {};
let configData = {};

function initApp() {
    setupTabs();
    setupCollapsibles();
    setupPasswordToggles();
    setupEventListeners();
    
    // Initial fetch
    fetchStatus();
    fetchConfig();
    fetchStats();

    // Setup polling
    startPolling();
}

// --- TAB ROUTING ---
function setupTabs() {
    const tabs = document.querySelectorAll('.tab');
    const hash = window.location.hash || '#home';
    
    function switchTab(targetId) {
        document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
        document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
        
        const pane = document.getElementById(targetId);
        if(pane) pane.classList.add('active');
        
        const tab = document.querySelector(`.tab[data-target="${targetId}"]`);
        if(tab) tab.classList.add('active');
        
        window.location.hash = targetId;
    }

    tabs.forEach(tab => {
        tab.addEventListener('click', (e) => {
            e.preventDefault();
            switchTab(tab.dataset.target);
        });
    });

    switchTab(hash.substring(1));
}

function setupCollapsibles() {
    document.querySelectorAll('.collapsible .card-header').forEach(header => {
        header.addEventListener('click', () => {
            header.parentElement.classList.toggle('open');
        });
    });
}

function setupPasswordToggles() {
    document.querySelectorAll('.toggle-pw').forEach(btn => {
        btn.addEventListener('click', () => {
            const input = btn.previousElementSibling;
            if (input.type === 'password') {
                input.type = 'text';
                btn.textContent = 'рџ™€';
            } else {
                input.type = 'password';
                btn.textContent = 'рџ‘ЃпёЏ';
            }
        });
    });
}

// --- NOTIFICATIONS ---
function showToast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.textContent = message;
    
    container.appendChild(toast);
    
    setTimeout(() => {
        toast.classList.add('fade-out');
        setTimeout(() => toast.remove(), 500);
    }, 3000);
}

function updateConnectionStatus(status) {
    const indicator = document.getElementById('conn-status');
    isOnline = status;
    if (status) {
        indicator.className = 'dot dot-green';
    } else {
        indicator.className = 'dot dot-red';
    }
}

// --- UTILS ---
const hexToUint32 = hex => parseInt(hex.replace('#', ''), 16);
const uint32ToHex = num => '#' + num.toString(16).padStart(6, '0');
const formatUptime = (sec) => {
    const d = Math.floor(sec / 86400);
    const h = Math.floor((sec % 86400) / 3600);
    const m = Math.floor((sec % 3600) / 60);
    return `${d}Рґ ${h}Рі ${m}С…РІ`;
};

// --- MODAL ---
function showConfirmModal(title, desc, onConfirm) {
    const modal = document.getElementById('confirm-modal');
    document.getElementById('modal-title').textContent = title;
    document.getElementById('modal-desc').textContent = desc;
    
    modal.classList.remove('hidden');
    
    const cancelBtn = document.getElementById('modal-cancel');
    const confirmBtn = document.getElementById('modal-confirm');
    
    const cleanup = () => {
        modal.classList.add('hidden');
        cancelBtn.removeEventListener('click', onCancel);
        confirmBtn.removeEventListener('click', onOk);
    };
    
    const onCancel = () => cleanup();
    const onOk = () => { cleanup(); onConfirm(); };
    
    cancelBtn.addEventListener('click', onCancel);
    confirmBtn.addEventListener('click', onOk);
}

// --- API CALLS ---
async function apiGet(endpoint) {
    try {
        const res = await fetch(endpoint, { timeout: 3000 });
        if (!res.ok) throw new Error('Response not OK');
        const data = await res.json();
        updateConnectionStatus(true);
        return data;
    } catch (err) {
        updateConnectionStatus(false);
        throw err;
    }
}

async function apiPost(endpoint, body) {
    try {
        const res = await fetch(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        });
        if (!res.ok) throw new Error('Response not OK');
        showToast('СѓСЃРїС–С€РЅРѕ!', 'success');
        return await res.json();
    } catch (err) {
        showToast('РџРѕРјРёР»РєР° Р·Р°РїРёС‚Сѓ', 'error');
        throw err;
    }
}

// --- DATA FETCHING & RENDERING ---
function startPolling() {
    pollingIntervals.status = setInterval(fetchStatus, 5000);
    pollingIntervals.stats = setInterval(fetchStats, 10000);
}

async function fetchStatus() {
    try {
        const data = await apiGet('/api/status');
        
        // Render Header & Dashboard
        document.getElementById('header-version').textContent = 'v' + data.version;
        document.getElementById('fw-version').textContent = data.version;
        
        document.getElementById('dash-wifi').textContent = data.wifi.ssid || '--';
        document.getElementById('dash-rssi').textContent = `${data.wifi.rssi} dBm`;
        document.getElementById('dash-time').textContent = data.time || '--:--';
        document.getElementById('dash-uptime').textContent = formatUptime(data.uptime);
        document.getElementById('dash-mode').textContent = data.isNightMode ? 'РќС–С‡' : 'Р”РµРЅСЊ';
        document.getElementById('dash-heap').textContent = `RAM: ${(data.freeHeap / 1024).toFixed(1)} KB`;

        // Render Alerts
        if (data.alerts) {
            document.getElementById('sum-red').textContent = data.alerts.red || 0;
            document.getElementById('sum-yellow').textContent = data.alerts.yellow || 0;
            document.getElementById('sum-green').textContent = data.alerts.green || 0;
        }

        // Render Events
        const elist = document.getElementById('event-list');
        if (data.recentEvents && data.recentEvents.length > 0) {
            elist.innerHTML = data.recentEvents.map(e => `
                <li>
                    <span class="district">${getAlertIcon(e.level)} ${e.district}</span>
                    <span class="time">${e.time}</span>
                </li>
            `).join('');
        } else {
            elist.innerHTML = '<li class="empty">РќРµРјР°С” РїРѕРґС–Р№</li>';
        }
        
    } catch (e) {
        console.error('Status fetch failed', e);
    }
}

function getAlertIcon(level) {
    if(level === 'red') return 'рџ”ґ';
    if(level === 'yellow') return 'рџџЎ';
    return 'рџџў';
}

async function fetchStats() {
    try {
        const data = await apiGet('/api/stats');
        document.getElementById('stat-alertsToday').textContent = data.alertsToday || 0;
        document.getElementById('stat-clearsToday').textContent = data.clearsToday || 0;
        document.getElementById('stat-lastAlert').textContent = data.lastAlertTime || '--:--';
        document.getElementById('stat-longestAlert').textContent = data.longestAlert || '--';
        document.getElementById('stat-uptime').textContent = formatUptime(data.uptime || 0);
        document.getElementById('stat-wifiReconnects').textContent = data.wifiReconnects || 0;
        document.getElementById('stat-apiRequests').textContent = data.apiRequests || 0;
        document.getElementById('stat-apiErrors').textContent = data.apiErrors || 0;
        document.getElementById('stat-freeHeap').textContent = data.freeHeap ? `${data.freeHeap} bytes` : '--';
    } catch (e) {}
}

async function fetchConfig() {
    try {
        configData = await apiGet('/api/config');
        populateConfigForm(configData);
    } catch (e) {
        console.error('Config fetch failed', e);
    }
}

function populateConfigForm(cfg) {
    document.getElementById('cfg-apiToken').value = cfg.apiToken || '';
    document.getElementById('cfg-updateInterval').value = cfg.updateInterval || 15;
    
    document.getElementById('cfg-colorOk').value = uint32ToHex(cfg.colorOk || 0x00ff00);
    document.getElementById('cfg-colorDrone').value = uint32ToHex(cfg.colorDrone || 0xffff00);
    document.getElementById('cfg-colorMissile').value = uint32ToHex(cfg.colorMissile || 0xff0000);
    document.getElementById('cfg-colorPartial').value = uint32ToHex(cfg.colorPartial || 0xff8800);
    document.getElementById('cfg-colorOffline').value = uint32ToHex(cfg.colorOffline || 0x0000ff);
    
    document.getElementById('cfg-dayBright').value = cfg.dayBright || 255;
    document.getElementById('val-dayBright').textContent = cfg.dayBright || 255;
    
    document.getElementById('cfg-nightBright').value = cfg.nightBright || 50;
    document.getElementById('val-nightBright').textContent = cfg.nightBright || 50;
    
    document.getElementById('cfg-nightMode').checked = cfg.nightMode || false;
    document.getElementById('cfg-nightStart').value = cfg.nightStart || 22;
    document.getElementById('cfg-nightEnd').value = cfg.nightEnd || 7;
    
    document.getElementById('cfg-animSmooth').checked = cfg.animSmooth !== false;
    document.getElementById('cfg-animPulse').checked = cfg.animPulse || false;
    document.getElementById('cfg-animWave').checked = cfg.animWave || false;
    
    document.getElementById('cfg-ledType').value = cfg.ledType || 'WS2812B';
    document.getElementById('cfg-colorOrder').value = cfg.colorOrder || 'GRB';
    
    document.getElementById('cfg-ghOwner').value = cfg.ghOwner || 'alertmap';
    document.getElementById('cfg-ghRepo').value = cfg.ghRepo || 'esp32-fw';
    document.getElementById('cfg-ghAutoCheck').checked = cfg.ghAutoCheck || false;
    
    document.getElementById('gh-repo-display').textContent = `${cfg.ghOwner || '-'}/${cfg.ghRepo || '-'}`;
}

// --- EVENT LISTENERS ---
function setupEventListeners() {
    // Config form
    document.getElementById('config-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const payload = {
            apiToken: document.getElementById('cfg-apiToken').value,
            updateInterval: parseInt(document.getElementById('cfg-updateInterval').value),
            
            colorOk: hexToUint32(document.getElementById('cfg-colorOk').value),
            colorDrone: hexToUint32(document.getElementById('cfg-colorDrone').value),
            colorMissile: hexToUint32(document.getElementById('cfg-colorMissile').value),
            colorPartial: hexToUint32(document.getElementById('cfg-colorPartial').value),
            colorOffline: hexToUint32(document.getElementById('cfg-colorOffline').value),
            
            dayBright: parseInt(document.getElementById('cfg-dayBright').value),
            nightBright: parseInt(document.getElementById('cfg-nightBright').value),
            nightMode: document.getElementById('cfg-nightMode').checked,
            nightStart: parseInt(document.getElementById('cfg-nightStart').value),
            nightEnd: parseInt(document.getElementById('cfg-nightEnd').value),
            
            animSmooth: document.getElementById('cfg-animSmooth').checked,
            animPulse: document.getElementById('cfg-animPulse').checked,
            animWave: document.getElementById('cfg-animWave').checked,
            
            ledType: document.getElementById('cfg-ledType').value,
            colorOrder: document.getElementById('cfg-colorOrder').value,
            
            ghOwner: document.getElementById('cfg-ghOwner').value,
            ghRepo: document.getElementById('cfg-ghRepo').value,
            ghAutoCheck: document.getElementById('cfg-ghAutoCheck').checked
        };
        
        await apiPost('/api/config', payload);
        fetchConfig(); // Reload
    });

    // WiFi form
    document.getElementById('wifi-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const payload = {
            ssid: document.getElementById('wifi-ssid').value,
            pass: document.getElementById('wifi-pass').value
        };
        await apiPost('/api/wifi', payload);
        showConfirmModal('WiFi Р—Р±РµСЂРµР¶РµРЅРѕ', 'РџСЂРёСЃС‚СЂС–Р№ РїРѕС‚СЂС–Р±РЅРѕ РїРµСЂРµР·Р°РІР°РЅС‚Р°Р¶РёС‚Рё РґР»СЏ Р·Р°СЃС‚РѕСЃСѓРІР°РЅРЅСЏ РЅР°Р»Р°С€С‚СѓРІР°РЅСЊ. РџРµСЂРµР·Р°РІР°РЅС‚Р°Р¶РёС‚Рё Р·Р°СЂР°Р·?', () => {
            apiPost('/api/restart', {});
        });
    });

    // Brightness live update handlers
    document.getElementById('cfg-dayBright').addEventListener('input', (e) => {
        document.getElementById('val-dayBright').textContent = e.target.value;
    });
    document.getElementById('cfg-nightBright').addEventListener('input', (e) => {
        document.getElementById('val-nightBright').textContent = e.target.value;
    });

    // Action buttons
    document.getElementById('btn-test').addEventListener('click', () => {
        apiPost('/api/test/pattern', { pattern: 'cycle' });
        showToast('РўРµСЃС‚РѕРІРёР№ СЂРµР¶РёРј Р·Р°РїСѓС‰РµРЅРѕ');
    });

    document.getElementById('btn-restart').addEventListener('click', () => {
        showConfirmModal('РџРµСЂРµР·Р°РІР°РЅС‚Р°Р¶РµРЅРЅСЏ', 'Р’Рё РІРїРµРІРЅРµРЅС– С‰Рѕ С…РѕС‡РµС‚Рµ РїРµСЂРµР·Р°РІР°РЅС‚Р°Р¶РёС‚Рё РїСЂРёСЃС‚СЂС–Р№?', () => {
            apiPost('/api/restart', {});
            showToast('РџСЂРёСЃС‚СЂС–Р№ РїРµСЂРµР·Р°РІР°РЅС‚Р°Р¶СѓС”С‚СЊСЃСЏ...', 'info');
        });
    });

    // OTA Section
    document.getElementById('btn-check-update').addEventListener('click', async () => {
        try {
            const btn = document.getElementById('btn-check-update');
            btn.textContent = 'вЏі РџРµСЂРµРІС–СЂРєР°...';
            btn.disabled = true;
            
            const data = await apiGet('/api/ota/check');
            
            if (data.available) {
                document.getElementById('gh-update-info').classList.remove('hidden');
                document.getElementById('gh-release-notes').innerHTML = `<strong>РќРѕРІР° РІРµСЂСЃС–СЏ: ${data.version}</strong><br>${data.notes || ''}`;
                showToast('Р—РЅР°Р№РґРµРЅРѕ РѕРЅРѕРІР»РµРЅРЅСЏ!', 'success');
            } else {
                document.getElementById('gh-update-info').classList.add('hidden');
                showToast('РЈ РІР°СЃ РѕСЃС‚Р°РЅРЅСЏ РІРµСЂСЃС–СЏ.', 'info');
            }
        } catch (e) {
            showToast('РџРѕРјРёР»РєР° РїРµСЂРµРІС–СЂРєРё РѕРЅРѕРІР»РµРЅСЊ', 'error');
        } finally {
            const btn = document.getElementById('btn-check-update');
            btn.textContent = 'рџ”Ќ РџРµСЂРµРІС–СЂРёС‚Рё РѕРЅРѕРІР»РµРЅРЅСЏ';
            btn.disabled = false;
        }
    });

    document.getElementById('btn-do-update').addEventListener('click', () => {
        showConfirmModal('РћРЅРѕРІР»РµРЅРЅСЏ РїСЂРѕС€РёРІРєРё', 'РџС–Рґ С‡Р°СЃ РѕРЅРѕРІР»РµРЅРЅСЏ РЅРµ РІРёРјРёРєР°Р№С‚Рµ Р¶РёРІР»РµРЅРЅСЏ. РџСЂРѕРґРѕРІР¶РёС‚Рё?', () => {
            apiPost('/api/ota/github', {});
            startUpdateProgressMonitoring();
        });
    });

    document.getElementById('upload-form').addEventListener('submit', (e) => {
        e.preventDefault();
        const fileInput = document.getElementById('fw-file');
        if (!fileInput.files.length) return;
        
        showConfirmModal('Р—Р°РІР°РЅС‚Р°Р¶РµРЅРЅСЏ РїСЂРѕС€РёРІРєРё', 'РџС–Рґ С‡Р°СЃ РѕРЅРѕРІР»РµРЅРЅСЏ РЅРµ РІРёРјРёРєР°Р№С‚Рµ Р¶РёРІР»РµРЅРЅСЏ. РџСЂРѕРґРѕРІР¶РёС‚Рё?', () => {
            uploadFirmware(fileInput.files[0]);
        });
    });
}

// --- FIRMWARE UPLOAD ---
function uploadFirmware(file) {
    document.getElementById('update-progress-container').classList.remove('hidden');
    const fill = document.getElementById('update-fill');
    const text = document.getElementById('update-text');
    
    const formData = new FormData();
    formData.append('update', file, 'firmware.bin');
    
    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/ota/upload', true);
    
    xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
            const percent = Math.round((e.loaded / e.total) * 100);
            fill.style.width = percent + '%';
            text.textContent = `${percent}% - Р—Р°РІР°РЅС‚Р°Р¶РµРЅРЅСЏ С„Р°Р№Р»Сѓ...`;
        }
    };
    
    xhr.onload = () => {
        if (xhr.status === 200) {
            text.textContent = '100% - Р—Р°РІРµСЂС€РµРЅРѕ! РџРµСЂРµР·Р°РІР°РЅС‚Р°Р¶РµРЅРЅСЏ...';
            showToast('РћРЅРѕРІР»РµРЅРЅСЏ СѓСЃРїС–С€РЅРµ', 'success');
            setTimeout(() => window.location.reload(), 10000);
        } else {
            text.textContent = 'РџРѕРјРёР»РєР° РѕРЅРѕРІР»РµРЅРЅСЏ';
            fill.style.background = 'var(--danger)';
            showToast('РџРѕРјРёР»РєР° РѕРЅРѕРІР»РµРЅРЅСЏ', 'error');
        }
    };
    
    xhr.onerror = () => {
        text.textContent = 'РџРѕРјРёР»РєР° РјРµСЂРµР¶С–';
        fill.style.background = 'var(--danger)';
        showToast('РџРѕРјРёР»РєР° РјРµСЂРµР¶С– РїСЂРё Р·Р°РІР°РЅС‚Р°Р¶РµРЅРЅС–', 'error');
    };
    
    xhr.send(formData);
}

function startUpdateProgressMonitoring() {
    document.getElementById('update-progress-container').classList.remove('hidden');
    const fill = document.getElementById('update-fill');
    const text = document.getElementById('update-text');
    
    const iv = setInterval(async () => {
        try {
            const data = await apiGet('/api/ota/progress');
            fill.style.width = data.progress + '%';
            text.textContent = `${data.progress}% - ${data.status || 'РћРЅРѕРІР»РµРЅРЅСЏ...'}`;
            
            if (data.progress >= 100 || data.status === 'done') {
                clearInterval(iv);
                text.textContent = 'РћРЅРѕРІР»РµРЅРЅСЏ СѓСЃРїС–С€РЅРµ! РџРµСЂРµР·Р°РІР°РЅС‚Р°Р¶РµРЅРЅСЏ...';
                showToast('РћРЅРѕРІР»РµРЅРЅСЏ Р·Р°РІРµСЂС€РµРЅРѕ', 'success');
                setTimeout(() => window.location.reload(), 5000);
            } else if (data.status === 'error') {
                clearInterval(iv);
                fill.style.background = 'var(--danger)';
                text.textContent = 'РџРѕРјРёР»РєР° РѕРЅРѕРІР»РµРЅРЅСЏ';
            }
        } catch (e) {
            // Keep trying or ignore temporary failure
        }
    }, 1000);
}
