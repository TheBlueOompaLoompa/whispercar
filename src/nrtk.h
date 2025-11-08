// I asked ChatGPT to generate a NRTK rover client for Arduino

TCPClient client;
// SecureTCPClient client; // Uncomment for TLS

String lastGGA = "";

// ---------------- HELPER FUNCTIONS ----------------
String base64Encode(const String &input) {
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String out;
    const uint8_t* data = (const uint8_t*)input.c_str();
    int len = input.length();
    int i = 0;
    while (i < len) {
        uint32_t octet_a = i < len ? data[i++] : 0;
        uint32_t octet_b = i < len ? data[i++] : 0;
        uint32_t octet_c = i < len ? data[i++] : 0;
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        out += b64[(triple >> 18) & 0x3F];
        out += b64[(triple >> 12) & 0x3F];
        out += (i - 2) <= len ? b64[(triple >> 6) & 0x3F] : '=';
        out += (i - 1) <= len ? b64[triple & 0x3F] : '=';
    }
    return out;
}

String readLatestGGA(unsigned long timeoutMs = 200) {
    static String buffer = "";
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        while (gnssSerial.available()) {
            char c = (char)gnssSerial.read();
            buffer += c;
            if (c == '\n') {
                int idx = buffer.indexOf("$GPGGA");
                if (idx < 0) idx = buffer.indexOf("$GNGGA");
                if (idx >= 0) {
                    int endIdx = buffer.indexOf('\n', idx);
                    if (endIdx > idx) {
                        String gga = buffer.substring(idx, endIdx);
                        buffer = buffer.substring(endIdx + 1);
                        gga.trim();
                        return gga;
                    }
                }
                if (buffer.length() > 1024) buffer = "";
            }
        }
        delay(1);
    }
    return String();
}

bool connectToCaster() {
    Serial.printf("Connecting to caster %s:%d ...\n", casterHost, casterPort);
    if (!client.connect(casterHost, casterPort)) {
        Serial.println("Failed to open TCP connection to caster.");
        return false;
    }

    // Auth header
    String authLine = "";
    if (strlen(ntripUser) > 0) {
        String cred = String(ntripUser) + ":" + String(ntripPass);
        authLine = "Authorization: Basic " + base64Encode(cred) + "\r\n";
    }

    // Read GGA for request
    String gga = readLatestGGA(300);
    if (gga.length() > 0) lastGGA = gga;

    // Build NTRIP GET request
    String req = "GET /" + String(mountpoint) + " HTTP/1.0\r\n";
    req += "User-Agent: NTRIP Photon/1.0\r\n";
    req += "Accept: */*\r\n";
    if (lastGGA.length() > 0) req += "Ntrip-GGA: " + lastGGA + "\r\n";
    req += authLine;
    req += "\r\n";

    client.print(req);
    client.flush();

    // Read headers
    String headers = "";
    unsigned long start = millis();
    while (millis() - start < 5000) {
        while (client.available()) {
            char c = (char)client.read();
            headers += c;
            int pos = headers.indexOf("\r\n\r\n");
            if (pos >= 0) {
                String statusLine = headers.substring(0, headers.indexOf("\r\n"));
                Serial.println("Caster response: " + statusLine);
                if (statusLine.indexOf("200") >= 0 || statusLine.startsWith("ICY 200")) {
                    Serial.println("Connected and streaming RTCM.");
                    return true;
                } else {
                    Serial.println("Caster rejected connection.");
                    client.stop();
                    return false;
                }
            }
        }
        delay(1);
    }
    Serial.println("Timed out waiting for caster headers.");
    client.stop();
    return false;
}

void streamToGNSS() {
    while (client.connected()) {
        while (client.available()) {
            uint8_t b = client.read();
            gnssSerial.write(b);
        }
        String gga = readLatestGGA(1);
        if (gga.length() > 0) lastGGA = gga;
        delay(1);
    }
    Serial.println("Caster connection closed.");
    client.stop();
}