// Configuration
let config = {
    deviceIP: localStorage.getItem('deviceIP') || '192.168.1.100',
    refreshRate: parseInt(localStorage.getItem('refreshRate')) || 5,
    criticalLevel: parseInt(localStorage.getItem('criticalLevel')) || 20,
    warningLevel: parseInt(localStorage.getItem('warningLevel')) || 50
};

let refreshInterval = null;
let isConnected = false;

// API Base URL
const getApiUrl = () => `http://${config.deviceIP}`;

// Initialize app
document.addEventListener('DOMContentLoaded', () => {
    initializeTabs();
    initializeButtons();
    initializeSliders();
    loadSettings();
    startAutoRefresh();
    loadSystemStatus();
});

// Tab Navigation
function initializeTabs() {
    const tabButtons = document.querySelectorAll('.tab-btn');
    const tabContents = document.querySelectorAll('.tab-content');

    tabButtons.forEach(button => {
        button.addEventListener('click', () => {
            const tabName = button.getAttribute('data-tab');
            
            // Remove active class from all
            tabButtons.forEach(btn => btn.classList.remove('active'));
            tabContents.forEach(content => content.classList.remove('active'));
            
            // Add active class to clicked
            button.classList.add('active');
            document.getElementById(tabName).classList.add('active');
            
            // Load specific data for tab
            if (tabName === 'camera') {
                loadCameraStream();
            } else if (tabName === 'servo') {
                loadServoStatus();
            }
        });
    });
}

// Initialize Buttons
function initializeButtons() {
    // Dashboard buttons
    document.getElementById('btnOpenDrain').addEventListener('click', () => controlDrain('open'));
    document.getElementById('btnCloseDrain').addEventListener('click', () => controlDrain('close'));
    document.getElementById('btnViewMap').addEventListener('click', viewMap);
    
    // Servo buttons (original tab)
    document.getElementById('btnArmOpen').addEventListener('click', () => controlArm('open'));
    document.getElementById('btnArmClose').addEventListener('click', () => controlArm('close'));
    document.getElementById('btnArmHome').addEventListener('click', () => controlArm('home'));
    document.getElementById('btnArmDemo').addEventListener('click', () => controlArm('demo'));
    
    // Servo buttons on camera/stream tab
    document.getElementById('btnArmOpenStream').addEventListener('click', () => controlArm('open'));
    document.getElementById('btnArmCloseStream').addEventListener('click', () => controlArm('close'));
    document.getElementById('btnArmHomeStream').addEventListener('click', () => controlArm('home'));
    
    // Camera button
    document.getElementById('btnRefreshCamera').addEventListener('click', loadCameraStream);
    
    // Settings button
    document.getElementById('btnSaveSettings').addEventListener('click', saveSettings);
}

// Initialize Servo Sliders
function initializeSliders() {
    // Original servo tab sliders (keep for compatibility)
    const sliders = {
        base: { slider: document.getElementById('baseSlider'), value: document.getElementById('baseValue') },
        shoulder: { slider: document.getElementById('shoulderSlider'), value: document.getElementById('shoulderValue') },
        elbow: { slider: document.getElementById('elbowSlider'), value: document.getElementById('elbowValue') },
        gripper: { slider: document.getElementById('gripperSlider'), value: document.getElementById('gripperValue') }
    };

    Object.keys(sliders).forEach(servo => {
        const { slider, value } = sliders[servo];
        
        slider.addEventListener('input', (e) => {
            value.textContent = e.target.value;
        });
        
        slider.addEventListener('change', (e) => {
            controlServo(servo, e.target.value);
        });
    });
    
    // Initialize joystick controls
    initializeJoysticks();
}

// Joystick Controller
let currentPositions = {
    base: 330,
    shoulder: 150,
    elbow: 300,
    gripper: 410
};

const servoLimits = {
    base: { min: 150, max: 450 },
    shoulder: { min: 150, max: 380 },
    elbow: { min: 300, max: 380 },
    gripper: { min: 410, max: 510 }
};

function initializeJoysticks() {
    const mainStick = document.getElementById('stickMain');
    createJoystick(mainStick, handleMainJoystick);
    
    // Elbow button controls
    document.getElementById('btnForward').addEventListener('mousedown', () => startContinuousMove('elbow', 8));
    document.getElementById('btnForward').addEventListener('mouseup', stopContinuousMove);
    document.getElementById('btnForward').addEventListener('touchstart', () => startContinuousMove('elbow', 8));
    document.getElementById('btnForward').addEventListener('touchend', stopContinuousMove);
    
    document.getElementById('btnReverse').addEventListener('mousedown', () => startContinuousMove('elbow', -8));
    document.getElementById('btnReverse').addEventListener('mouseup', stopContinuousMove);
    document.getElementById('btnReverse').addEventListener('touchstart', () => startContinuousMove('elbow', -8));
    document.getElementById('btnReverse').addEventListener('touchend', stopContinuousMove);
    
    // Single claw button - toggle between open and close
    let clawState = 'closed'; // Track current state
    const btnClaw = document.getElementById('btnClaw');
    
    const toggleClaw = () => {
        if (clawState === 'closed') {
            // Open the claw
            controlServo('gripper', 510); // Max open
            btnClaw.innerHTML = '<i class="fas fa-hand-paper"></i><span>OPEN</span>';
            clawState = 'open';
        } else {
            // Close the claw
            controlServo('gripper', 410); // Min close
            btnClaw.innerHTML = '<i class="fas fa-hand-rock"></i><span>GRIP</span>';
            clawState = 'closed';
        }
    };
    
    btnClaw.addEventListener('click', toggleClaw);
}

let continuousMoveInterval = null;

function startContinuousMove(servo, delta) {
    // Immediate first move
    moveServo(servo, delta);
    
    // Continue moving while button held
    continuousMoveInterval = setInterval(() => {
        moveServo(servo, delta);
    }, 100);
}

function stopContinuousMove() {
    if (continuousMoveInterval) {
        clearInterval(continuousMoveInterval);
        continuousMoveInterval = null;
    }
}

function createJoystick(stick, handler) {
    let isDragging = false;
    let startX, startY;
    const maxDistance = 60; // pixels from center
    
    const onStart = (e) => {
        isDragging = true;
        const rect = stick.parentElement.getBoundingClientRect();
        startX = rect.left + rect.width / 2;
        startY = rect.top + rect.height / 2;
        stick.style.transition = 'none';
    };
    
    const onMove = (e) => {
        if (!isDragging) return;
        
        e.preventDefault();
        
        const clientX = e.touches ? e.touches[0].clientX : e.clientX;
        const clientY = e.touches ? e.touches[0].clientY : e.clientY;
        
        let deltaX = clientX - startX;
        let deltaY = clientY - startY;
        
        // Limit distance
        const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
        if (distance > maxDistance) {
            const angle = Math.atan2(deltaY, deltaX);
            deltaX = Math.cos(angle) * maxDistance;
            deltaY = Math.sin(angle) * maxDistance;
        }
        
        stick.style.transform = `translate(${deltaX}px, ${deltaY}px)`;
        
        // Calculate direction
        const normalizedX = deltaX / maxDistance;
        const normalizedY = -deltaY / maxDistance; // Invert Y
        
        handler(normalizedX, normalizedY);
    };
    
    const onEnd = () => {
        if (!isDragging) return;
        isDragging = false;
        stick.style.transition = 'transform 0.2s';
        stick.style.transform = 'translate(0, 0)';
        handler(0, 0); // Return to center
    };
    
    // Mouse events
    stick.addEventListener('mousedown', onStart);
    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup', onEnd);
    
    // Touch events
    stick.addEventListener('touchstart', onStart);
    document.addEventListener('touchmove', onMove, { passive: false });
    document.addEventListener('touchend', onEnd);
}

let lastUpdate = Date.now();
const updateInterval = 100; // ms

function handleMainJoystick(x, y) {
    const now = Date.now();
    if (now - lastUpdate < updateInterval) return;
    lastUpdate = now;
    
    // X axis: Base rotation (left/right)
    if (Math.abs(x) > 0.2) {
        moveServo('base', x * 10);
    }
    
    // Y axis: Shoulder (up/down)
    if (Math.abs(y) > 0.2) {
        moveServo('shoulder', y * 10);
    }
}

async function moveServo(servo, delta) {
    // Calculate new position
    let newPos = currentPositions[servo] + delta;
    
    // Apply limits
    newPos = Math.max(servoLimits[servo].min, Math.min(servoLimits[servo].max, newPos));
    
    // Only update if changed significantly
    if (Math.abs(newPos - currentPositions[servo]) < 2) return;
    
    // Update current position
    currentPositions[servo] = Math.round(newPos);
    
    // Send command to ESP32
    try {
        await controlServo(servo, currentPositions[servo]);
    } catch (error) {
        // Silently fail to avoid flooding console
    }
}

// API Calls
async function fetchAPI(endpoint, method = 'GET', body = null) {
    try {
        const options = {
            method,
            headers: { 'Content-Type': 'application/json' }
        };
        
        if (body) {
            options.body = JSON.stringify(body);
        }
        
        const response = await fetch(`${getApiUrl()}${endpoint}`, options);
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        updateConnectionStatus(true);
        
        // Return text for endpoints that don't return JSON
        const contentType = response.headers.get('content-type');
        if (contentType && contentType.includes('application/json')) {
            return await response.json();
        } else {
            return await response.text();
        }
    } catch (error) {
        console.error('API Error:', error);
        updateConnectionStatus(false);
        // Don't show notification for every failed request (too noisy)
        throw error;
    }
}

// Load System Status
async function loadSystemStatus() {
    try {
        const status = await fetchAPI('/api/status');
        updateDashboard(status);
    } catch (error) {
        console.error('Failed to load status:', error);
    }
}

// Update Dashboard
function updateDashboard(data) {
    // Water level
    const waterLevel = data.water_level || 0;
    const distance = data.distance || 0;
    
    document.getElementById('waterLevel').textContent = waterLevel.toFixed(1);
    document.getElementById('distance').textContent = distance.toFixed(1);
    
    // Progress bar
    const percentage = Math.min((waterLevel / 200) * 100, 100);
    const progressFill = document.getElementById('progressFill');
    progressFill.style.width = `${percentage}%`;
    
    // Status color
    const levelStatus = document.getElementById('levelStatus');
    if (waterLevel > config.criticalLevel + 100) {
        levelStatus.textContent = 'CRITICAL';
        levelStatus.className = 'level-status critical';
        progressFill.style.background = 'linear-gradient(90deg, #F44336, #E57373)';
        document.getElementById('waterLevel').style.color = '#F44336';
    } else if (waterLevel > config.warningLevel + 50) {
        levelStatus.textContent = 'WARNING';
        levelStatus.className = 'level-status warning';
        progressFill.style.background = 'linear-gradient(90deg, #FF9800, #FFB74D)';
        document.getElementById('waterLevel').style.color = '#FF9800';
    } else {
        levelStatus.textContent = 'NORMAL';
        levelStatus.className = 'level-status normal';
        progressFill.style.background = 'linear-gradient(90deg, #4CAF50, #66BB6A)';
        document.getElementById('waterLevel').style.color = '#4CAF50';
    }
    
    // Drain status
    const drainStatus = document.getElementById('drainStatus');
    if (data.drain_open) {
        drainStatus.textContent = 'OPEN';
        drainStatus.className = 'status-badge open';
    } else {
        drainStatus.textContent = 'CLOSED';
        drainStatus.className = 'status-badge closed';
    }
    
    // GPS data
    document.getElementById('latitude').textContent = data.latitude ? data.latitude.toFixed(6) : 'N/A';
    document.getElementById('longitude').textContent = data.longitude ? data.longitude.toFixed(6) : 'N/A';
    document.getElementById('satellites').textContent = data.satellites || 0;
}

// Control Drain
async function controlDrain(action) {
    try {
        await fetchAPI(`/api/drain/${action}`, 'POST');
        showNotification(`Drain ${action} command sent`, 'success');
        setTimeout(loadSystemStatus, 1000);
    } catch (error) {
        showNotification(`Failed to ${action} drain`, 'danger');
    }
}

// Control Arm
async function controlArm(action) {
    try {
        await fetchAPI(`/api/arm/${action}`, 'POST');
        showNotification(`Arm ${action} command sent`, 'success');
        if (action === 'demo') {
            showNotification('Running demo sequence...', 'info');
        }
    } catch (error) {
        showNotification(`Failed to execute arm ${action}`, 'danger');
    }
}

// Control Individual Servo
async function controlServo(servo, position) {
    try {
        await fetchAPI(`/api/servo/${servo}?position=${position}`, 'POST');
    } catch (error) {
        console.error(`Failed to control ${servo}:`, error);
    }
}

// Load Servo Status
async function loadServoStatus() {
    try {
        const status = await fetchAPI('/api/servo/status');
        document.getElementById('baseSlider').value = status.base;
        document.getElementById('baseValue').textContent = status.base;
        document.getElementById('shoulderSlider').value = status.shoulder;
        document.getElementById('shoulderValue').textContent = status.shoulder;
        document.getElementById('elbowSlider').value = status.elbow;
        document.getElementById('elbowValue').textContent = status.elbow;
        document.getElementById('gripperSlider').value = status.gripper;
        document.getElementById('gripperValue').textContent = status.gripper;
    } catch (error) {
        console.error('Failed to load servo status:', error);
    }
}

// Load Camera Stream
async function loadCameraStream() {
    try {
        const data = await fetchAPI('/api/camera/stream');
        const cameraImg = document.getElementById('cameraStream');
        const cameraOverlay = document.getElementById('cameraOverlay');
        
        if (data.stream_url) {
            cameraImg.src = data.stream_url;
            cameraImg.onload = () => {
                cameraOverlay.classList.add('hidden');
            };
            cameraImg.onerror = () => {
                cameraOverlay.classList.remove('hidden');
            };
        }
    } catch (error) {
        console.error('Failed to load camera stream:', error);
        document.getElementById('cameraOverlay').classList.remove('hidden');
    }
}

// View Map
function viewMap() {
    const lat = document.getElementById('latitude').textContent;
    const lon = document.getElementById('longitude').textContent;
    
    if (lat !== 'N/A' && lon !== 'N/A') {
        const url = `https://www.google.com/maps?q=${lat},${lon}`;
        window.open(url, '_blank');
    } else {
        showNotification('GPS location not available', 'warning');
    }
}

// Settings
function loadSettings() {
    document.getElementById('deviceIP').value = config.deviceIP;
    document.getElementById('refreshRate').value = config.refreshRate;
    document.getElementById('criticalLevel').value = config.criticalLevel;
    document.getElementById('warningLevel').value = config.warningLevel;
}

function saveSettings() {
    config.deviceIP = document.getElementById('deviceIP').value;
    config.refreshRate = parseInt(document.getElementById('refreshRate').value);
    config.criticalLevel = parseInt(document.getElementById('criticalLevel').value);
    config.warningLevel = parseInt(document.getElementById('warningLevel').value);
    
    localStorage.setItem('deviceIP', config.deviceIP);
    localStorage.setItem('refreshRate', config.refreshRate);
    localStorage.setItem('criticalLevel', config.criticalLevel);
    localStorage.setItem('warningLevel', config.warningLevel);
    
    showNotification('Settings saved successfully', 'success');
    
    // Restart auto refresh
    stopAutoRefresh();
    startAutoRefresh();
    loadSystemStatus();
}

// Auto Refresh
function startAutoRefresh() {
    if (refreshInterval) {
        clearInterval(refreshInterval);
    }
    
    refreshInterval = setInterval(() => {
        const activeTab = document.querySelector('.tab-content.active').id;
        if (activeTab === 'dashboard') {
            loadSystemStatus();
        }
    }, config.refreshRate * 1000);
}

function stopAutoRefresh() {
    if (refreshInterval) {
        clearInterval(refreshInterval);
        refreshInterval = null;
    }
}

// Connection Status
function updateConnectionStatus(connected) {
    isConnected = connected;
    const statusDot = document.getElementById('connectionStatus');
    const statusText = document.getElementById('connectionText');
    
    if (connected) {
        statusDot.classList.remove('disconnected');
        statusText.textContent = 'Connected';
    } else {
        statusDot.classList.add('disconnected');
        statusText.textContent = 'Disconnected';
    }
}

// Notifications
function showNotification(message, type = 'info') {
    // Create notification element
    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    notification.textContent = message;
    notification.style.cssText = `
        position: fixed;
        top: 80px;
        right: 20px;
        padding: 16px 24px;
        background: ${type === 'success' ? '#4CAF50' : type === 'danger' ? '#F44336' : type === 'warning' ? '#FF9800' : '#2196F3'};
        color: white;
        border-radius: 8px;
        box-shadow: 0 4px 12px rgba(0,0,0,0.3);
        z-index: 1000;
        animation: slideIn 0.3s ease;
    `;
    
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.style.animation = 'slideOut 0.3s ease';
        setTimeout(() => notification.remove(), 300);
    }, 3000);
}

// Add animation styles
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from { transform: translateX(400px); opacity: 0; }
        to { transform: translateX(0); opacity: 1; }
    }
    @keyframes slideOut {
        from { transform: translateX(0); opacity: 1; }
        to { transform: translateX(400px); opacity: 0; }
    }
`;
document.head.appendChild(style);
