// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.5.3
// LVGL version: 9.2.2
// Project name: SquareLine_Project

#include "../ui.h"

lv_obj_t * uic_BrightnessValue;
lv_obj_t * uic_BrightNessAjustScreen;
lv_obj_t * ui_BrightNessAjustScreen = NULL;
lv_obj_t * ui_Container1 = NULL;
lv_obj_t * ui_Label5 = NULL;
lv_obj_t * ui_Container2 = NULL;
lv_obj_t * ui_BrightnessValue = NULL;
lv_obj_t * ui_BrightnessSlider = NULL;
// event funtions
void ui_event_BrightNessAjustScreen(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_GESTURE &&  lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_LEFT) {
        lv_indev_wait_release(lv_indev_active());
        _ui_screen_change(&ui_MenuScreen, LV_SCR_LOAD_ANIM_FADE_ON, 100, 0, &ui_MenuScreen_screen_init);
    }
}
void ui_event_BrightnessSlider(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_VALUE_CHANGED) {
        u1_BrightNessValue = (U1)lv_slider_get_value(target) > (U1)100 ?100:(U1)lv_slider_get_value(target);
        _ui_slider_set_text_value(ui_BrightnessValue, target, "", "");
        event_BrightnessAdjust_Callback(e);
    }
}

// build funtions

void ui_BrightNessAjustScreen_screen_init(void)
{
    ui_BrightNessAjustScreen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_BrightNessAjustScreen, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_flex_flow(ui_BrightNessAjustScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_BrightNessAjustScreen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    ui_Container1 = lv_obj_create(ui_BrightNessAjustScreen);
    lv_obj_remove_style_all(ui_Container1);
    lv_obj_set_width(ui_Container1, lv_pct(100));
    lv_obj_set_height(ui_Container1, lv_pct(20));
    lv_obj_set_align(ui_Container1, LV_ALIGN_TOP_MID);
    lv_obj_remove_flag(ui_Container1, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_border_width(ui_Container1, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label5 = lv_label_create(ui_Container1);
    lv_obj_set_width(ui_Label5, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label5, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Label5, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label5, "Brightness Adjust");
    lv_obj_set_style_text_font(ui_Label5, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Container2 = lv_obj_create(ui_BrightNessAjustScreen);
    lv_obj_remove_style_all(ui_Container2);
    lv_obj_set_width(ui_Container2, lv_pct(100));
    lv_obj_set_height(ui_Container2, lv_pct(80));
    lv_obj_set_x(ui_Container2, 2);
    lv_obj_set_y(ui_Container2, 0);
    lv_obj_set_align(ui_Container2, LV_ALIGN_BOTTOM_MID);
    lv_obj_remove_flag(ui_Container2, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_BrightnessValue = lv_label_create(ui_Container2);
    lv_obj_set_width(ui_BrightnessValue, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_BrightnessValue, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_BrightnessValue, -4);
    lv_obj_set_y(ui_BrightnessValue, -30);
    lv_obj_set_align(ui_BrightnessValue, LV_ALIGN_CENTER);


    ui_BrightnessSlider = lv_slider_create(ui_Container2);
    lv_slider_set_value(ui_BrightnessSlider, u1_BrightNessValue, LV_ANIM_OFF);
    if(lv_slider_get_mode(ui_BrightnessSlider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ui_BrightnessSlider, 0,
                                                                                                     LV_ANIM_OFF);
    lv_obj_set_height(ui_BrightnessSlider, 20);
    lv_obj_set_width(ui_BrightnessSlider, lv_pct(70));
    lv_obj_set_align(ui_BrightnessSlider, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_BrightnessSlider, lv_color_hex(0xD5D4D6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BrightnessSlider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    //Compensating for LVGL9.1 draw crash with bar/slider max value when top-padding is nonzero and right-padding is 0
    if(lv_obj_get_style_pad_top(ui_BrightnessSlider, LV_PART_MAIN) > 0) lv_obj_set_style_pad_right(ui_BrightnessSlider,
                                                                                                       lv_obj_get_style_pad_right(ui_BrightnessSlider, LV_PART_MAIN) + 1, LV_PART_MAIN);
    lv_obj_add_event_cb(ui_BrightnessSlider, ui_event_BrightnessSlider, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_BrightNessAjustScreen, ui_event_BrightNessAjustScreen, LV_EVENT_ALL, NULL);
    uic_BrightNessAjustScreen = ui_BrightNessAjustScreen;
    uic_BrightnessValue = ui_BrightnessValue;

    _ui_slider_set_text_value(ui_BrightnessValue, ui_BrightnessSlider, "", "");

}

void ui_BrightNessAjustScreen_screen_destroy(void)
{
    if(ui_BrightNessAjustScreen) lv_obj_del(ui_BrightNessAjustScreen);

    // NULL screen variables
    uic_BrightNessAjustScreen = NULL;
    ui_BrightNessAjustScreen = NULL;
    ui_Container1 = NULL;
    ui_Label5 = NULL;
    ui_Container2 = NULL;
    uic_BrightnessValue = NULL;
    ui_BrightnessValue = NULL;
    ui_BrightnessSlider = NULL;

}
