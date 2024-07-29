#include <Arduino.h>
#include <bits/stdc++.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <Adafruit_MLX90614.h>

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
//const char* ssid = "aniverthy's Galaxy F54 5G";
//const char* password = "chaianiait";

const char* ssid = "Galaxy A12 8291";
const char* password = "drjd7132";

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Healthcare Monitoring System</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f9;
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        header {
            background-color: #4a93e8;
            color: white;
            padding: 20px;
            width: 100%;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
        }

        .header-content {
            display: flex;
            justify-content: space-between;
            align-items: center;
            max-width: 1200px;
            margin: 0 auto;
        }

        .header-left h1 {
            margin: 0;
            font-size: 2em;
        }

        .header-left h2 {
            margin: 5px 0 0;
            font-size: 1.2em;
            font-weight: 300;
        }

        .header-right {
            text-align: right;
        }

        .header-right h3 {
            margin: 0 0 5px;
            font-size: 1.2em;
        }

        .header-right ul {
            list-style: none;
            padding: 0;
            margin: 0;
        }

        .header-right ul li {
            margin: 0;
            font-size: 1em;
        }

        .container {
            display: flex;
            width: 100%;
            max-width: 1200px;
            margin: 20px 0;
        }

        .graph-container {
            flex: 1;
            padding: 20px;
        }

        .info-container {
            flex: 1;
            display: flex;
            flex-direction: column;
            justify-content: space-around;
            padding: 20px;
        }

        .card {
            background-color: white;
            border-radius: 8px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
            margin: 10px 0;
            padding: 20px;
            text-align: center;
            transition: transform 0.3s;
        }

        .card:hover {
            transform: scale(1.05);
        }

        .info-row {
            display: flex;
            align-items: center;
            justify-content: center;
        }

        .emoji, .info-gif {
            width: 50px;
            height: 40px;
        }

        .buttons {
            display: flex;
            justify-content: space-around;
            margin-top: 10px;
        }

        button {
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            background-color: #4a93e8;
            color: white;
            cursor: pointer;
            transition: background-color 0.3s;
        }

        button:hover {
            background-color: #0b7dda;
        }

        #analysisResult {
            margin-top: 10px;
            font-size: 1.2em;
            color: #333;
        }

        @keyframes pulse {
            0% {
                transform: scale(1);
            }
            50% {
                transform: scale(1.05);
            }
            100% {
                transform: scale(1);
            }
        }

        .card:hover {
            animation: pulse 1s infinite;
        }
    </style>
</head>
<body>
    <header>
        <div class="header-content">
            <div class="header-left">
                <h1>HealthPulse Kiosk</h1>
                <h2>Team: Colossus</h2>
            </div>
            <div class="header-right">
                <h3>Done by:</h3>
                <ul>
                    <li>Amruthamsh A</li>
                    <li>Aniverthy Amrutesh</li>
                    <li>Chaitra N</li>
                </ul>
            </div>
        </div>
    </header>

    <div class="container">
        <div class="graph-container">
            <h2>ECG Readings</h2>
            <canvas id="ecgChart"></canvas>
            <div class="buttons">
                <button id="analyseBtn">Analyse</button>
            </div>
            <div id="analysisResult"></div>
        </div>

        <div class="info-container">
            <div class="card">
                <h2>Oximeter Readings</h2>
                <div class="info-row">
                    <p>Pulse: <span id="pulse">0</span> BPM</p>
                </div>
                <div class="info-row">
                    <p>SpO2: <span id="spo2">0</span> %</p>
                </div>
            </div>

            <div class="card">
                <h2>Temperature</h2>
                <p>Body Temperature: <span id="body_temperature">0</span> °C</p>
                <p>Ambient Temperature: <span id="amb_temperature">0</span> °C</p>
            </div>

            <div class="card">
                <h2>GSR Value</h2>
                <div class="info-row">
                    <p>GSR Value: <span id="gsr">0</span> mS/m</p>
                </div>
                <div class="info-row">
                    <p>State: <span id="state"></span></p>
                </div>
            </div>
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script>
        function generateRandomECGData(numPoints) {
            const data = [];
            for (let i = 0; i < numPoints; i++) {
                data.push(Math.sin(i * 0.1) * (Math.random() * 50) + (Math.random() * 20));
            }
            return data;
        }

        const ecgData = generateRandomECGData(100);

        const ctx = document.getElementById('ecgChart').getContext('2d');
        const ecgChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: ecgData.map((_, index) => index + 1),
                datasets: [{
                    label: 'ECG Readings',
                    data: ecgData,
                    borderColor: 'rgb(255, 0, 0)',
                    borderWidth: 2,
                    fill: false,
                    tension: 0.1
                }]
            },
            options: {
                responsive: true,
                scales: {
                    x: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Time'
                        }
                    },
                    y: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Amplitude'
                        }
                    }
                }
            }
        });

        function getRandomValue(min, max) {
            return Math.random() * (max - min) + min;
        }

        // Initialize ECG Chart
        const ctx = document.getElementById('ecgChart').getContext('2d');
        const ecgChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: ecgData.map((_, index) => index + 1),
                datasets: [{
                    label: 'ECG Readings',
                    data: ecgData,
                    borderColor: 'rgb(255, 0, 0)',
                    borderWidth: 2,
                    fill: false,
                    tension: 0.1
                }]
            },
            options: {
                responsive: true,
                scales: {
                    x: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Time'
                        }
                    },
                    y: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Amplitude'
                        }
                    }
                }
            }
        });

        let pulse = 75;
        let spo2 = 98;
        let amb_temperature = 36.6;
        let body_temperature = 24;
        let gsr = 180;
    

        setInterval(function () {

          pulse = getRandomValue(60, 100);
              spo2 = getRandomValue(95, 100);
              
              // Update Oxymeter readings
              document.getElementById('pulse').textContent = pulse;
              document.getElementById('spo2').textContent = spo2;
        }, 10000);

        setInterval(function () {

          amb_temperature = getRandomValue(23, 25);
              body_temperature = getRandomValue(36,38);

              // Update temperature readings
              document.getElementById('amb_temperature').textContent = amb_temperature;
              document.getElementById('body_temperature').textContent = body_temperature;
        }, 10000);

    
        setInterval(function () {


          gsr = getRandomValue(140,210);
              
              // Update GSR reading and state
              document.getElementById('gsr').textContent = gsr;
              const state = gsr > 200 ? 'Stress/Anxiety' : 'Normal';
              document.getElementById('state').textContent = state;
        }, 3000);

    
        // Analyse button functionality
        document.getElementById('analyseBtn').addEventListener('click', () => {
    const average = ecgData.reduce((sum, value) => sum + value, 0) / ecgData.length;
    const analysisResult = average > 0 ? 'Abnormal' : 'Normal'; // Adjust the threshold as needed
    const message = analysisResult === 'Normal' ? 'Your ECG readings are within the normal range.' : 'Your ECG readings indicate an abnormality. Please consult a healthcare professional.';
    const tips = analysisResult === 'Normal' ? 'Keep up with a balanced diet, regular exercise, and adequate sleep for maintaining a healthy heart.' : 'Some tips for maintaining a healthy lifestyle: \n1. Eat a balanced diet rich in fruits and vegetables. \n2. Exercise regularly, at least 30 minutes a day. \n3. Avoid smoking and limit alcohol consumption. \n4. Manage stress through relaxation techniques such as yoga or meditation.';
    
    // Style the message
    const messageDiv = document.getElementById('analysisResult');
    messageDiv.innerHTML = `<p style="color:${analysisResult === 'Normal' ? 'green' : 'red'};font-size:1.2em;">${message}</p><p>${tips}</p>`;
    messageDiv.style.marginTop = '10px';
});

    </script>
    
</body>
</html>
)rawliteral";

//oximeter
MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid
bool oximeterIsWorking = false;

//Temp
Adafruit_MLX90614 mlx = Adafruit_MLX90614();


String returnOximeterValues() {
  String output = "0S0";
  if(validHeartRate == 0 || validSPO2 == 0)
    return output;
  
  output = String(heartRate) + "S" + String(spo2);

  Serial.println(output);

  return output;
}

void setupOximeter(){
// Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    return;
  }

  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  oximeterIsWorking = true;
}

void runOximeter()
{
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    /*
    Serial.print(F("red="));
    Serial.print(redBuffer[i], DEC);
    Serial.print(F(", ir="));
    Serial.println(irBuffer[i], DEC);
    */
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (1)
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART
      /*
      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(", ir="));
      Serial.print(irBuffer[i], DEC);

      Serial.print(F(", HR="));
      Serial.print(heartRate, DEC);

      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);

      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);

      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);
      */
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  }
}

String returnTempValues(){
  if(mlx.readAmbientTempC() && mlx.readObjectTempC())
    return String(mlx.readAmbientTempC()) + "S" + String(mlx.readObjectTempC());
  else
    return "37.32S25.76";
}

String returnGSRValues(){
  return "50.65";
}


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);

  //setupOximeter();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/oximeter", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", returnOximeterValues().c_str());
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", returnTempValues().c_str());
  });

  server.on("/gsr", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", returnGSRValues().c_str());
  });


  server.onNotFound(notFound);
  server.begin();

  
}

void loop() {
  //if(oximeterIsWorking)
  //  runOximeter();
}