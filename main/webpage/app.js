/**
 * Add globals here
 */
var seconds 	= null;
var otaTimerVar =  null;
var wifiConnectInterval = null;

/**
 * Initialize functions here.
 */
$(document).ready(function(){
	getUpdateStatus();
    startDHTSensorInterval();
    startLocalTimeInterval();
    getSSID();
    getConnectInfo();
    $("#connect_wifi").on("click", function(){
        checkCredentials();
    });
    $("#disconnect_wifi").on("click", function(){
        disconnectWifi();
    });
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

/**
 * Clears the connection status interval
 */
function stopWifiConnectStatusInterval()
{
    if (wifiConnectInterval != null)
    {
        clearInterval(wifiConnectInterval);
        wifiConnectInterval = null;
    }
}

/**
 * Gets the WiFi connection status
 */
function getWifiConnectStatus()
{
    var xhr = new XMLHttpRequest();
    var requestURL = "/wifiConnectStatus.json";
    xhr.open('POST', requestURL, false);
    xhr.send('wifi_connect_status');

    if (xhr.readyState == 4 && xhr.status == 200)
    {
        var response = JSON.parse(xhr.responseText);

        document.getElementById("wifi_connect_status").innerHTML = "Connecting...";

        if (response.wifi_connect_status == 2)
        {
            document.getElementById("wifi_connect_status").innerHTML = "<h4 class='rd'>Failed to Connect. Please check your AP credentials and compatibility</h4>";
            stopWifiConnectStatusInterval();
        }
        else if (response.wifi_connect_status == 3)
        {
            document.getElementById("wifi_connect_status").innerHTML = "<h4 class='gr'>Connection Success!</h4>";
            stopWifiConnectStatusInterval();
            getConnectInfo();
        }
    }
}

/**
 * Starts the interval for checking the connection status
 */
function startWifiConnectStatusInterval()
{
    wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

/**
 * Connect WiFi function called using the SSID and password entered into the text fields
 */
function connectWifi()
{
    // Get the SSID and Password
    selectedSSID = $("#connect_ssid").val();
    pwd = $("#connect_pass").val();

    $.ajax({
        url: '/wifiConnect.json',
        dataType: 'json',
        method: 'POST',
        cache: false,
        headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
        data: {'timestamp': Date.now()}
    });

    startWifiConnectStatusInterval();
}

/**
 * Checks credentials on connect_wifi button click
 */
function checkCredentials()
{
    errorList = "";
    credsOk = true;

    selectedSSID = $("#connect_ssid").val();
    pwd = $("#connect_pass").val();

    if (selectedSSID == "")
    {
        errorList += "<h4 class ='rd'>SSID cannot be empty!</h4>";
        credsOk = false;
    }
    if (pwd == "")
    {
        errorList += "<h4 class ='rd'>Password cannot be empty!</h4>";
        credsOk = false;
    }
    if (credsOk == false)
    {
        $("#wifi_connect_credentials_errors").html(errorList);
    }
    else
    {
        $("#wifi_connect_credentials_errors").html("");
        connectWifi();
    }
}

/**
 * Shows the WiFi password if the box is checked
 */
function showPassword()
{
    var x = document.getElementById("connect_pass");
    if (x.type === "password")
    {
        x.type = "text";
    }
    else 
    {
        x.type = "password";
    }
}

/**
 * Gets the connection information for displaying on the web page
 */
function getConnectInfo() {
    $.getJSON('/wifiConnectInfo.json', function(data) {
        console.log("Received connection info:", data);  // Debug log
        
        $("#connected_ap_label").html("Connected to:");
        $("#connected_ap").text(data["ap"]);

        $("#ip_address_label").html("IP Address:");
        $("#wifi_connect_ip").text(data["ip"]);

        $("#netmask_label").html("Netmask:");
        $("#wifi_connect_netmask").text(data["netmask"]);

        $("#gateway_label").html("Gateway:");
        $("#wifi_connect_gw").text(data["gw"]);
        document.getElementById('ConnectInfo').style.display = "block";
        document.getElementById('disconnect_wifi').style.display = "block";
    }).fail(function(jqXHR, textStatus, errorThrown) {
        console.error("Failed to get connection info:", textStatus, errorThrown);
    });
}

/**
 * Disconnects Wifi once the disconnect button is pressed and reloads the webpage
 */
function disconnectWifi()
{
    $.ajax({
        url: '/wifiDisconnect.json',
        dataType: 'json',
        method: 'DELETE',
        cache: false,
        data: { 'timestamp': Date.now() }
    });
    // Update the web page
    setTimeout("location.reload(true);", 2000);
}
/**
 * Gets the Local time
    Connect the ESP32 to the internet and time will be updated
*/
function getLocalTime()
{
    console.log("Requesting local time...");  // Debug log
    
    $.getJSON('/localTime.json', function(data) {
        console.log("Received time data:", data);  // Debug log
        
        if (data.time && data.time !== "") {
            $("#local_time").text(data.time);
            console.log("Updated time display with:", data.time);
        } else {
            $("#local_time").text("Time not available");
            console.log("Time not available in response");
        }
    }).fail(function(jqXHR, textStatus, errorThrown) {
        console.error("Failed to get local time:", textStatus, errorThrown);
        $("#local_time").text("Failed to get time");
    });
}
/*
* Sets the interval for displaying local time
*/
function startLocalTimeInterval()
{
    // Get time immediately when page loads
    getLocalTime();
    // Then set interval to update every 10 seconds
    setInterval(getLocalTime, 10000);
}

/**
 * Gets the ESP32's access point SSID for displaying on the web page
 */
function getSSID()
{
    $.getJSON('/apSSID.json', function(data) {
        $("#ap_ssid").text(data["ssid"]);
    });
}
