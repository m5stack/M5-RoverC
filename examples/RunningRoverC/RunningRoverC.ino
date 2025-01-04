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
#include "M5_RoverC.h"

M5_RoverC roverc;

void view_test(void);
void setup()
{
    M5.begin();
    roverc.begin();
    view_test();
}

void loop()
{
    // Servo Control
    // roverc.setServoAngle(uint8_t pos, uint8_t angle)
    roverc.setServoAngle(0, 90);
    roverc.setServoAngle(1, 90);
    delay(1000);
    roverc.setServoAngle(0, 10);
    roverc.setServoAngle(1, 10);
    delay(1000);
    // Car Control
    // roverc.setSpeed(int8_t x, int8_t y, int8_t z)
    roverc.setSpeed(100, 0, 0);
    delay(1000);
    roverc.setSpeed(0, 100, 0);
    delay(1000);
    roverc.setSpeed(0, 0, 100);
    delay(1000);
    roverc.setSpeed(0, 0, 0);
    delay(1000);
}
void view_test(void)
{
    M5.Display.setRotation(3);
    M5.Display.setTextColor(ORANGE);
    int screenWidth   = M5.Display.width();
    int screenHeight  = M5.Display.height();
    String text       = "RoverC TEST";
    int textSize      = 1;
    m5::board_t board = M5.getBoard();
    switch (board) {
        case m5::board_t::board_M5StickC: {
            textSize = 2;
        } break;
        case m5::board_t::board_M5StickCPlus: {
            textSize = 3;
        } break;
        case m5::board_t::board_M5StickCPlus2: {
            textSize = 3;
        } break;
    }
    M5.Display.setTextSize(textSize);
    int textWidth  = M5.Display.textWidth(text);
    int textHeight = M5.Display.fontHeight();
    int x          = (screenWidth - textWidth) / 2;
    int y          = (screenHeight - textHeight) / 2;
    M5.Display.drawString(text, x, y);
}