/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5Hat-JoyC: https://github.com/m5stack/M5Hat-JoyC
 */

#include "M5Unified.h"
#include "Hat_JoyC.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "EEPROM.h"
#include "icon.c"

#define EEPROM_SIZE (64)
#define SYSNUM      (3)

JoyC joyc;

extern const unsigned char connect_on[800];
extern const unsigned char connect_off[800];

IPAddress local_IP(192, 168, 4, 100 + SYSNUM);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);    // optional
IPAddress secondaryDNS(8, 8, 4, 4);  // optional

uint64_t realTime[4], time_count = 0;
bool k_ready       = false;
uint32_t key_count = 0;

const char *ssid     = "M5AP";
const char *password = "77777777";

WiFiUDP Udp;
uint32_t send_count  = 0;
uint8_t system_state = 0;
bool joys_l          = false;
uint8_t color[3]     = {0, 100, 0};
uint8_t SendBuff[9]  = {0xAA, 0x55, SYSNUM, 0x00, 0x00, 0x00, 0x00, 0x00, 0xee};
char APName[20];
String WfifAPBuff[16];
uint32_t count_bn_a = 0, choose = 0;
String ssidname;

void drawCenteredText(int y, const String &text, int textSize, uint16_t textColor);
void SendUDP(void);
void setup()
{
    // put your setup code here, to run once:
    M5.begin();
    Serial.begin(115200);
    joyc.begin();  // Initialize JoyC. 初始化 JoyC
    EEPROM.begin(EEPROM_SIZE);
    int textSize      = 1;
    m5::board_t board = M5.getBoard();
    switch (board) {
        case m5::board_t::board_M5StickC: {
            textSize = 1;
        } break;
        case m5::board_t::board_M5StickCPlus: {
            textSize = 2;
        } break;
        case m5::board_t::board_M5StickCPlus2: {
            textSize = 2;
        } break;
    }
    M5.Display.setTextSize(textSize);
    M5.Display.fillRect(0, 0, 135, 40, M5.Display.color565(50, 50, 50));
    M5.Display.pushImage(M5.Display.width() / 2 - 10, 10, 20, 20, (uint16_t *)connect_off);

    M5.update();
    if ((EEPROM.read(0) != 0x56) || (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(500))) {
        WiFi.mode(WIFI_STA);
        int n = WiFi.scanNetworks();
        if (n == 0) {
            drawCenteredText(50, "no networks", textSize, RED);
        } else {
            int count = 0;
            for (int i = 0; i < n; ++i) {
                if (WiFi.SSID(i).indexOf("M5AP") != -1) {
                    if (count == 0) {
                        M5.Display.setTextColor(GREEN);
                    } else {
                        M5.Display.setTextColor(WHITE);
                    }
                    String str = WiFi.SSID(i);
                    drawCenteredText(50, str.c_str(), textSize, GREEN);
                    WfifAPBuff[count] = WiFi.SSID(i);
                    count++;
                }
            }
            while (1) {
                if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(500)) {
                    if (count_bn_a >= 200) {
                        count_bn_a = 201;
                        EEPROM.writeUChar(0, 0x56);
                        EEPROM.writeString(1, WfifAPBuff[choose]);
                        ssidname = WfifAPBuff[choose];
                        break;
                    }
                    count_bn_a++;
                    Serial.printf("count_bn_a %d \n", count_bn_a);
                } else if ((M5.BtnA.isReleased()) && (count_bn_a != 0)) {
                    Serial.printf("count_bn_a %d", count_bn_a);
                    if (count_bn_a > 200) {
                    } else {
                        choose++;
                        if (choose >= count) {
                            choose = 0;
                        }
                        for (int i = 0; i < count; i++) {
                            if (choose == i) {
                                M5.Display.setTextColor(GREEN);
                            } else {
                                M5.Display.setTextColor(WHITE);
                            }
                            drawCenteredText(50 + i * 25, WfifAPBuff[i].c_str(), textSize, GREEN);
                        }
                    }
                    count_bn_a = 0;
                }
                delay(10);
                M5.update();
            }
            // EEPROM.writeString(1,WfifAPBuff[0]);
        }
    } else if (EEPROM.read(0) == 0x56) {
        ssidname = EEPROM.readString(1);
        EEPROM.readString(1, APName, 16);
    }

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure");
    }

    WiFi.begin(ssidname.c_str(), password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Udp.begin(2000);
    M5.Display.pushImage(M5.Display.width() / 2 - 10, 10, 20, 20, (uint16_t *)connect_on);
}

uint8_t keyPressStatus[2] = {0};
uint8_t keyStatus         = 0;
uint16_t AngleBuff[4];
uint32_t count = 0;
void loop()
{
    keyStatus         = 0;
    keyPressStatus[0] = 0;
    keyPressStatus[1] = 0;
    keyPressStatus[0] = joyc.getPress(0);
    keyPressStatus[1] = joyc.getPress(1);
    if (keyPressStatus[0] || keyPressStatus[1]) {
        keyStatus = 1;
    }

    AngleBuff[0] = 200 - joyc.getX(0);
    AngleBuff[1] = joyc.getY(0);
    AngleBuff[2] = joyc.getX(1);
    AngleBuff[3] = joyc.getY(1);

    delay(10);

    if (WiFi.status() != WL_CONNECTED) {
        count++;
        if (count > 500) {
            WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
            count = 0;
        }
    } else {
        SendBuff[3] = AngleBuff[0];
        SendBuff[4] = AngleBuff[1];
        SendBuff[5] = AngleBuff[2];
        SendBuff[7] = keyStatus;
        if ((SendBuff[3] > 110) || (SendBuff[3] < 90) || (SendBuff[4] > 110) || (SendBuff[4] < 90) ||
            (SendBuff[5] > 110) || (SendBuff[5] < 90)) {
            SendBuff[6] = 0x01;
        } else {
            SendBuff[6] = 0x00;
        }
        SendUDP();
    }

    drawCenteredText(50, String(SendBuff[3]), 3, ORANGE);
    drawCenteredText(80, String(SendBuff[4]), 3, ORANGE);
    drawCenteredText(110, String(SendBuff[5]), 3, ORANGE);
    drawCenteredText(140, String(keyStatus), 3, ORANGE);
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

void SendUDP(void)
{
    if (WiFi.status() == WL_CONNECTED) {
        Udp.beginPacket(IPAddress(192, 168, 4, 1), 1000 + SYSNUM);
        Udp.write(SendBuff, sizeof(SendBuff));
        Udp.endPacket();
    }
}
