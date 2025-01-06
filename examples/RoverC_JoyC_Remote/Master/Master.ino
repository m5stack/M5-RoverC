
/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5-RoverC: https://github.com/m5stack/M5-RoverC
 */

#include "M5Unified.h"
#include "Wire.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "M5_RoverC.h"

const char *ssid     = "M5AP";
const char *password = "77777777";

M5_RoverC roverc;

WiFiServer server(80);
WiFiUDP Udp1;

uint32_t count = 0;

uint8_t servo = 0;

void drawCenteredText(int y, const String &text, int textSize, uint16_t textColor);
void view_head(void);
unsigned long lastTime = 0;
void setup()
{
    M5.begin();
    M5.update();
    roverc.begin();

    uint64_t chipid = ESP.getEfuseMac();
    String str      = ssid + String((uint32_t)(chipid >> 32), HEX);
    M5.Display.setRotation(3);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(WHITE);
    M5.Display.fillRect(0, 0, 240, 30, M5.Display.color565(50, 50, 50));
    drawCenteredText(40, str, 2, GREEN);

    Serial.begin(115200);
    // Set device in STA mode to begin with
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

    WiFi.softAP(str.c_str(), password);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.begin();
    Udp1.begin(1003);
    view_head();
    lastTime = millis();
}

void loop()
{
    int udplength = Udp1.parsePacket();
    if (udplength) {
        char udodata[udplength];
        Udp1.read(udodata, udplength);
        IPAddress udp_client = Udp1.remoteIP();
        if ((udodata[0] == 0xAA) && (udodata[1] == 0x55) && (udodata[8] == 0xee)) {
            for (int i = 0; i < 9; i++) {
                Serial.printf("%02X ", udodata[i]);
            }
            Serial.println();
            if (udodata[6] == 0x01) {
                roverc.setSpeed(udodata[3] - 100, udodata[4] - 100, udodata[5] - 100);
            } else {
                roverc.setSpeed(0, 0, 0);
            }
        } else {
            roverc.setSpeed(0, 0, 0);
        }

        if (udodata[7] != 0) {
            if (millis() - lastTime > 300) {
                lastTime = millis();
                if (servo) {
                    roverc.setServoAngle(0, 90);
                    roverc.setServoAngle(1, 90);
                } else {
                    roverc.setServoAngle(0, 10);
                    roverc.setServoAngle(1, 10);
                }
                servo = 1 - servo;
            }
        }
    }
    count++;
    if (count > 5000) {
        count = 0;
        view_head();
    }
}

/**
 * @brief Draws horizontally centered text on the M5 display with a specified color.
 *
 * This function clears the area where the text will be displayed (with black background),
 * calculates the position to horizontally center the text, and then draws it with the specified color.
 *
 * @param y The fixed Y-axis position where the text will be drawn.
 * @param text The string to be displayed.
 * @param textSize The size of the text to be drawn.
 * @param textColor The color of the text to be displayed.
 */
void drawCenteredText(int y, const String &text, int textSize, uint16_t textColor)
{
    // Get the screen width
    int screenWidth = M5.Display.width();

    // Set the text size and color
    M5.Display.setTextSize(textSize);
    M5.Display.setTextColor(textColor, M5.Display.color565(0, 0, 0));  // Set text color and black background color

    // Get the width and height of the text
    int textWidth  = M5.Display.textWidth(text);  // Text width
    int textHeight = M5.Display.fontHeight();     // Text height

    // Calculate the X-axis position to center the text
    int x = (screenWidth - textWidth) / 2;  // Centering on the X-axis

    // Clear the entire area where the text will be displayed (black background)
    M5.Display.fillRect(0, y, screenWidth, textHeight, M5.Display.color565(0, 0, 0));

    // Set the cursor position and draw the text
    M5.Display.setCursor(x, y);
    M5.Display.printf("%s", text.c_str());  // Draw the text
}
void view_head(void)
{
    float bat         = 0;
    m5::board_t board = M5.getBoard();
    if (board == m5::board_t::board_M5StickC) {
        bat = M5.Power.Axp192.getBatteryVoltage();
    } else if (board == m5::board_t::board_M5StickCPlus) {
        bat = M5.Power.Axp192.getBatteryVoltage();
    } else if (board == m5::board_t::board_M5StickCPlus2) {
        bat = M5.Power.getBatteryVoltage() / 1000.0;
    }
    M5.Display.fillRect(0, 0, 240, 30, M5.Display.color565(50, 50, 50));
    M5.Display.setTextSize(2);
    if (bat > 3.30) {
        M5.Display.setTextColor(GREEN);
    } else {
        M5.Display.setTextColor(RED);
    }
    char buffer[64] = {0};

    sprintf(buffer, "BAT:%.3fV", bat);
    String text    = String(buffer);
    int textWidth  = M5.Display.textWidth(text);
    int textHeight = M5.Display.fontHeight();
    int x          = (M5.Display.width() - textWidth) / 2;
    M5.Display.setCursor(x, 7);
    M5.Display.printf("%s", text.c_str());
}