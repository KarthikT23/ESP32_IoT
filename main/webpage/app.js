/**
 * ESP32 Control Panel - Modern JavaScript Implementation
 * Using ES6+ features, modern DOM APIs, and improved UX
 */

class ESP32Controller {
    constructor() {
        this.intervals = new Map();
        this.statusPollTimer = null;
        this.wifiConnectInterval = null;
        this.otaTimerVar = null;
        this.seconds = null;
        
        this.init();
    }

    /**
     * Initialize the application
     */
    init() {
        document.addEventListener('DOMContentLoaded', () => {
            this.setupEventListeners();
            this.startPeriodicUpdates();
            this.setupFileUpload();
            this.getInitialData();
        });
    }

    /**
     * Setup all event listeners
     */
    setupEventListeners() {
        // WiFi connection
        const connectBtn = document.getElementById('connect_wifi');
        const disconnectBtn = document.getElementById('disconnect_wifi');
        const passwordToggle = document.getElementById('password-toggle');
        const updateFirmwareBtn = document.getElementById('update-firmware-btn');

        connectBtn?.addEventListener('click', () => this.handleWifiConnect());
        disconnectBtn?.addEventListener('click', () => this.handleWifiDisconnect());
        passwordToggle?.addEventListener('click', () => this.togglePasswordVisibility());
        updateFirmwareBtn?.addEventListener('click', () => this.handleFirmwareUpdate());

        // Enter key support for forms
        const ssidInput = document.getElementById('connect_ssid');
        const passInput = document.getElementById('connect_pass');
        
        [ssidInput, passInput].forEach(input => {
            input?.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    this.handleWifiConnect();
                }
            });
        });
    }

    /**
     * Setup drag and drop file upload
     */
    setupFileUpload() {
        const fileUploadArea = document.getElementById('file-upload-area');
        const fileInput = document.getElementById('selected_file');
        const updateBtn = document.getElementById('update-firmware-btn');

        // Click to select file
        fileUploadArea?.addEventListener('click', () => fileInput?.click());

        // File input change
        fileInput?.addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (file) {
                this.displayFileInfo(file);
                updateBtn.disabled = false;
            }
        });

        // Drag and drop functionality
        fileUploadArea?.addEventListener('dragover', (e) => {
            e.preventDefault();
            fileUploadArea.classList.add('dragover');
        });

        fileUploadArea?.addEventListener('dragleave', () => {
            fileUploadArea.classList.remove('dragover');
        });

        fileUploadArea?.addEventListener('drop', (e) => {
            e.preventDefault();
            fileUploadArea.classList.remove('dragover');
            
            const files = e.dataTransfer.files;
            if (files.length === 1 && files[0].name.endsWith('.bin')) {
                fileInput.files = files;
                this.displayFileInfo(files[0]);
                updateBtn.disabled = false;
            } else {
                this.showStatusMessage('Please select a valid .bin file', 'error');
            }
        });
    }

    /**
     * Get initial data when page loads
     */
    async getInitialData() {
        try {
            await Promise.all([
                this.getUpdateStatus(),
                this.getSSID(),
                this.getConnectionInfo()
            ]);
        } catch (error) {
            console.error('Error loading initial data:', error);
        }
    }

    /**
     * Start periodic data updates
     */
    startPeriodicUpdates() {
        // DHT sensor readings every 5 seconds
        this.intervals.set('dht', setInterval(() => this.getDHTSensorValues(), 5000));
        
        // Local time every 10 seconds
        this.intervals.set('time', setInterval(() => this.getLocalTime(), 10000));
        
        // Get initial readings immediately
        this.getDHTSensorValues();
        this.getLocalTime();
    }

    /**
     * Modern fetch wrapper with error handling
     */
    async fetchJSON(url, options = {}) {
        try {
            const response = await fetch(url, {
                headers: {
                    'Content-Type': 'application/json',
                    ...options.headers
                },
                ...options
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            return await response.json();
        } catch (error) {
            console.error(`Fetch error for ${url}:`, error);
            throw error;
        }
    }

    /**
     * Get DHT11 sensor values
     */
    async getDHTSensorValues() {
        try {
            const data = await this.fetchJSON('/dhtSensor.json');
            
            const tempElement = document.getElementById('temperature_reading');
            const humidityElement = document.getElementById('humidity_reading');
            
            if (tempElement) {
                tempElement.textContent = data.temperature || '--';
                this.animateValue(tempElement);
            }
            
            if (humidityElement) {
                humidityElement.textContent = data.humidity || '--';
                this.animateValue(humidityElement);
            }
        } catch (error) {
            console.error('Error getting DHT sensor values:', error);
            document.getElementById('temperature_reading').textContent = 'Error';
            document.getElementById('humidity_reading').textContent = 'Error';
        }
    }

    /**
     * Get local time
     */
    async getLocalTime() {
        try {
            const data = await this.fetchJSON('/localTime.json');
            const timeElement = document.getElementById('local_time');
            
            if (timeElement) {
                timeElement.textContent = data.time || 'Time not available';
            }
        } catch (error) {
            console.error('Error getting local time:', error);
            document.getElementById('local_time').textContent = 'Failed to get time';
        }
    }

    /**
     * Get ESP32 SSID
     */
    async getSSID() {
        try {
            const data = await this.fetchJSON('/apSSID.json');
            const ssidElement = document.getElementById('ap_ssid');
            
            if (ssidElement) {
                ssidElement.textContent = data.ssid || 'Unknown';
            }
        } catch (error) {
            console.error('Error getting SSID:', error);
            document.getElementById('ap_ssid').textContent = 'Error';
        }
    }

    /**
     * Get firmware update status
     */
    async getUpdateStatus() {
        try {
            const response = await fetch('/OTAstatus', {
                method: 'POST',
                body: 'ota_update_status'
            });

            if (response.ok) {
                const data = await response.json();
                const firmwareElement = document.getElementById('latest_firmware');
                
                if (firmwareElement && data.compile_date && data.compile_time) {
                    firmwareElement.textContent = `${data.compile_date} - ${data.compile_time}`;
                }

                // Handle OTA status
                if (data.ota_update_status === 1) {
                    this.handleOTASuccess();
                } else if (data.ota_update_status === -1) {
                    this.showStatusMessage('Upload Error!', 'error', 'ota_update_status');
                    this.stopStatusPolling();
                }
            }
        } catch (error) {
            console.error('Error getting update status:', error);
        }
    }

    /**
     * Handle successful OTA update
     */
    handleOTASuccess() {
        console.log('OTA update successful, starting timer');
        this.seconds = 10;
        this.startOTARebootTimer();
        this.stopStatusPolling();
    }

    /**
     * Start OTA reboot countdown timer
     */
    startOTARebootTimer() {
        const statusElement = document.getElementById('ota_update_status');
        
        const countdown = () => {
            if (statusElement) {
                statusElement.innerHTML = `
                    <div class="status-message success">
                        <i class="fas fa-check-circle"></i>
                        OTA Firmware Update Complete. Page will reload in: ${this.seconds}s
                    </div>
                `;
            }

            if (--this.seconds <= 0) {
                clearTimeout(this.otaTimerVar);
                window.location.reload();
            } else {
                this.otaTimerVar = setTimeout(countdown, 1000);
            }
        };

        countdown();
    }

    /**
     * Handle firmware update
     */
    async handleFirmwareUpdate() {
        const fileInput = document.getElementById('selected_file');
        const updateBtn = document.getElementById('update-firmware-btn');
        const progressContainer = document.getElementById('progress-container');
        const progressFill = document.getElementById('progress-fill');
        const progressText = document.getElementById('progress-text');

        if (!fileInput.files || fileInput.files.length !== 1) {
            this.showStatusMessage('Please select a firmware file first', 'error', 'ota_update_status');
            return;
        }

        const file = fileInput.files[0];
        const formData = new FormData();
        formData.set('file', file, file.name);

        // Update UI
        updateBtn.disabled = true;
        updateBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Uploading...';
        progressContainer.style.display = 'block';
        
        this.showStatusMessage(`Uploading ${file.name}, Firmware Update in Progress...`, 'info', 'ota_update_status');

        try {
            const xhr = new XMLHttpRequest();

            // Progress tracking
            xhr.upload.addEventListener('progress', (e) => {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    progressFill.style.width = `${percent}%`;
                    progressText.textContent = `${percent}%`;
                }
            });

            // Setup promise for xhr
            const uploadPromise = new Promise((resolve, reject) => {
                xhr.onload = () => {
                    if (xhr.status === 200) {
                        resolve();
                    } else {
                        reject(new Error(`Upload failed: ${xhr.status}`));
                    }
                };
                xhr.onerror = () => reject(new Error('Upload failed'));
            });

            xhr.open('POST', '/OTAupdate');
            xhr.responseType = 'blob';
            xhr.send(formData);

            await uploadPromise;

            // Start polling for status
            this.statusPollTimer = setInterval(() => this.getUpdateStatus(), 1000);

        } catch (error) {
            console.error('Firmware update error:', error);
            this.showStatusMessage('Upload failed. Please try again.', 'error', 'ota_update_status');
            
            // Reset UI
            updateBtn.disabled = false;
            updateBtn.innerHTML = '<i class="fas fa-upload"></i> Update Firmware';
            progressContainer.style.display = 'none';
        }
    }

    /**
     * Stop status polling
     */
    stopStatusPolling() {
        if (this.statusPollTimer) {
            clearInterval(this.statusPollTimer);
            this.statusPollTimer = null;
        }
    }

    /**
     * Display file information
     */
    displayFileInfo(file) {
        const fileInfoElement = document.getElementById('file_info');
        const fileSize = this.formatFileSize(file.size);
        
        if (fileInfoElement) {
            fileInfoElement.innerHTML = `
                <div class="file-info">
                    <i class="fas fa-file"></i>
                    <strong>File:</strong> ${file.name}<br>
                    <strong>Size:</strong> ${fileSize}
                </div>
            `;
        }
    }

    /**
     * Format file size in human readable format
     */
    formatFileSize(bytes) {
        if (bytes === 0) return '0 Bytes';
        
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    /**
     * Handle WiFi connection
     */
    async handleWifiConnect() {
        const ssid = document.getElementById('connect_ssid').value.trim();
        const password = document.getElementById('connect_pass').value.trim();
        const connectBtn = document.getElementById('connect_wifi');
        const errorContainer = document.getElementById('wifi_connect_credentials_errors');

        // Clear previous errors
        errorContainer.innerHTML = '';

        // Validation
        const errors = [];
        if (!ssid) errors.push('SSID cannot be empty!');
        if (!password) errors.push('Password cannot be empty!');

        if (errors.length > 0) {
            errorContainer.innerHTML = errors.map(error => 
                `<div class="error-messages"><i class="fas fa-exclamation-triangle"></i> ${error}</div>`
            ).join('');
            return;
        }

        // Update UI
        connectBtn.disabled = true;
        connectBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Connecting...';

        try {
            await fetch('/wifiConnect.json', {
                method: 'POST',
                headers: {
                    'my-connect-ssid': ssid,
                    'my-connect-pwd': password,
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ timestamp: Date.now() })
            });

            this.startWifiStatusPolling();
        } catch (error) {
            console.error('WiFi connection error:', error);
            this.showStatusMessage('Connection request failed', 'error', 'wifi_connect_status');
            
            // Reset button
            connectBtn.disabled = false;
            connectBtn.innerHTML = '<i class="fas fa-plug"></i> Connect';
        }
    }

    /**
     * Start WiFi connection status polling
     */
    startWifiStatusPolling() {
        this.wifiConnectInterval = setInterval(() => this.checkWifiStatus(), 2800);
        this.showStatusMessage('Connecting...', 'info', 'wifi_connect_status');
    }

    /**
     * Check WiFi connection status
     */
    async checkWifiStatus() {
        try {
            const response = await fetch('/wifiConnectStatus.json', {
                method: 'POST',
                body: 'wifi_connect_status'
            });

            if (response.ok) {
                const data = await response.json();
                const connectBtn = document.getElementById('connect_wifi');

                if (data.wifi_connect_status === 2) {
                    // Connection failed
                    this.showStatusMessage('Failed to Connect. Please check your AP credentials and compatibility', 'error', 'wifi_connect_status');
                    this.stopWifiStatusPolling();
                    
                    // Reset button
                    connectBtn.disabled = false;
                    connectBtn.innerHTML = '<i class="fas fa-plug"></i> Connect';
                    
                } else if (data.wifi_connect_status === 3) {
                    // Connection successful
                    this.showStatusMessage('Connection Success!', 'success', 'wifi_connect_status');
                    this.stopWifiStatusPolling();
                    
                    // Reset button
                    connectBtn.disabled = false;
                    connectBtn.innerHTML = '<i class="fas fa-plug"></i> Connect';
                    
                    // Get connection info
                    setTimeout(() => this.getConnectionInfo(), 1000);
                }
            }
        } catch (error) {
            console.error('Error checking WiFi status:', error);
        }
    }

    /**
     * Stop WiFi status polling
     */
    stopWifiStatusPolling() {
        if (this.wifiConnectInterval) {
            clearInterval(this.wifiConnectInterval);
            this.wifiConnectInterval = null;
        }
    }

    /**
     * Get WiFi connection information
     */
    async getConnectionInfo() {
        try {
            const data = await this.fetchJSON('/wifiConnectInfo.json');
            
            // Update connection info
            document.getElementById('connected_ap').textContent = data.ap || 'Unknown';
            document.getElementById('wifi_connect_ip').textContent = data.ip || 'Unknown';
            document.getElementById('wifi_connect_netmask').textContent = data.netmask || 'Unknown';
            document.getElementById('wifi_connect_gw').textContent = data.gw || 'Unknown';
            
            // Show connection info section
            const connectionInfo = document.getElementById('connection-info');
            if (connectionInfo) {
                connectionInfo.style.display = 'block';
                connectionInfo.classList.add('fade-in');
            }
            
        } catch (error) {
            console.error('Error getting connection info:', error);
        }
    }

    /**
     * Handle WiFi disconnection
     */
    async handleWifiDisconnect() {
        const disconnectBtn = document.getElementById('disconnect_wifi');
        
        disconnectBtn.disabled = true;
        disconnectBtn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Disconnecting...';

        try {
            await fetch('/wifiDisconnect.json', {
                method: 'DELETE',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ timestamp: Date.now() })
            });

            // Hide connection info and reload page
            setTimeout(() => {
                window.location.reload();
            }, 2000);

        } catch (error) {
            console.error('WiFi disconnect error:', error);
            disconnectBtn.disabled = false;
            disconnectBtn.innerHTML = '<i class="fas fa-unlink"></i> Disconnect';
        }
    }

    /**
     * Toggle password visibility
     */
    togglePasswordVisibility() {
        const passwordInput = document.getElementById('connect_pass');
        const toggleBtn = document.getElementById('password-toggle');
        const icon = toggleBtn.querySelector('i');

        if (passwordInput.type === 'password') {
            passwordInput.type = 'text';
            icon.className = 'fas fa-eye-slash';
        } else {
            passwordInput.type = 'password';
            icon.className = 'fas fa-eye';
        }
    }

    /**
     * Show status message with styling
     */
    showStatusMessage(message, type = 'info', elementId = null) {
        const element = elementId ? document.getElementById(elementId) : null;
        
        if (element) {
            const iconMap = {
                success: 'fas fa-check-circle',
                error: 'fas fa-exclamation-circle',
                info: 'fas fa-info-circle'
            };

            element.innerHTML = `
                <div class="status-message ${type}">
                    <i class="${iconMap[type]}"></i>
                    ${message}
                </div>
            `;
        }
    }

    /**
     * Animate value changes
     */
    animateValue(element) {
        element.style.transform = 'scale(1.1)';
        element.style.transition = 'transform 0.2s ease';
        
        setTimeout(() => {
            element.style.transform = 'scale(1)';
        }, 200);
    }

    /**
     * Cleanup intervals on page unload
     */
    cleanup() {
        this.intervals.forEach((interval) => clearInterval(interval));
        this.intervals.clear();
        
        if (this.statusPollTimer) clearInterval(this.statusPollTimer);
        if (this.wifiConnectInterval) clearInterval(this.wifiConnectInterval);
        if (this.otaTimerVar) clearTimeout(this.otaTimerVar);
    }
}

// Initialize the application
const esp32Controller = new ESP32Controller();

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    esp32Controller.cleanup();
});
