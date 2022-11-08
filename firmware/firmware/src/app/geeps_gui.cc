#include "geeps_gui.hh"
#include <stdio.h> // for printf
#include <string.h> // for c-string operations
#include "gui_bitmaps.hh"

#define MIN(A, B) ((A) < (B) ? (A) : (B))

GeepsGUIElement::GeepsGUIElement(GeepsGUIElementConfig_t config)
: config_(config) {
    if (!config.display) {
        printf("geeps_gui: GeepsGUIElement: Trying to initialize a GeepsGUIElement with a NULL display, will SEGFAULT!\r\n");
    }
}

/* GUIStatusBar */
GUIStatusBar::GUIStatusBar(GeepsGUIElementConfig_t config)
    : GeepsGUIElement(config)
    , battery_percent(0.0f)
    , position_fix(GGAPacket::FIX_NOT_AVAILABLE)
    , num_satellites(0)
{
    memset(time_string, '\0', NMEAPacket::kMaxPacketFieldLen);
    memset(latitude_string, '\0', NMEAPacket::kMaxPacketFieldLen);
    memset(longitude_string, '\0', NMEAPacket::kMaxPacketFieldLen);
}

/**
 * @brief Draws a status bar in the specified y location (always takes full width of screen).
 */
void GUIStatusBar::Draw() {
    // TODO: Draw battery icon with fill bar in top left.
    config_.display->DrawBitmap(0, 0, battery_icon_15x15, 15, 15, EPaperDisplay::EPAPER_BLACK);
    char battery_text[kNumberStringLength]; // this could be shorter
    sprintf(battery_text, "%.0f%%", battery_percent);
    config_.display->DrawText(20, kStatusBarHeight/3, battery_text);

    config_.display->DrawText(0, 20, time_string);

    // TODO: Draw Satellite icon in middle with number of sats (red if none).
    char satellites_str[kNumberStringLength];
    sprintf(satellites_str, "%d", num_satellites);
    EPaperDisplay::EPaper_Color_t satellite_color = num_satellites > 0 ? EPaperDisplay::EPAPER_BLACK : EPaperDisplay::EPAPER_RED;
    config_.display->DrawBitmap(50, 0, satellite_icon_15x15, 15, 15, satellite_color);
    config_.display->DrawText(70, kStatusBarHeight/3, satellites_str, satellite_color);

    // TODO: Draw GPS coordinates on second line (red "NO FIX" if no fix).
    config_.display->DrawText(0, 50, latitude_string);
    config_.display->DrawText(0, 70, longitude_string);
    // config_.display->DrawBitmap(70, 0, satellite_icon_15x15, 15, 15, EPaperDisplay::EPAPER_BLACK);
}

/* GUIHintBox */
GUIHintBox::GUIHintBox(GeepsGUIElementConfig_t config) 
: GeepsGUIElement(config) {


}

void GUIHintBox::Draw() {
    // uint16_t y = 10;
    // config_.screen->selectFont(Font_Terminal6x8);
    // config_.screen->gText(10, y, (char *)"Hello World!\r\n", myColours.red);
    // config_.screen->gText(10, y+config_.screen->characterSizeY(), (char *)"Doot Doot.\r\n", myColours.black);
    
    // config_.screen->selectFont(Font_Terminal6x8);
    // char hint_text_row[kRowNumChars];
    // for (uint row = 0; row < kHintTextMaxLen / kRowNumChars; row++) {
    //     strncpy(hint_text_row, hint_text+(row*kRowNumChars), kRowNumChars);
    //     config_.screen->gText(config_.pos_x + kTextMargin, config_.pos_y + row*kCharHeight, hint_text_row, myColours.black);
    // }
}