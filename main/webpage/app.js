/**
 * Add gobals here
 */
var seconds 	= null;
var otaTimerVar =  null;

/**
 * Initialize functions here.
 */
$(document).ready(function(){
	getUpdateStatus();
    startDHTSensorInterval();
});   

function stopStatusPolling() {
    if (statusPollTimer) {
        clearInterval(statusPollTimer);
        statusPollTimer = null;
    }
}

/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() 
{
    var x = document.getElementById("selected_file");
    var file = x.files[0];

    document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
}

/**
 * Handles the firmware update.
 */
function updateFirmware() 
{
    // Form Data
    var formData = new FormData();
    var fileSelect = document.getElementById("selected_file");
    
    if (fileSelect.files && fileSelect.files.length == 1) 
	{
        var file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Http Request
        var request = new XMLHttpRequest();

        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");
        request.responseType = "blob";
        request.send(formData);

        // Start polling for status updates
        statusPollTimer = setInterval(getUpdateStatus, 1000);  // Poll every second
    } 
	else 
	{
        window.alert('Select A File First')
    }
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) 
{
    if (oEvent.lengthComputable) 
	{
        getUpdateStatus();
    } 
	else 
	{
        window.alert('total size is unknown')
    }
}

/**
 * Posts the firmware udpate status.
 */
function getUpdateStatus() {
    var xhr = new XMLHttpRequest();
    var requestURL = "/OTAstatus";
    xhr.open('POST', requestURL, false);
    xhr.send('ota_update_status');

    if (xhr.readyState == 4 && xhr.status == 200) {
        try {
            var response = JSON.parse(xhr.responseText);
            console.log("OTA Status Response:", response);  // Debug log
            
            document.getElementById("latest_firmware").innerHTML = response.compile_date + " - " + response.compile_time;

            // If flashing was complete it will return a 1, else -1
            // A return of 0 is just for information on the Latest Firmware request
            if (response.ota_update_status == 1) {
                console.log("OTA update successful, starting timer");  // Debug log
                // Set the countdown timer time
                seconds = 10;
                // Start the countdown timer
                otaRebootTimer();
                // Stop polling since we've received a successful status
                stopStatusPolling();
            } else if (response.ota_update_status == -1) {
                document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
                // Stop polling on error
                stopStatusPolling();
            }
        } catch (e) {
            console.error("Error parsing JSON response:", e);
        }
    }
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() {
    console.log("Reboot timer running, seconds remaining:", seconds);  // Debug log
    document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds == 0) {
        clearTimeout(otaTimerVar);
        window.location.reload();
    } else {
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}

/**
 * Gets DHT11 sensor temperature and humidity values for display on the web page. 
 */
function getDHTSensorValues()
{
    $.getJSON('/dhtSensor.json', function(data) {
        $("#temperature_reading").text(data["temperature"]);
        $("#humidity_reading").text(data["humidity"]);
    });
}

/**
 * Sets the interval for getting updated DHT11 sensor values.
 */
function startDHTSensorInterval()
{
    setInterval(getDHTSensorValues, 5000);
}
