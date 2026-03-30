/* core */
static const uint32_t mod_main = WLR_MODIFIER_ALT;
static const uint32_t mod_aux = WLR_MODIFIER_LOGO;
static const int ws_n = 9;
static const float main_ratio = 0.60f;
static const float ratio_step = 0.03f;
static const float ratio_min = 0.15f;
static const float ratio_max = 0.85f;
static const int gap_px = 8;
static const int kb_rate = 25;
static const int kb_delay = 600;

/* focus policy */
static const bool foc_cur = true;
static const bool foc_click = false;
static const bool foc_new = true;
static const bool raise_foc = true;
static const bool warp_cur_foc = false;

/* commands */
static const char term_cmd[] = "kitty || foot || alacritty || xterm";
static const char menu_cmd[] = "rofi -show drun";
static const bool set_cursor_theme = false;
static const char cursor_theme[] = "Bibata-Modern-Ice";
static const int cursor_size = 24;

#define MOD mod_main
#define AUX mod_aux
#define SHF WLR_MODIFIER_SHIFT
#define BIND(MASK, KEY, FUNC, ...) {.mod = (MASK), .sym = (KEY), .fn = (FUNC), .arg = {__VA_ARGS__}}

#define WSKEYS(KEY, WS) \
    BIND(MOD, KEY, act_ws_go, .i = WS), \
    BIND(MOD | SHF, KEY, act_ws_send, .i = WS), \
    BIND(AUX, KEY, act_ws_send, .i = WS),

static const struct env_kv envs[] = {
    {"MOZ_ENABLE_WAYLAND", "1"},
    {NULL, NULL},
};

/* autostart */
static const char *start[] = {
    NULL,
};

/* keybinds */
static const struct bind binds[] = {
    BIND(MOD, XKB_KEY_q, act_spawn, .v = term_cmd),
    BIND(MOD, XKB_KEY_Return, act_spawn, .v = term_cmd),
    BIND(MOD, XKB_KEY_d, act_spawn, .v = menu_cmd),
    BIND(MOD, XKB_KEY_c, act_close, 0),
    BIND(MOD | SHF, XKB_KEY_q, act_quit, 0),
    BIND(MOD, XKB_KEY_space, act_lay_step, 0),
    BIND(MOD, XKB_KEY_f, act_float, 0),
    BIND(AUX, XKB_KEY_f, act_full, 0),
    BIND(MOD, XKB_KEY_h, act_ratio, .f = -ratio_step),
    BIND(MOD, XKB_KEY_l, act_ratio, .f = ratio_step),
    BIND(MOD, XKB_KEY_j, act_foc_step, .i = 1),
    BIND(MOD, XKB_KEY_k, act_foc_step, .i = -1),
    WSKEYS(XKB_KEY_1, 0)
    WSKEYS(XKB_KEY_2, 1)
    WSKEYS(XKB_KEY_3, 2)
    WSKEYS(XKB_KEY_4, 3)
    WSKEYS(XKB_KEY_5, 4)
    WSKEYS(XKB_KEY_6, 5)
    WSKEYS(XKB_KEY_7, 6)
    WSKEYS(XKB_KEY_8, 7)
    WSKEYS(XKB_KEY_9, 8)
};

#undef WSKEYS
#undef BIND
#undef AUX
#undef SHF
#undef MOD

static const struct rule rules[] = {
    {"foot", NULL, -1, -1, false, false},
    {"kitty", NULL, -1, -1, false, false},
    {"Alacritty", NULL, -1, -1, false, false},
    {"org.pwmt.zathura", NULL, -1, -1, true, false},
    {NULL, "mpv", -1, -1, true, false},
    {NULL, NULL, -1, -1, false, false},
};
