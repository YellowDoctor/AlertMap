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
    
    // Початкове завантаження
    fetchStatus();
    fetchConfig();
    fetchStats();

    // Запуск періодичного оновлення
    startPolling();
}

// --- НАВІГАЦІЯ ПО ВКЛАДКАХ ---
function setupTabs() {
    const tabs = document.querySelectorAll('.tab');
    const hash = window.location.hash || '#home';
    
    function switchTab(targetId) {
        document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
        document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
        
        const pane = document.getElementById(targetId);
        if (pane) pane.classList.add('active');
        
        const tab = document.querySelector(`.tab[data-target="${targetId}"]`);
        if (tab) tab.classList.add('active');
        
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
            if (input && input.tagName === 'INPUT') {
                if (input.type === 'password') {
                    input.type = 'text';
                    btn.textContent = '🔒';
                } else {
                    input.type = 'password';
                    btn.textContent = '👁️';
                }
            }
        });
    });
}

// --- СПОВІЩЕННЯ (TOASTS) ---
function showToast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    
    let icon = 'ℹ️';
    if (type === 'success') icon = '✅';
    if (type === 'error') icon = '❌';
    if (type === 'warning') icon = '⚠️';
    
    toast.innerHTML = `<span class="toast-icon">${icon}</span><span>${message}</span>`;
    container.appendChild(toast);
    
    setTimeout(() => {
        toast.classList.add('fade-out');
        setTimeout(() => toast.remove(), 300);
    }, 4000);
}

// --- СТАТУС З'ЄДНАННЯ ---
function updateConnectionStatus(online) {
    isOnline = online;
    const dot = document.getElementById('conn-status');
    if (dot) {
        if (online) {
            dot.className = 'dot dot-green';
            dot.title = "Плата онлайн";
        } else {
            dot.className = 'dot dot-red';
            dot.title = "Немає зв'язку з картою";
        }
    }
}

// --- ДОПОМІЖНІ ФУНКЦІЇ ---
const hexToUint32 = hex => parseInt(hex.replace('#', ''), 16);
const uint32ToHex = num => '#' + ((num || 0) & 0xFFFFFF).toString(16).padStart(6, '0');

function formatUptime(sec) {
    if (!sec && sec !== 0) return '--';
    const d = Math.floor(sec / 86400);
    const h = Math.floor((sec % 86400) / 3600);
    const m = Math.floor((sec % 3600) / 60);
    const s = sec % 60;
    if (d > 0) return `${d}д ${h}г ${m}хв`;
    if (h > 0) return `${h}г ${m}хв ${s}с`;
    return `${m}хв ${s}с`;
}

function formatDuration(sec) {
    if (!sec) return '--';
    const h = Math.floor(sec / 3600);
    const m = Math.floor((sec % 3600) / 60);
    const s = sec % 60;
    if (h > 0) return `${h}г ${m}хв`;
    return `${m}хв ${s}с`;
}

// --- МОДАЛЬНЕ ВІКНО ПІДТВЕРДЖЕННЯ ---
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

// --- API ВИКЛИКИ ---
async function apiGet(endpoint) {
    try {
        const res = await fetch(endpoint, { cache: 'no-store' });
        if (!res.ok) throw new Error('Помилка сервера: ' + res.status);
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
        if (!res.ok) throw new Error('Помилка сервера: ' + res.status);
        updateConnectionStatus(true);
        return await res.json();
    } catch (err) {
        updateConnectionStatus(false);
        throw err;
    }
}

// --- ПЕРІОДИЧНЕ ОПИТУВАННЯ ---
function startPolling() {
    pollingIntervals.status = setInterval(fetchStatus, 4000);
    pollingIntervals.stats = setInterval(fetchStats, 10000);
}

async function fetchStatus() {
    try {
        const data = await apiGet('/api/status');
        
        // Шапка та версія
        if (data.version) {
            document.getElementById('header-version').textContent = 'v' + data.version;
            document.getElementById('fw-version').textContent = 'v' + data.version;
        }
        
        // Wi-Fi інформація
        const wifiEl = document.getElementById('dash-wifi');
        if (data.wifi_state === 3) {
            wifiEl.textContent = 'Точка доступу';
        } else if (data.wifi_ssid && data.wifi_ssid.length > 0) {
            wifiEl.textContent = data.wifi_ssid;
        } else {
            wifiEl.textContent = 'Немає з'єднання';
        }
        
        if (data.wifi_rssi) {
            document.getElementById('dash-rssi').textContent = `${data.wifi_rssi} dBm`;
        } else {
            document.getElementById('dash-rssi').textContent = '';
        }
        
        // Час та аптайм
        document.getElementById('dash-time').textContent = data.current_time || '--:--:--';
        document.getElementById('dash-uptime').textContent = formatUptime(data.uptime);
        document.getElementById('dash-mode').textContent = data.is_night ? '🌙 Ніч' : '☀️ День';
        
        if (data.free_heap) {
            document.getElementById('dash-heap').textContent = `RAM: ${(data.free_heap / 1024).toFixed(1)} KB`;
        }

        // Кількість тривог
        const missiles = data.active_missiles || 0;
        const drones = data.active_drones || 0;
        const totalAlerts = data.active_alerts || (missiles + drones);
        const peaceful = Math.max(0, 128 - totalAlerts);

        document.getElementById('sum-red').textContent = missiles;
        document.getElementById('sum-yellow').textContent = drones;
        document.getElementById('sum-green').textContent = peaceful;

        // Список останніх подій
        const elist = document.getElementById('event-list');
        if (data.recent_events && data.recent_events.length > 0) {
            elist.innerHTML = data.recent_events.map(e => {
                let badge = '🟢';
                let actionText = 'Відбій тривоги';
                if (e.level === 3) { badge = '🔴'; actionText = 'Ракетна небезпека'; }
                else if (e.level === 1) { badge = '🟡'; actionText = 'Загроза дронів'; }
                else if (e.level === 2) { badge = '🟠'; actionText = 'Часткова тривога'; }
                
                let timeStr = '';
                if (e.timestamp && e.timestamp > 100000) {
                    const d = new Date(e.timestamp * 1000);
                    timeStr = d.toLocaleTimeString('uk-UA');
                }
                
                return `<li>
                    <span class="district">${badge} <strong>${e.district}</strong> — ${actionText}</span>
                    <span class="time">${timeStr}</span>
                </li>`;
            }).join('');
        } else {
            elist.innerHTML = '<li class="empty">Наразі немає нових подій</li>';
        }
        
    } catch (e) {
        // Якщо запит не вдалося виконати, помилка вже оброблена в apiGet
    }
}

async function fetchStats() {
    try {
        const data = await apiGet('/api/stats');
        document.getElementById('stat-alertsToday').textContent = data.totalAlertsToday !== undefined ? data.totalAlertsToday : (data.alertsToday || 0);
        document.getElementById('stat-clearsToday').textContent = data.totalAllClearToday !== undefined ? data.totalAllClearToday : (data.clearsToday || 0);
        
        if (data.lastAlertTime && data.lastAlertTime > 100000) {
            const d = new Date(data.lastAlertTime * 1000);
            document.getElementById('stat-lastAlert').textContent = d.toLocaleTimeString('uk-UA');
        } else {
            document.getElementById('stat-lastAlert').textContent = 'Немає даних';
        }
        
        if (data.longestAlertDuration && data.longestAlertDuration > 0) {
            const dur = formatDuration(data.longestAlertDuration);
            const dist = data.longestAlertDistrict ? ` (${data.longestAlertDistrict})` : '';
            document.getElementById('stat-longestAlert').textContent = dur + dist;
        } else {
            document.getElementById('stat-longestAlert').textContent = 'Немає даних';
        }
        
        document.getElementById('stat-uptime').textContent = formatUptime(data.uptimeSeconds || data.uptime || 0);
        document.getElementById('stat-wifiReconnects').textContent = data.wifiReconnects || 0;
        document.getElementById('stat-apiRequests').textContent = data.apiRequests || 0;
        document.getElementById('stat-apiErrors').textContent = data.apiErrors || 0;
        document.getElementById('stat-freeHeap').textContent = data.freeHeap ? `${(data.freeHeap / 1024).toFixed(1)} KB` : '--';
    } catch (e) {}
}

async function fetchConfig() {
    try {
        configData = await apiGet('/api/config');
        populateConfigForm(configData);
    } catch (e) {
        console.error('Не вдалося завантажити конфігурацію', e);
    }
}

function populateConfigForm(cfg) {
    if (cfg.api_token) {
        document.getElementById('cfg-apiToken').value = cfg.api_token;
    }
    document.getElementById('cfg-updateInterval').value = cfg.poll_interval ? Math.round(cfg.poll_interval / 1000) : 10;
    
    document.getElementById('cfg-colorOk').value = uint32ToHex(cfg.color_ok !== undefined ? cfg.color_ok : 0x00FF00);
    document.getElementById('cfg-colorDrone').value = uint32ToHex(cfg.color_drones !== undefined ? cfg.color_drones : 0xFFFF00);
    document.getElementById('cfg-colorMissile').value = uint32ToHex(cfg.color_missiles !== undefined ? cfg.color_missiles : 0xFF0000);
    document.getElementById('cfg-colorPartial').value = uint32ToHex(cfg.color_partial !== undefined ? cfg.color_partial : 0xFF8800);
    document.getElementById('cfg-colorOffline').value = uint32ToHex(cfg.color_offline !== undefined ? cfg.color_offline : 0x0000FF);
    
    const dayB = cfg.brightness_day !== undefined ? cfg.brightness_day : 128;
    document.getElementById('cfg-dayBright').value = dayB;
    document.getElementById('val-dayBright').textContent = dayB;
    
    const nightB = cfg.brightness_night !== undefined ? cfg.brightness_night : 20;
    document.getElementById('cfg-nightBright').value = nightB;
    document.getElementById('val-nightBright').textContent = nightB;
    
    document.getElementById('cfg-nightMode').checked = cfg.night_mode_enabled !== false;
    document.getElementById('cfg-nightStart').value = cfg.night_start_hour !== undefined ? cfg.night_start_hour : 23;
    document.getElementById('cfg-nightEnd').value = cfg.night_end_hour !== undefined ? cfg.night_end_hour : 7;
    
    // Анімації
    document.getElementById('cfg-animMode').value = cfg.alert_animation_mode !== undefined ? cfg.alert_animation_mode : 0;
    document.getElementById('cfg-animSmooth').checked = cfg.smooth_transitions !== false;
    document.getElementById('cfg-animPulse').checked = cfg.pulse_on_alert !== false;
    document.getElementById('cfg-animWave').checked = cfg.wave_on_new_alert !== false;
    
    // LED та кольори
    document.getElementById('cfg-ledType').value = cfg.led_type !== undefined ? cfg.led_type : 1;
    document.getElementById('cfg-colorOrder').value = cfg.color_order !== undefined ? cfg.color_order : 0;
    
    // GitHub OTA
    const owner = cfg.github_owner || 'YellowDoctor';
    const repo = cfg.github_repo || 'AlertMap';
    document.getElementById('cfg-ghOwner').value = owner;
    document.getElementById('cfg-ghRepo').value = repo;
    document.getElementById('cfg-ghAutoCheck').checked = cfg.auto_check_updates || false;
    document.getElementById('gh-repo-display').textContent = `${owner}/${repo}`;
    
    // Wi-Fi
    if (cfg.wifi_ssid) {
        document.getElementById('wifi-ssid').value = cfg.wifi_ssid;
    }
}

// --- ОБРОБНИКИ ПОДІЙ ---
function setupEventListeners() {
    // Повзунки яскравості
    document.getElementById('cfg-dayBright').addEventListener('input', (e) => {
        document.getElementById('val-dayBright').textContent = e.target.value;
    });
    
    document.getElementById('cfg-nightBright').addEventListener('input', (e) => {
        document.getElementById('val-nightBright').textContent = e.target.value;
    });

    // Форма налаштувань
    document.getElementById('config-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const tokenInput = document.getElementById('cfg-apiToken').value.trim();
        const payload = {
            poll_interval: parseInt(document.getElementById('cfg-updateInterval').value) * 1000,
            
            color_ok: hexToUint32(document.getElementById('cfg-colorOk').value),
            color_drones: hexToUint32(document.getElementById('cfg-colorDrone').value),
            color_missiles: hexToUint32(document.getElementById('cfg-colorMissile').value),
            color_partial: hexToUint32(document.getElementById('cfg-colorPartial').value),
            color_offline: hexToUint32(document.getElementById('cfg-colorOffline').value),
            
            brightness_day: parseInt(document.getElementById('cfg-dayBright').value),
            brightness_night: parseInt(document.getElementById('cfg-nightBright').value),
            night_mode_enabled: document.getElementById('cfg-nightMode').checked,
            night_start_hour: parseInt(document.getElementById('cfg-nightStart').value),
            night_end_hour: parseInt(document.getElementById('cfg-nightEnd').value),
            
            alert_animation_mode: parseInt(document.getElementById('cfg-animMode').value),
            smooth_transitions: document.getElementById('cfg-animSmooth').checked,
            pulse_on_alert: document.getElementById('cfg-animPulse').checked,
            wave_on_new_alert: document.getElementById('cfg-animWave').checked,
            
            led_type: parseInt(document.getElementById('cfg-ledType').value),
            color_order: parseInt(document.getElementById('cfg-colorOrder').value),
            
            github_owner: document.getElementById('cfg-ghOwner').value.trim(),
            github_repo: document.getElementById('cfg-ghRepo').value.trim(),
            auto_check_updates: document.getElementById('cfg-ghAutoCheck').checked
        };
        
        // Передаємо токен лише якщо користувач ввів новий (не маскований)
        if (tokenInput && !tokenInput.startsWith('...')) {
            payload.api_token = tokenInput;
        }

        try {
            await apiPost('/api/config', payload);
            showToast('Налаштування успішно збережено!', 'success');
            document.getElementById('gh-repo-display').textContent = `${payload.github_owner}/${payload.github_repo}`;
        } catch (err) {
            showToast('Помилка збереження налаштувань', 'error');
        }
    });

    // Форма Wi-Fi
    document.getElementById('wifi-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const ssid = document.getElementById('wifi-ssid').value.trim();
        const pass = document.getElementById('wifi-pass').value;
        
        showConfirmModal(
            'Підключення до Wi-Fi',
            `Зберегти мережу "${ssid}" та перезапустити модуль зв'язку?`,
            async () => {
                try {
                    await apiPost('/api/wifi', { wifi_ssid: ssid, wifi_password: pass });
                    showToast('Налаштування Wi-Fi збережено! Перепідключення...', 'success');
                } catch (err) {
                    showToast('Помилка при збереженні Wi-Fi', 'error');
                }
            }
        );
    });

    // Тест світлодіодів
    document.getElementById('btn-test').addEventListener('click', async () => {
        try {
            await apiPost('/api/test/pattern', {});
            showToast('Запущено тестовий райдужний режим діодів', 'info');
        } catch (e) {
            showToast('Помилка запуску тесту', 'error');
        }
    });

    // Перезавантаження
    document.getElementById('btn-restart').addEventListener('click', () => {
        showConfirmModal(
            'Перезавантаження пристрою',
            'Ви впевнені, що хочете перезавантажити карту?',
            async () => {
                try {
                    await apiPost('/api/restart', {});
                    showToast('Карта перезавантажується... Зачекайте 10 секунд', 'warning');
                } catch (e) {
                    showToast('Помилка при відправці команди', 'error');
                }
            }
        );
    });

    // Перевірка GitHub OTA
    document.getElementById('btn-check-update').addEventListener('click', async () => {
        const btn = document.getElementById('btn-check-update');
        btn.disabled = true;
        btn.textContent = '⏳ Перевірка релізів на GitHub...';
        
        try {
            const data = await apiGet('/api/ota/check');
            const infoBox = document.getElementById('gh-update-info');
            const notesEl = document.getElementById('gh-release-notes');
            
            if (data.available) {
                notesEl.innerHTML = `<strong>Доступна нова версія: v${data.latest_version}</strong><br><br>${data.release_notes || 'Опис змін відсутній.'}`;
                infoBox.classList.remove('hidden');
                showToast(`Знайдено нову версію v${data.latest_version}!`, 'success');
            } else {
                infoBox.classList.add('hidden');
                showToast(`У вас встановлена найновіша версія (v${data.current_version || '1.0.0'})`, 'info');
            }
        } catch (e) {
            showToast('Не вдалося перевірити оновлення на GitHub', 'error');
        } finally {
            btn.disabled = false;
            btn.textContent = '🔍 Перевірити наявність оновлень';
        }
    });

    // Оновлення напряму з GitHub
    document.getElementById('btn-do-update').addEventListener('click', () => {
        showConfirmModal(
            'OTA Оновлення з GitHub',
            'Плата завантажить нову прошивку з GitHub та перезавантажиться. Не вимикайте живлення під час оновлення!',
            async () => {
                try {
                    await apiPost('/api/ota/github', {});
                    showToast('Завантаження прошивки з GitHub розпочато...', 'info');
                    trackOtaProgress();
                } catch (e) {
                    showToast('Помилка запуску OTA оновлення', 'error');
                }
            }
        );
    });

    // Ручне завантаження файлу прошивки
    document.getElementById('upload-form').addEventListener('submit', (e) => {
        e.preventDefault();
        const fileInput = document.getElementById('fw-file');
        if (!fileInput.files.length) return;
        
        const file = fileInput.files[0];
        showConfirmModal(
            'Прошивка файлу',
            `Завантажити та прошити "${file.name}" (${(file.size / 1024).toFixed(1)} KB)?`,
            () => {
                uploadFirmwareFile(file);
            }
        );
    });
}

// --- ВІДСТЕЖЕННЯ ПРОГРЕСУ OTA ---
function trackOtaProgress() {
    const container = document.getElementById('update-progress-container');
    const fill = document.getElementById('update-fill');
    const text = document.getElementById('update-text');
    
    container.classList.remove('hidden');
    fill.style.width = '0%';
    text.textContent = '0% — Завантаження...';
    
    const interval = setInterval(async () => {
        try {
            const data = await apiGet('/api/ota/progress');
            const percent = data.progress || 0;
            fill.style.width = `${percent}%`;
            text.textContent = `${percent}% — ${data.message || 'Оновлення...'}`;
            
            if (percent >= 100 || data.status === 'success') {
                clearInterval(interval);
                text.textContent = '100% — Успішно! Перезавантаження плати...';
                showToast('Оновлення успішно завершено! Перезавантаження...', 'success');
            } else if (data.status === 'error') {
                clearInterval(interval);
                text.textContent = 'Помилка оновлення: ' + (data.message || 'Невідома помилка');
                showToast('Помилка під час оновлення!', 'error');
            }
        } catch (e) {
            // При перезавантаженні зв'язок втрачається — це успіх
            clearInterval(interval);
            text.textContent = 'Оновлення завершено! Перезавантаження...';
        }
    }, 1500);
}

// --- ЗАВАНТАЖЕННЯ ФАЙЛУ ЧЕРЕЗ XHR ---
function uploadFirmwareFile(file) {
    const container = document.getElementById('update-progress-container');
    const fill = document.getElementById('update-fill');
    const text = document.getElementById('update-text');
    
    container.classList.remove('hidden');
    fill.style.width = '0%';
    text.textContent = '0% — Передача файлу на плату...';
    
    const xhr = new XMLHttpRequest();
    const formData = new FormData();
    formData.append('file', file);
    
    xhr.upload.addEventListener('progress', (e) => {
        if (e.lengthComputable) {
            const percent = Math.round((e.loaded / e.total) * 100);
            fill.style.width = `${percent}%`;
            text.textContent = `${percent}% — Завантаження (${(e.loaded / 1024).toFixed(0)} / ${(e.total / 1024).toFixed(0)} KB)...`;
        }
    });
    
    xhr.onload = () => {
        if (xhr.status === 200) {
            fill.style.width = '100%';
            text.textContent = '100% — Успішно записано! Перезавантаження...';
            showToast('Файл успішно записано! Карта перезавантажується...', 'success');
            setTimeout(() => location.reload(), 12000);
        } else {
            text.textContent = `Помилка запису (код ${xhr.status})`;
            showToast('Помилка при завантаженні файлу', 'error');
        }
    };
    
    xhr.onerror = () => {
        text.textContent = 'Помилка зв'язку під час завантаження';
        showToast('Помилка мережі при завантаженні', 'error');
    };
    
    xhr.open('POST', '/api/ota/upload');
    xhr.send(formData);
}
