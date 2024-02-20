// Complete project details: https://randomnerdtutorials.com/esp32-web-server-websocket-sliders/

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var mode = ["Off", "Auto", "On"];
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues() {
    console.log('Web->HMI getValues');
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateThreshold(element) {
    document.getElementById("threshold").innerHTML = element.value;
    document.getElementById("thresholdValue").innerHTML = element.value;
    console.log("Web->HMI threshold=" + element.value);
    websocket.send("threshold=" + element.value);
}

function updateWorkMode(element) {
    document.getElementById("workModeValue").innerHTML = mode[element.value];
    console.log("Web->HMI mode=" + element.value);
    websocket.send("mode=" + element.value);
}

function updateLamp1(element) {
    document.getElementById("lamp").style = "font-size:64px;color:grey"
    console.log("Web->HMI lampSwitch")
    websocket.send("LampSwitch")
}

function onMessage(event) {
    console.log("Web<-HMI" + event.data);
    var data = JSON.parse(event.data);
    if (data.mode == 1) {
        document.getElementById("workMode").value = 1;
        document.getElementById("workModeValue").innerHTML = mode[1];
    } else {
        if (data.status == 1) {
            document.getElementById("workMode").value = 2;
            document.getElementById("workModeValue").innerHTML = mode[2];
        } else {
            document.getElementById("workMode").value = 0;
            document.getElementById("workModeValue").innerHTML = mode[0];
        }
    }
    document.getElementById("threshold").value = data.threshold;
    document.getElementById("thresholdValue").innerHTML = data.threshold;
    document.getElementById("ambientLight").innerHTML = data.ambient_light;

    if (data.status == "0") {
        document.getElementById("lamp").style = "font-size:64px;color:darkblue"
    }
    else if (data.status == "1") {
        document.getElementById("lamp").style = "font-size:64px;color:yellow"
    }
}
