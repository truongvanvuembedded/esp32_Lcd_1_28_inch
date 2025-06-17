idf_component_register(
    SRCS 
        "UI/ui.c"
        "UI/ui_events.c"
        "UI/ui_helpers.c"
        "UI/components/ui_comp_hook.c"
        "UI/components/ui_comp_wifiitems.c"
        "UI/components/ui_comp.c"
        "UI/screens/ui_MenuScreen.c"
        "UI/screens/ui_WifiTyping.c"
        "UI/screens/ui_WifiScreen.c"
        "UI/screens/ui_WeatherScreen.c"
    INCLUDE_DIRS 
        "UI"
        "UI/components"
        "UI/fonts"
        "UI/screens"
    REQUIRES lvgl common)
