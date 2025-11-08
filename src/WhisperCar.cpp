#include "Particle.h"
#include "RdJson.h"
#include "TinyGPS++.h"
#include "ICM_20948.h"
#include "calibration.h"
#include "whisperapi.h"

// ---------------- CONFIG ----------------
#include "secrets.h"
/*
    "secrets.h" defines
    const char* ntripUser
    const char* ntripPass
    const char* API_HOST
    const char* casterHost
    const uint16_t casterPort
    const char* mountpoint
*/

const bool useSSL = false; // set true for TLS

#define gnssSerial Serial1 // Use Serial1 for GNSS
const int GNSS_BAUD = 9600;
#define WIRE_PORT Wire

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

LEDStatus hardware_error_status(RGB_COLOR_RED, LED_PATTERN_BLINK, LED_SPEED_FAST, LED_PRIORITY_CRITICAL);
LEDStatus hardware_waiting_status(RGB_COLOR_YELLOW, LED_PATTERN_FADE, LED_SPEED_SLOW, LED_PRIORITY_NORMAL);
LEDStatus wait_for_lock_status(RGB_COLOR_YELLOW, LED_PATTERN_FADE, LED_SPEED_FAST, LED_PRIORITY_CRITICAL);

// Reconnect timing
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_DELAY_MS = 5000UL;

#include "nrtk.h"

bool pos_valid = false;
double lng = 0;
double lat = 0;
double target_lng = 0;
double target_lat = 0;
double heading = 0;
char buf[500];

TinyGPSPlus gps;
ICM_20948_I2C imu;

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }
  Serial.print(" ");
  Serial.printf("%f", gps.course.deg());
  Serial.print(" ");
  Serial.print(buf);

  Serial.println();
}

bool updated = false;
unsigned long last_time = 0;

int cloud_calibrateIMU(String _s) {
    calibrateIMU(&imu);
    return 0;
}

int cloud_setTargetWaypoint(String waypoint) {
    char* lat_ptr = strchr(waypoint.c_str(), ',');

    int lat_len = strlen(lat_ptr);
    int total_len = waypoint.length();

    String lng_str = waypoint.substring(total_len-lat_len-1);

    lat = atof(lat_ptr);
    lng = atof(lng_str.c_str());

    return 0;
}

void setup() {
    hardware_waiting_status.setActive(true);
    Serial.begin(115200);
    delay(500);

    gnssSerial.begin(GNSS_BAUD);

    Whisper::updateVariable("pos_valid", &pos_valid, pos_valid);
    Whisper::updateVariable("longitude", &lng, lng);
    Whisper::updateVariable("latitude", &lat, lat);
    Whisper::updateVariable("heading", &heading, heading);

    WIRE_PORT.begin();
    WIRE_PORT.setClock(400000);
    imu.begin(WIRE_PORT, 0);

    while(imu.status != ICM_20948_Stat_Ok) {
        if(imu.status != ICM_20948_Stat_Ok) {
            //Particle.publish("IMU", "Waiting...");
            delay(500);
        }
    }

    bool calibrated = loadCalibrationData();
    if (!calibrated) {
        //Particle.publish("IMU", "No calibration data found");
        calibrateIMU(&imu);  // Run automatically on first boot
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "Loaded calibration: mag(%.2f, %.2f, %.2f)", magOffset.x, magOffset.y, magOffset.z);
        //Particle.publish("IMU", msg);
    }

    Whisper::registerFunction("calibrateIMU", &cloud_calibrateIMU);
    Whisper::registerFunction("setTargetWaypoint", &cloud_setTargetWaypoint);

    hardware_waiting_status.setActive(false);
}


void loop() {
    static unsigned long lastReconnectAttempt = 0;
    static unsigned long lastLocationPrint = 0;

    // Ensure Wi-Fi / Particle cloud connected
    if (WiFi.ready() != true) {
        delay(1);
        return;
    }

    // Reconnect to caster if needed
    if (!client.connected()) {
        Serial.println("Connecting to caster");
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_DELAY_MS) {
            lastReconnectAttempt = now;
            connectToCaster();
        }
    }

    // --- Stream a few bytes to GNSS (non-blocking) ---
    if (client.connected()) {
        if (client.available()) {
            uint8_t b = client.read();
            gnssSerial.write(b);
        }
    }

    if(gnssSerial.available()) {
        if(gps.encode(gnssSerial.read())) {
            displayInfo();
            updated = true;
        }
    }

    if(updated && last_time + 1000 < millis()) {
        Whisper::updateVariable("pos_valid", &pos_valid, gps.location.isValid());
        Whisper::updateVariable("longitude", &lng, gps.location.lng());
        Whisper::updateVariable("latitude", &lat, gps.location.lat());
        Whisper::handleCloudFunctions();
        last_time = millis();
        updated = false;

        if(imu.dataReady()) {
            imu.getAGMT();

            float ax = imu.accX() - accOffset.x;
            float ay = imu.accY() - accOffset.y;
            float az = imu.accZ() - accOffset.z;

            // Get magnetometer readings
            float mx = imu.magX() - magOffset.x;
            float my = imu.magY() - magOffset.y;
            float mz = imu.magZ() - magOffset.z;

            // Calculate pitch and roll (in radians)
            float pitch = atan2(-ax, sqrt(ay * ay + az * az));
            float roll = atan2(ay, az);

            // Calculate tilt-compensated magnetic field values
            float mx_comp = mx * cos(pitch) + mz * sin(pitch);
            float my_comp = mx * sin(roll) * sin(pitch) + my * cos(roll) - mz * sin(roll) * cos(pitch);

            // Calculate heading (in degrees)
            heading = atan2(my_comp, mx_comp) * 180.0 / PI;
            if (heading < 0) heading += 360;
            Whisper::updateVariable("heading", &heading, heading);

            sprintf(buf, "\nMag: %f, %f, %f, %f Accel: %f %f %f", mx, my, mz, heading, ax / 1000.0f * 9.81, ay / 1000.0f * 9.81, az / 1000.0f * 9.81);
            //Particle.publish("IMU", buf);
        }

        if(gps.location.isValid()) {
            wait_for_lock_status.setActive(false);
        }else {
            wait_for_lock_status.setActive(true);
        }
    }


    delay(1);
}
