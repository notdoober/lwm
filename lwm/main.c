#define _POSIX_C_SOURCE 200809L

#ifndef LWM_HAS_XWAYLAND
#define LWM_HAS_XWAYLAND 0
#endif

#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/backend/session.h>
#include <wlr/render/allocator.h>
#include <wlr/render/pixman.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_linux_dmabuf_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_shm.h>
#include <wlr/types/wlr_single_pixel_buffer_v1.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/edges.h>
#include <wlr/util/log.h>
#if LWM_HAS_XWAYLAND
#include <wlr/xwayland/xwayland.h>
#include <xcb/xproto.h>
#endif
#include <xkbcommon/xkbcommon.h>

enum layout_mode {
    LAYOUT_TILE = 0,
    LAYOUT_MONOCLE = 1,
    LAYOUT_FLOATING = 2,
};

enum cursor_mode {
    CURSOR_PASSTHROUGH = 0,
    CURSOR_MOVE = 1,
    CURSOR_RESIZE = 2,
};

struct srv;

struct env_kv {
    const char *name;
    const char *val;
};

struct rule {
    const char *app;
    const char *cls;
    int ws;
    int out;
    bool flt;
    bool fs;
};

struct arg {
    int i;
    float f;
    const char *v;
};

struct bind;
typedef void (*bind_fn)(struct srv *s, const struct arg *arg);

struct bind {
    uint32_t mod;
    xkb_keysym_t sym;
    bind_fn fn;
    struct arg arg;
};

struct win {
    struct wl_list link;
    struct srv *s;
    struct mon *m;
    struct wlr_surface *surface;
    struct wlr_xdg_surface *xdg;
#if LWM_HAS_XWAYLAND
    struct wlr_xwayland_surface *xwl;
#endif
    struct wlr_scene_tree *t;

    bool mapped;
    bool flt;
    bool fs;
    int ws;
    int x;
    int y;
    int w;
    int h;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
    struct wl_listener req_move;
    struct wl_listener req_resize;
    struct wl_listener req_fs;
#if LWM_HAS_XWAYLAND
    struct wl_listener assoc;
    struct wl_listener dissoc;
#endif
};

struct lay {
    struct wl_list link;
    struct srv *s;
    struct wlr_layer_surface_v1 *lyr;
    struct wlr_scene_layer_surface_v1 *scn;
    bool mapped;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
};

struct kbd {
    struct wl_list link;
    struct srv *s;
    struct wlr_keyboard *kb;
    struct wl_listener key;
    struct wl_listener mod;
    struct wl_listener destroy;
};

struct ptr {
    struct wl_list link;
    struct srv *s;
    struct wlr_input_device *dev;
    struct wl_listener destroy;
};

struct mon {
    struct wl_list link;
    struct srv *s;
    struct wlr_output *out;
    struct wlr_box box;
    int ws;
    struct wl_listener frame;
    struct wl_listener destroy;
};

struct srv {
    struct wl_display *dpy;
    struct wlr_backend *be;
    struct wlr_session *session;
    struct wlr_renderer *rnd;
    struct wlr_allocator *alloc;
    struct wlr_compositor *comp;

    struct wlr_scene *scn;
    struct wlr_scene_output_layout *sol;
    struct wlr_output_layout *ol;

    struct wlr_scene_tree *bg;
    struct wlr_scene_tree *bot;
    struct wlr_scene_tree *win;
    struct wlr_scene_tree *top;
    struct wlr_scene_tree *ovr;

    struct wlr_xdg_shell *xdg;
    struct wlr_layer_shell_v1 *lay;
    struct wlr_xdg_decoration_manager_v1 *deco;
#if LWM_HAS_XWAYLAND
    struct wlr_xwayland *xwl;
    bool xwl_on;
#endif

    struct wlr_cursor *cur;
    struct wlr_xcursor_manager *xcm;
    struct wlr_seat *seat;

    struct wl_list wins;
    struct wl_list lays;
    struct wl_list kbs;
    struct wl_list ptrs;
    struct wl_list mons;

    struct mon *sel;
    struct win *foc;
    struct win *grab;
    enum cursor_mode mode;
    uint32_t edges;
    double gx;
    double gy;
    struct wlr_box gbox;

    enum layout_mode layout;
    int ws;
    float ratio;
    int gap;
    int start_i;
    bool started;

    struct wl_listener new_out;
    struct wl_listener new_in;
    struct wl_listener req_cur;
    struct wl_listener req_sel;
    struct wl_listener cur_mot;
    struct wl_listener cur_abs;
    struct wl_listener cur_btn;
    struct wl_listener cur_axis;
    struct wl_listener cur_frm;
    struct wl_listener new_xdg;
#if LWM_HAS_XWAYLAND
    struct wl_listener xwl_ready;
    struct wl_listener new_xwl;
#endif
    struct wl_listener new_lay;
    struct wl_listener new_deco;
};

static void tile(struct srv *s);
static void win_focus(struct win *w);
static void win_unmap(struct wl_listener *l, void *data);
static void win_commit(struct wl_listener *l, void *data);
static void win_die(struct wl_listener *l, void *data);
static void win_activate(struct win *w, bool active);
static void win_request_close(struct win *w);
#if LWM_HAS_XWAYLAND
static void win_destroy_scene(struct win *w);
#endif
static void win_set_fullscreen(struct win *w, bool fs);
static void win_configure(struct win *w, int x, int y, int ww, int hh);
static int win_pref_w(struct win *w);
static int win_pref_h(struct win *w);
static void seat_refocus(struct srv *s);
static void ws_sync(struct srv *s);
static void xdg_req_on(struct win *w);
static void xdg_req_off(struct win *w);
static void foc_top(struct srv *s);
static void act_spawn(struct srv *s, const struct arg *arg);
static void act_close(struct srv *s, const struct arg *arg);
static void act_quit(struct srv *s, const struct arg *arg);
static void act_lay_step(struct srv *s, const struct arg *arg);
static void act_float(struct srv *s, const struct arg *arg);
static void act_full(struct srv *s, const struct arg *arg);
static void act_ratio(struct srv *s, const struct arg *arg);
static void act_foc_step(struct srv *s, const struct arg *arg);
static void act_ws_go(struct srv *s, const struct arg *arg);
static void act_ws_send(struct srv *s, const struct arg *arg);

#include "config.h"

static bool env_is(const char *name, const char *want) {
    const char *val = getenv(name);
    return val && strcmp(val, want) == 0;
}

static bool env_on(const char *name, bool def) {
    const char *val = getenv(name);
    if (!val || val[0] == '\0') {
        return def;
    }
    if (!strcmp(val, "0") || !strcmp(val, "false") || !strcmp(val, "no") || !strcmp(val, "off")) {
        return false;
    }
    return true;
}

static void env_apply(void) {
    if (set_cursor_theme) {
        char size[16];
        snprintf(size, sizeof(size), "%d", cursor_size);
        setenv("XCURSOR_THEME", cursor_theme, 1);
        setenv("XCURSOR_SIZE", size, 1);
    }
    for (int i = 0; envs[i].name; i++) {
        setenv(envs[i].name, envs[i].val, 1);
    }
}

static void set_sess_env(void) {
    setenv("XDG_CURRENT_DESKTOP", "lwm", 1);
    setenv("XDG_SESSION_DESKTOP", "lwm", 1);
    setenv("DESKTOP_SESSION", "lwm", 1);
    setenv("XDG_SESSION_TYPE", "wayland", 1);
}

static void dbus_boot(char **argv) {
    if (!env_on("LWM_DBUS", false)) {
        return;
    }
    if (getenv("DBUS_SESSION_BUS_ADDRESS")) {
        return;
    }
    if (env_is("LWM_DBUS_BOOT", "1")) {
        return;
    }

    setenv("LWM_DBUS_BOOT", "1", 1);
    execvp("dbus-run-session", (char *const[]){"dbus-run-session", "--", argv[0], NULL});
    unsetenv("LWM_DBUS_BOOT");
    fprintf(stderr, "lwm: dbus-run-session not found, continuing without a session bus\n");
}

static struct wlr_renderer *rnd_mk(struct wlr_backend *be) {
    const char *name = getenv("LWM_RENDERER");

    if (name && name[0] != '\0' && strcmp(name, "pixman") == 0) {
        struct wlr_renderer *rnd = wlr_pixman_renderer_create();
        if (!rnd) {
            fprintf(stderr, "lwm: pixman renderer unavailable, falling back to auto\n");
        } else {
            fprintf(stdout, "lwm: renderer=pixman\n");
            fflush(stdout);
            return rnd;
        }
    }

    fprintf(stdout, "lwm: renderer=auto\n");
    fflush(stdout);
    struct wlr_renderer *rnd = wlr_renderer_autocreate(be);
    if (rnd) {
        return rnd;
    }

    fprintf(stderr, "lwm: auto renderer unavailable, falling back to pixman\n");
    return wlr_pixman_renderer_create();
}

static void lis_drop(struct wl_listener *lis) {
    if (!lis) {
        return;
    }
    if (!lis->link.prev || !lis->link.next) {
        return;
    }
    wl_list_remove(&lis->link);
    lis->link.prev = NULL;
    lis->link.next = NULL;
}

static void sig_chld(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

static void srv_fini(struct srv *s) {
    if (!s) {
        return;
    }

    lis_drop(&s->new_out);
    lis_drop(&s->new_in);
    lis_drop(&s->req_cur);
    lis_drop(&s->req_sel);
    lis_drop(&s->cur_mot);
    lis_drop(&s->cur_abs);
    lis_drop(&s->cur_btn);
    lis_drop(&s->cur_axis);
    lis_drop(&s->cur_frm);
    lis_drop(&s->new_xdg);
#if LWM_HAS_XWAYLAND
    lis_drop(&s->xwl_ready);
    lis_drop(&s->new_xwl);
#endif
    lis_drop(&s->new_lay);
    lis_drop(&s->new_deco);

    if (s->be) {
        wlr_backend_destroy(s->be);
        s->be = NULL;
    }
}

static void spawn(const char *cmd) {
    if (!cmd || cmd[0] == '\0') {
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        return;
    }
    if (pid == 0) {
        int nullfd = open("/dev/null", O_RDWR);
        setsid();
        if (nullfd >= 0) {
            dup2(nullfd, STDIN_FILENO);
            dup2(nullfd, STDOUT_FILENO);
            dup2(nullfd, STDERR_FILENO);
            if (nullfd > STDERR_FILENO) {
                close(nullfd);
            }
        }
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }
}

static void start_idle(void *data) {
    struct srv *s = data;
    const char *cmd = start[s->start_i];

    if (!cmd) {
        return;
    }

    s->start_i++;
    spawn(cmd);
    if (start[s->start_i]) {
        wl_event_loop_add_idle(wl_display_get_event_loop(s->dpy), start_idle, s);
    }
}

static void start_req(struct srv *s) {
    if (!s || s->started) {
        return;
    }

    s->started = true;
    wl_event_loop_add_idle(wl_display_get_event_loop(s->dpy), start_idle, s);
}

static bool mod_on(uint32_t mod) {
    return (mod & mod_main) != 0U;
}

static uint32_t mod_clean(uint32_t mod) {
    return mod & (WLR_MODIFIER_SHIFT | WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT | WLR_MODIFIER_LOGO);
}

static bool sym_eq(xkb_keysym_t sym, xkb_keysym_t want) {
    if (want == XKB_KEY_NoSymbol) {
        return false;
    }
    return xkb_keysym_to_lower(sym) == xkb_keysym_to_lower(want);
}

static int gap_fit(int gap, int span, int count) {
    if (gap <= 0 || span <= 0 || count <= 1) {
        return 0;
    }

    int max_gap = span - count;
    if (max_gap <= 0) {
        return 0;
    }

    max_gap /= count - 1;
    return gap < max_gap ? gap : max_gap;
}

static bool str_is(const char *s, const char *pat) {
    return !pat || (s && strcmp(s, pat) == 0);
}

static struct mon *mon_first(struct srv *s) {
    if (!s || wl_list_empty(&s->mons)) {
        return NULL;
    }
    struct mon *m = NULL;
    return wl_container_of(s->mons.next, m, link);
}

static struct mon *mon_of_out(struct srv *s, struct wlr_output *out) {
    struct mon *m = NULL;
    wl_list_for_each(m, &s->mons, link) {
        if (m->out == out) {
            return m;
        }
    }
    return NULL;
}

static struct mon *mon_at(struct srv *s, double lx, double ly) {
    struct wlr_output *out = wlr_output_layout_output_at(s->ol, lx, ly);
    return out ? mon_of_out(s, out) : NULL;
}

static struct mon *mon_cur(struct srv *s) {
    if (s->foc && s->foc->m) {
        return s->foc->m;
    }
    if (s->sel) {
        return s->sel;
    }
    struct mon *m = mon_at(s, s->cur->x, s->cur->y);
    return m ? m : mon_first(s);
}

static struct mon *mon_id(struct srv *s, int id) {
    if (id < 0) {
        return mon_cur(s);
    }
    int i = 0;
    struct mon *m = NULL;
    wl_list_for_each(m, &s->mons, link) {
        if (i++ == id) {
            return m;
        }
    }
    return mon_cur(s);
}

static void mon_move_wins(struct srv *s, struct mon *src, struct mon *dst) {
    struct win *w = NULL;
    wl_list_for_each(w, &s->wins, link) {
        if (w->m == src) {
            w->m = dst;
        }
    }
}

static void caps_set(struct srv *s) {
    uint32_t caps = 0;
    if (!wl_list_empty(&s->ptrs)) {
        caps |= WL_SEAT_CAPABILITY_POINTER;
    }
    if (!wl_list_empty(&s->kbs)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(s->seat, caps);
}

static void foc_nil(struct srv *s) {
    if (!s) {
        return;
    }
    win_activate(s->foc, false);
    s->foc = NULL;
    seat_refocus(s);
}

static void foc_warp(struct win *w) {
    if (!warp_cur_foc || !w || !w->s || !w->mapped) {
        return;
    }
    wlr_cursor_warp(w->s->cur, NULL, w->x + (double)w->w / 2.0, w->y + (double)w->h / 2.0);
}

static bool win_vis(struct win *w) {
    return w && w->mapped && w->m && w->ws == w->m->ws;
}

static struct wlr_surface *win_surface(struct win *w) {
    if (!w) {
        return NULL;
    }
    return w->surface;
}

static const char *win_app(struct win *w) {
    if (!w) {
        return NULL;
    }
    if (w->xdg && w->xdg->toplevel) {
        return w->xdg->toplevel->app_id;
    }
#if LWM_HAS_XWAYLAND
    if (w->xwl) {
        return w->xwl->instance ? w->xwl->instance : w->xwl->class;
    }
#endif
    return NULL;
}

static const char *win_cls(struct win *w) {
#if LWM_HAS_XWAYLAND
    return w && w->xwl ? w->xwl->class : NULL;
#else
    (void)w;
    return NULL;
#endif
}

static void win_rule(struct win *w) {
    if (!w) {
        return;
    }
    w->m = mon_cur(w->s);
    w->ws = w->m ? w->m->ws : 0;
    for (int i = 0; rules[i].app || rules[i].cls || i == 0; i++) {
        const struct rule *r = &rules[i];
        if (!r->app && !r->cls && i > 0) {
            break;
        }
        if (!str_is(win_app(w), r->app) || !str_is(win_cls(w), r->cls)) {
            continue;
        }
        if (r->ws >= 0 && r->ws < ws_n) {
            w->ws = r->ws;
        }
        w->m = mon_id(w->s, r->out);
        w->flt = r->flt;
        w->fs = r->fs;
        return;
    }
}

static void win_activate(struct win *w, bool active) {
    if (!w) {
        return;
    }
    if (w->xdg && w->xdg->toplevel) {
        if (!w->xdg->initialized) {
            return;
        }
        wlr_xdg_toplevel_set_activated(w->xdg->toplevel, active);
        return;
    }
#if LWM_HAS_XWAYLAND
    if (w->xwl) {
        wlr_xwayland_surface_activate(w->xwl, active);
        if (active) {
            wlr_xwayland_surface_restack(w->xwl, NULL, XCB_STACK_MODE_ABOVE);
        }
    }
#endif
}

static void win_request_close(struct win *w) {
    if (!w) {
        return;
    }
    if (w->xdg && w->xdg->toplevel) {
        wlr_xdg_toplevel_send_close(w->xdg->toplevel);
        return;
    }
#if LWM_HAS_XWAYLAND
    if (w->xwl) {
        wlr_xwayland_surface_close(w->xwl);
    }
#endif
}

#if LWM_HAS_XWAYLAND
static void win_destroy_scene(struct win *w) {
    if (!w || !w->t) {
        return;
    }
    wlr_scene_node_destroy(&w->t->node);
    w->t = NULL;
}
#endif

static void win_set_fullscreen(struct win *w, bool fs) {
    if (!w) {
        return;
    }
    w->fs = fs;
    if (w->xdg && w->xdg->toplevel) {
        wlr_xdg_toplevel_set_fullscreen(w->xdg->toplevel, fs);
    }
#if LWM_HAS_XWAYLAND
    else if (w->xwl) {
        wlr_xwayland_surface_set_fullscreen(w->xwl, fs);
    }
#endif
    tile(w->s);
}

static void win_configure(struct win *w, int x, int y, int ww, int hh) {
    if (!w) {
        return;
    }
    if (w->xdg && w->xdg->toplevel) {
        if (!w->xdg->initialized) {
            return;
        }
        wlr_xdg_toplevel_set_size(w->xdg->toplevel, (uint32_t)ww, (uint32_t)hh);
        return;
    }
#if LWM_HAS_XWAYLAND
    if (w->xwl) {
        wlr_xwayland_surface_configure(w->xwl, (int16_t)x, (int16_t)y, (uint16_t)ww, (uint16_t)hh);
    }
#else
    (void)x;
    (void)y;
#endif
}

static int win_pref_w(struct win *w) {
    if (!w) {
        return 0;
    }
    if (w->xdg) {
        return w->xdg->current.geometry.width;
    }
#if LWM_HAS_XWAYLAND
    if (w->xwl) {
        return w->xwl->width;
    }
#endif
    return 0;
}

static int win_pref_h(struct win *w) {
    if (!w) {
        return 0;
    }
    if (w->xdg) {
        return w->xdg->current.geometry.height;
    }
#if LWM_HAS_XWAYLAND
    if (w->xwl) {
        return w->xwl->height;
    }
#endif
    return 0;
}

static struct win *top_vis(struct srv *s) {
    struct win *w = NULL;
    struct mon *m = mon_cur(s);
    wl_list_for_each(w, &s->wins, link) {
        if (win_vis(w) && w->m == m) {
            return w;
        }
    }
    return NULL;
}

static void foc_top(struct srv *s) {
    struct win *top = top_vis(s);
    if (top) {
        win_focus(top);
    } else {
        foc_nil(s);
    }
}

static void ws_sync(struct srv *s) {
    struct win *w = NULL;
    wl_list_for_each(w, &s->wins, link) {
        wlr_scene_node_set_enabled(&w->t->node, win_vis(w));
    }
    if (!win_vis(s->foc)) {
        foc_nil(s);
    }
}

static void ws_go(struct srv *s, int ws) {
    struct mon *m = mon_cur(s);
    if (!m || ws < 0 || ws >= ws_n || m->ws == ws) {
        return;
    }
    m->ws = ws;
    s->sel = m;
    ws_sync(s);
    tile(s);
    foc_top(s);
}

static void ws_send(struct srv *s, struct win *w, int ws) {
    if (!w || ws < 0 || ws >= ws_n) {
        return;
    }
    w->ws = ws;
    s->sel = w->m ? w->m : s->sel;
    ws_sync(s);
    tile(s);
    if (!s->foc) {
        struct win *top = top_vis(s);
        if (top) {
            win_focus(top);
        }
    }
}

static int vt_id(xkb_keysym_t sym) {
    switch (sym) {
        case XKB_KEY_XF86Switch_VT_1:
        case XKB_KEY_F1:
            return 1;
        case XKB_KEY_XF86Switch_VT_2:
        case XKB_KEY_F2:
            return 2;
        case XKB_KEY_XF86Switch_VT_3:
        case XKB_KEY_F3:
            return 3;
        case XKB_KEY_XF86Switch_VT_4:
        case XKB_KEY_F4:
            return 4;
        case XKB_KEY_XF86Switch_VT_5:
        case XKB_KEY_F5:
            return 5;
        case XKB_KEY_XF86Switch_VT_6:
        case XKB_KEY_F6:
            return 6;
        case XKB_KEY_XF86Switch_VT_7:
        case XKB_KEY_F7:
            return 7;
        case XKB_KEY_XF86Switch_VT_8:
        case XKB_KEY_F8:
            return 8;
        case XKB_KEY_XF86Switch_VT_9:
        case XKB_KEY_F9:
            return 9;
        case XKB_KEY_XF86Switch_VT_10:
        case XKB_KEY_F10:
            return 10;
        case XKB_KEY_XF86Switch_VT_11:
        case XKB_KEY_F11:
            return 11;
        case XKB_KEY_XF86Switch_VT_12:
        case XKB_KEY_F12:
            return 12;
        default:
            return 0;
    }
}

static struct wlr_scene_tree *lay_tree(struct srv *s, enum zwlr_layer_shell_v1_layer l) {
    switch (l) {
        case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
            return s->bg;
        case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
            return s->bot;
        case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
            return s->ovr;
        case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
        default:
            return s->top;
    }
}

static struct wlr_output *act_out(struct srv *s) {
    struct mon *m = mon_cur(s);
    return m ? m->out : wlr_output_layout_get_center_output(s->ol);
}

static bool lay_ready(struct lay *ly) {
    return ly && ly->lyr && ly->lyr->initialized;
}

static bool lay_kbd(struct lay *ly) {
    return lay_ready(ly) &&
        ly->mapped &&
        ly->lyr->current.keyboard_interactive !=
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE;
}

static void seat_focus_surface(struct srv *s, struct wlr_surface *sf) {
    if (!s) {
        return;
    }
    if (!sf) {
        wlr_seat_keyboard_notify_clear_focus(s->seat);
        return;
    }

    struct wlr_keyboard *kb = wlr_seat_get_keyboard(s->seat);
    if (kb) {
        wlr_seat_keyboard_notify_enter(s->seat, sf, kb->keycodes, kb->num_keycodes, &kb->modifiers);
    } else {
        wlr_seat_keyboard_notify_enter(s->seat, sf, NULL, 0, NULL);
    }
}

static void seat_refocus(struct srv *s) {
    if (!s) {
        return;
    }

    /* Interactive layer-shell surfaces take keyboard focus before regular windows. */
    struct mon *m = mon_cur(s);
    struct lay *ly = NULL;
    wl_list_for_each(ly, &s->lays, link) {
        if (!lay_kbd(ly)) {
            continue;
        }
        if (m && ly->lyr->output && ly->lyr->output != m->out) {
            continue;
        }
        seat_focus_surface(s, ly->lyr->surface);
        return;
    }

    if (win_vis(s->foc)) {
        seat_focus_surface(s, win_surface(s->foc));
    } else {
        seat_focus_surface(s, NULL);
    }
}

static void win_focus(struct win *w) {
    struct srv *s = w ? w->s : NULL;
    if (!s) {
        return;
    }
    if (!win_vis(w)) {
        return;
    }
    if (s->foc == w) {
        s->sel = w->m;
        seat_refocus(s);
        return;
    }
    win_activate(s->foc, false);
    s->foc = w;
    if (!w) {
        seat_refocus(s);
        return;
    }
    if (raise_foc) {
        wl_list_remove(&w->link);
        wl_list_insert(&s->wins, &w->link);
        wlr_scene_node_raise_to_top(&w->t->node);
    }
    win_activate(w, true);
    s->sel = w->m;
    seat_refocus(s);
    foc_warp(w);
}

static struct win *win_of(struct wlr_scene_node *n) {
    while (n) {
        if (n->data) {
            return n->data;
        }
        n = n->parent ? &n->parent->node : NULL;
    }
    return NULL;
}

static struct win *desk_at(
    struct srv *s,
    double lx,
    double ly,
    struct wlr_surface **sf,
    double *sx,
    double *sy
) {
    struct wlr_scene_node *n = wlr_scene_node_at(&s->scn->tree.node, lx, ly, sx, sy);
    if (!n) {
        *sf = NULL;
        return NULL;
    }

    struct wlr_scene_buffer *buf = wlr_scene_buffer_from_node(n);
    if (!buf) {
        *sf = NULL;
        return NULL;
    }

    struct wlr_scene_surface *ss = wlr_scene_surface_try_from_buffer(buf);
    if (!ss) {
        *sf = NULL;
        return NULL;
    }

    *sf = ss->surface;
    return win_of(n);
}

static void win_geo(struct win *w, int x, int y, int ww, int hh) {
    if (ww < 1) {
        ww = 1;
    }
    if (hh < 1) {
        hh = 1;
    }
    w->x = x;
    w->y = y;
    w->w = ww;
    w->h = hh;
    wlr_scene_node_set_position(&w->t->node, x, y);
    win_configure(w, x, y, ww, hh);
}

static void xdg_req_on(struct win *w) {
    if (!w || !w->xdg || !w->xdg->toplevel) {
        return;
    }
    if (!w->req_move.link.prev) {
        wl_signal_add(&w->xdg->toplevel->events.request_move, &w->req_move);
    }
    if (!w->req_resize.link.prev) {
        wl_signal_add(&w->xdg->toplevel->events.request_resize, &w->req_resize);
    }
    if (!w->req_fs.link.prev) {
        wl_signal_add(&w->xdg->toplevel->events.request_fullscreen, &w->req_fs);
    }
}

static void xdg_req_off(struct win *w) {
    if (!w || !w->xdg) {
        return;
    }
    lis_drop(&w->req_move);
    lis_drop(&w->req_resize);
    lis_drop(&w->req_fs);
}

static void tile(struct srv *s) {
    struct win *w = NULL;
    wl_list_for_each(w, &s->wins, link) {
        wlr_scene_node_set_enabled(&w->t->node, win_vis(w));
    }

    struct mon *m = NULL;
    wl_list_for_each(m, &s->mons, link) {
        struct wlr_box full = {0};
        struct wlr_box use = {0};
        wlr_output_layout_get_box(s->ol, m->out, &full);
        m->box = full;
        use = full;

        enum zwlr_layer_shell_v1_layer z = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
        for (; z <= ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY; z++) {
            struct lay *l = NULL;
            wl_list_for_each(l, &s->lays, link) {
                /* Layer-shell clients need an initial configure before first map. */
                if (!l->mapped && !lay_ready(l)) {
                    continue;
                }
                if (l->lyr->output && l->lyr->output != m->out) {
                    continue;
                }
                if (l->lyr->current.layer != z) {
                    continue;
                }
                wlr_scene_layer_surface_v1_configure(l->scn, &full, &use);
            }
        }

        struct win *fsw = NULL;
        wl_list_for_each(w, &s->wins, link) {
            if (win_vis(w) && w->m == m && w->fs) {
                fsw = w;
                break;
            }
        }
        if (fsw) {
            wl_list_for_each(w, &s->wins, link) {
                if (w->m == m) {
                    wlr_scene_node_set_enabled(&w->t->node, win_vis(w) && w == fsw);
                }
            }
            win_geo(fsw, use.x, use.y, use.width, use.height);
            continue;
        }

        int n = 0;
        wl_list_for_each(w, &s->wins, link) {
            if (!win_vis(w) || w->m != m) {
                continue;
            }
            if (s->layout == LAYOUT_FLOATING || w->flt) {
                if (w->w <= 0 || w->h <= 0) {
                    int ww = use.width / 2;
                    int hh = use.height / 2;
                    int x = use.x + (use.width - ww) / 2;
                    int y = use.y + (use.height - hh) / 2;
                    win_geo(w, x, y, ww, hh);
                } else {
                    win_geo(w, w->x, w->y, w->w, w->h);
                }
                continue;
            }
            n++;
        }

        if (!n) {
            continue;
        }
        if (s->layout == LAYOUT_MONOCLE) {
            wl_list_for_each(w, &s->wins, link) {
                if (win_vis(w) && w->m == m && !w->flt) {
                    win_geo(w, use.x, use.y, use.width, use.height);
                }
            }
            continue;
        }

        int mcnt = n > 1 ? 1 : n;
        int st = n - mcnt;
        int col_gap = st > 0 ? gap_fit(s->gap, use.width, 2) : 0;
        int avail_w = use.width - col_gap;
        if (avail_w < 1) {
            avail_w = 1;
        }

        int mx = use.x;
        int sx = use.x;
        int mww = avail_w;
        int sww = 0;
        if (st > 0) {
            mww = (int)(avail_w * s->ratio);
            if (mww < 1) {
                mww = 1;
            }
            if (mww > avail_w - 1) {
                mww = avail_w - 1;
            }
            sww = avail_w - mww;
            sx = mx + mww + col_gap;
        }

        int mgap = gap_fit(s->gap, use.height, mcnt);
        int sgap = gap_fit(s->gap, use.height, st);
        int my = use.y;
        int sy = use.y;
        int mrem = use.height - mgap * (mcnt - 1);
        int srem = use.height - sgap * (st - 1);
        int mi = 0;
        int si = 0;

        wl_list_for_each(w, &s->wins, link) {
            if (!win_vis(w) || w->m != m || w->flt) {
                continue;
            }

            if (mi < mcnt) {
                int left = mcnt - mi;
                int hh = left > 0 ? mrem / left : mrem;
                if (hh < 1) {
                    hh = 1;
                }
                win_geo(w, mx, my, mww, hh);
                my += hh + mgap;
                mrem -= hh;
                mi++;
                continue;
            }

            int left = st - si;
            int hh = left > 0 ? srem / left : srem;
            if (hh < 1) {
                hh = 1;
            }
            win_geo(w, sx, sy, sww, hh);
            sy += hh + sgap;
            srem -= hh;
            si++;
        }
    }
}

static void grab_move(struct srv *s, struct win *w) {
    if (!w) {
        return;
    }
    w->flt = true;
    s->mode = CURSOR_MOVE;
    s->grab = w;
    s->gx = s->cur->x - w->x;
    s->gy = s->cur->y - w->y;
}

static void grab_resize(struct srv *s, struct win *w, uint32_t e) {
    if (!w) {
        return;
    }
    w->flt = true;
    s->mode = CURSOR_RESIZE;
    s->grab = w;
    s->edges = e;
    s->gx = s->cur->x;
    s->gy = s->cur->y;
    s->gbox.x = w->x;
    s->gbox.y = w->y;
    s->gbox.width = w->w;
    s->gbox.height = w->h;
}

static void cur_do(struct srv *s, uint32_t t) {
    struct mon *m = mon_at(s, s->cur->x, s->cur->y);
    if (m) {
        s->sel = m;
    }
    if (s->mode == CURSOR_MOVE && s->grab) {
        int x = (int)(s->cur->x - s->gx);
        int y = (int)(s->cur->y - s->gy);
        win_geo(s->grab, x, y, s->grab->w, s->grab->h);
        return;
    }

    if (s->mode == CURSOR_RESIZE && s->grab) {
        int dx = (int)(s->cur->x - s->gx);
        int dy = (int)(s->cur->y - s->gy);
        int x = s->gbox.x;
        int y = s->gbox.y;
        int ww = s->gbox.width;
        int hh = s->gbox.height;

        if (s->edges & WLR_EDGE_LEFT) {
            x += dx;
            ww -= dx;
        }
        if (s->edges & WLR_EDGE_RIGHT) {
            ww += dx;
        }
        if (s->edges & WLR_EDGE_TOP) {
            y += dy;
            hh -= dy;
        }
        if (s->edges & WLR_EDGE_BOTTOM) {
            hh += dy;
        }
        win_geo(s->grab, x, y, ww, hh);
        return;
    }

    struct wlr_surface *sf = NULL;
    double sx = 0.0;
    double sy = 0.0;
    struct win *w = desk_at(s, s->cur->x, s->cur->y, &sf, &sx, &sy);
    if (!sf) {
        wlr_cursor_set_xcursor(s->cur, s->xcm, "left_ptr");
        wlr_seat_pointer_notify_clear_focus(s->seat);
        return;
    }
    if (foc_cur && w && s->mode == CURSOR_PASSTHROUGH && s->foc != w) {
        win_focus(w);
    }
    wlr_seat_pointer_notify_enter(s->seat, sf, sx, sy);
    wlr_seat_pointer_notify_motion(s->seat, t, sx, sy);
}

static void foc_step(struct srv *s, int dir) {
    if (wl_list_empty(&s->wins)) {
        return;
    }

    struct win *cur = s->foc;
    struct wl_list *head = &s->wins;
    struct wl_list *n = cur ? &cur->link : head;

    for (;;) {
        n = dir > 0 ? n->next : n->prev;
        if (n == head) {
            n = dir > 0 ? n->next : n->prev;
        }
        if (n == head || (cur && n == &cur->link)) {
            break;
        }
        struct win *w = wl_container_of(n, w, link);
        if (!win_vis(w)) {
            continue;
        }
        win_focus(w);
        return;
    }
}

static void lay_step(struct srv *s) {
    if (s->layout == LAYOUT_TILE) {
        s->layout = LAYOUT_MONOCLE;
    } else if (s->layout == LAYOUT_MONOCLE) {
        s->layout = LAYOUT_FLOATING;
    } else {
        s->layout = LAYOUT_TILE;
    }
    tile(s);
}

static void act_spawn(struct srv *s, const struct arg *arg) {
    (void)s;
    spawn(arg->v);
}

static void act_close(struct srv *s, const struct arg *arg) {
    (void)arg;
    win_request_close(s->foc);
}

static void act_quit(struct srv *s, const struct arg *arg) {
    (void)arg;
    wl_display_terminate(s->dpy);
}

static void act_lay_step(struct srv *s, const struct arg *arg) {
    (void)arg;
    lay_step(s);
}

static void act_float(struct srv *s, const struct arg *arg) {
    (void)arg;
    struct win *foc = s->foc;
    if (!foc) {
        return;
    }
    foc->flt = !foc->flt;
    tile(s);
}

static void act_full(struct srv *s, const struct arg *arg) {
    (void)arg;
    struct win *foc = s->foc;
    if (!foc) {
        return;
    }
    win_set_fullscreen(foc, !foc->fs);
}

static void act_ratio(struct srv *s, const struct arg *arg) {
    s->ratio += arg->f;
    if (s->ratio < ratio_min) {
        s->ratio = ratio_min;
    } else if (s->ratio > ratio_max) {
        s->ratio = ratio_max;
    }
    tile(s);
}

static void act_foc_step(struct srv *s, const struct arg *arg) {
    foc_step(s, arg->i);
}

static void act_ws_go(struct srv *s, const struct arg *arg) {
    ws_go(s, arg->i);
}

static void act_ws_send(struct srv *s, const struct arg *arg) {
    ws_send(s, s->foc, arg->i);
}

static bool key_do(struct srv *s, uint32_t mod, xkb_keysym_t sym) {
    mod = mod_clean(mod);
    for (size_t i = 0; i < sizeof(binds) / sizeof(binds[0]); i++) {
        if (binds[i].mod != mod) {
            continue;
        }
        if (!sym_eq(sym, binds[i].sym)) {
            continue;
        }
        binds[i].fn(s, &binds[i].arg);
        return true;
    }

    return false;
}

static void kb_key(struct wl_listener *l, void *data) {
    struct kbd *k = wl_container_of(l, k, key);
    struct srv *s = k->s;
    struct wlr_keyboard_key_event *ev = data;
    bool done = false;
    if (ev->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        uint32_t mod = wlr_keyboard_get_modifiers(k->kb);
        bool ca = (mod & WLR_MODIFIER_CTRL) && (mod & WLR_MODIFIER_ALT);

        if (ca && s->session) {
            xkb_keysym_t const *syms = NULL;
            int n = xkb_state_key_get_syms(k->kb->xkb_state, ev->keycode + 8, &syms);
            for (int i = 0; i < n; i++) {
                int vt = vt_id(syms[i]);
                if (vt > 0 && wlr_session_change_vt(s->session, (unsigned)vt)) {
                    done = true;
                    break;
                }
            }
        }

        if (!done && mod) {
            xkb_keysym_t const *syms = NULL;
            int n = xkb_state_key_get_syms(k->kb->xkb_state, ev->keycode + 8, &syms);
            for (int i = 0; i < n; i++) {
                if (key_do(s, mod, syms[i])) {
                    done = true;
                    break;
            }
        }
    }

    seat_refocus(s);
}

    if (!done) {
        wlr_seat_set_keyboard(s->seat, k->kb);
        wlr_seat_keyboard_notify_key(s->seat, ev->time_msec, ev->keycode, ev->state);
    }
}

static void kb_mod(struct wl_listener *l, void *data) {
    (void)data;
    struct kbd *k = wl_container_of(l, k, mod);
    struct srv *s = k->s;
    wlr_seat_set_keyboard(s->seat, k->kb);
    wlr_seat_keyboard_notify_modifiers(s->seat, &k->kb->modifiers);
}

static void kb_die(struct wl_listener *l, void *data) {
    (void)data;
    struct kbd *k = wl_container_of(l, k, destroy);
    wl_list_remove(&k->key.link);
    wl_list_remove(&k->mod.link);
    wl_list_remove(&k->destroy.link);
    wl_list_remove(&k->link);
    caps_set(k->s);
    free(k);
}

static void ptr_die(struct wl_listener *l, void *data) {
    (void)data;
    struct ptr *p = wl_container_of(l, p, destroy);
    wlr_cursor_detach_input_device(p->s->cur, p->dev);
    wl_list_remove(&p->destroy.link);
    wl_list_remove(&p->link);
    caps_set(p->s);
    free(p);
}

static void req_cur(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, req_cur);
    struct wlr_seat_pointer_request_set_cursor_event *ev = data;
    if (s->seat->pointer_state.focused_client != ev->seat_client) {
        return;
    }
    wlr_cursor_set_surface(s->cur, ev->surface, ev->hotspot_x, ev->hotspot_y);
}

static void req_sel(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, req_sel);
    struct wlr_seat_request_set_selection_event *ev = data;
    wlr_seat_set_selection(s->seat, ev->source, ev->serial);
}

static void cur_mot(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, cur_mot);
    struct wlr_pointer_motion_event *ev = data;
    wlr_cursor_move(s->cur, &ev->pointer->base, ev->delta_x, ev->delta_y);
    cur_do(s, ev->time_msec);
}

static void cur_abs(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, cur_abs);
    struct wlr_pointer_motion_absolute_event *ev = data;
    wlr_cursor_warp_absolute(s->cur, &ev->pointer->base, ev->x, ev->y);
    cur_do(s, ev->time_msec);
}

static void cur_btn(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, cur_btn);
    struct wlr_pointer_button_event *ev = data;
    struct wlr_surface *sf = NULL;
    double sx = 0.0;
    double sy = 0.0;
    struct win *w = desk_at(s, s->cur->x, s->cur->y, &sf, &sx, &sy);

    uint32_t mod = 0;
    struct wlr_keyboard *kb = wlr_seat_get_keyboard(s->seat);
    if (kb) {
        mod = wlr_keyboard_get_modifiers(kb);
    }

    if (ev->state == WL_POINTER_BUTTON_STATE_PRESSED && mod_on(mod) && w) {
        if (ev->button == BTN_LEFT) {
            win_focus(w);
            grab_move(s, w);
            return;
        }
        if (ev->button == BTN_RIGHT) {
            win_focus(w);
            grab_resize(s, w, WLR_EDGE_BOTTOM | WLR_EDGE_RIGHT);
            return;
        }
    }

    wlr_seat_pointer_notify_button(s->seat, ev->time_msec, ev->button, ev->state);
    if (ev->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        s->mode = CURSOR_PASSTHROUGH;
        s->grab = NULL;
        return;
    }
    if (w && foc_click) {
        win_focus(w);
    }
}

static void cur_axis(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, cur_axis);
    struct wlr_pointer_axis_event *ev = data;
    wlr_seat_pointer_notify_axis(
        s->seat,
        ev->time_msec,
        ev->orientation,
        ev->delta,
        ev->delta_discrete,
        ev->source,
        ev->relative_direction
    );
}

static void cur_frm(struct wl_listener *l, void *data) {
    (void)data;
    struct srv *s = wl_container_of(l, s, cur_frm);
    wlr_seat_pointer_notify_frame(s->seat);
}

static void mon_frm(struct wl_listener *l, void *data) {
    (void)data;
    struct mon *m = wl_container_of(l, m, frame);
    struct srv *s = m->s;
    struct wlr_scene_output *so = wlr_scene_get_scene_output(s->scn, m->out);
    if (!so) {
        return;
    }

    wlr_scene_output_commit(so, NULL);

    struct timespec now = {0};
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(so, &now);
}

static void mon_die(struct wl_listener *l, void *data) {
    (void)data;
    struct mon *m = wl_container_of(l, m, destroy);
    struct srv *s = m->s;
    wl_list_remove(&m->frame.link);
    wl_list_remove(&m->destroy.link);
    wl_list_remove(&m->link);
    struct mon *dst = mon_first(s);
    mon_move_wins(s, m, dst);
    if (s->sel == m) {
        s->sel = dst;
    }
    ws_sync(s);
    tile(s);
    foc_top(s);
    free(m);
}

static void new_out(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, new_out);
    struct wlr_output *out = data;
    if (!wlr_output_init_render(out, s->alloc, s->rnd)) {
        return;
    }

    struct wlr_output_state st;
    wlr_output_state_init(&st);
    struct wlr_output_mode *m = wlr_output_preferred_mode(out);
    if (m) {
        wlr_output_state_set_mode(&st, m);
    }
    wlr_output_state_set_enabled(&st, true);
    wlr_output_commit_state(out, &st);
    wlr_output_state_finish(&st);

    struct mon *mo = calloc(1, sizeof(*mo));
    if (!mo) {
        return;
    }
    mo->s = s;
    mo->out = out;
    mo->ws = 0;
    mo->frame.notify = mon_frm;
    wl_signal_add(&out->events.frame, &mo->frame);
    mo->destroy.notify = mon_die;
    wl_signal_add(&out->events.destroy, &mo->destroy);
    wl_list_insert(&s->mons, &mo->link);
    if (!s->sel) {
        s->sel = mo;
    }

    struct wlr_scene_output *so = wlr_scene_output_create(s->scn, out);
    struct wlr_output_layout_output *lo = wlr_output_layout_add_auto(s->ol, out);
    wlr_scene_output_layout_add_output(s->sol, lo, so);

    wlr_xcursor_manager_load(s->xcm, out->scale);
    tile(s);
}

static void new_in(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, new_in);
    struct wlr_input_device *dev = data;

    if (dev->type == WLR_INPUT_DEVICE_POINTER) {
        wlr_cursor_attach_input_device(s->cur, dev);
        struct ptr *p = calloc(1, sizeof(*p));
        if (!p) {
            return;
        }
        p->s = s;
        p->dev = dev;
        p->destroy.notify = ptr_die;
        wl_signal_add(&dev->events.destroy, &p->destroy);
        wl_list_insert(&s->ptrs, &p->link);
    } else if (dev->type == WLR_INPUT_DEVICE_KEYBOARD) {
        struct wlr_keyboard *wk = wlr_keyboard_from_input_device(dev);
        struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!ctx) {
            fprintf(stderr, "lwm: xkb_context_new failed, using backend keymap\n");
        } else {
            struct xkb_keymap *km = xkb_keymap_new_from_names(ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
            if (!km) {
                fprintf(stderr, "lwm: xkb_keymap_new_from_names failed, using backend keymap\n");
            } else {
                wlr_keyboard_set_keymap(wk, km);
                xkb_keymap_unref(km);
            }
            xkb_context_unref(ctx);
        }
        wlr_keyboard_set_repeat_info(wk, kb_rate, kb_delay);

        struct kbd *k = calloc(1, sizeof(*k));
        if (!k) {
            return;
        }
        k->s = s;
        k->kb = wk;
        k->key.notify = kb_key;
        wl_signal_add(&wk->events.key, &k->key);
        k->mod.notify = kb_mod;
        wl_signal_add(&wk->events.modifiers, &k->mod);
        k->destroy.notify = kb_die;
        wl_signal_add(&dev->events.destroy, &k->destroy);
        wl_list_insert(&s->kbs, &k->link);
        wlr_seat_set_keyboard(s->seat, wk);
        if (s->foc && s->foc->surface) {
            win_focus(s->foc);
        }
    }

    caps_set(s);
}

static void win_map(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, map);
    struct srv *s = w->s;
    struct wlr_box box = {0};
    if (!w->m) {
        w->m = mon_cur(s);
    }
    if (w->ws < 0 || w->ws >= ws_n) {
        w->ws = w->m ? w->m->ws : 0;
    }
    struct wlr_output *out = w->m ? w->m->out : act_out(s);

    w->mapped = true;
    wl_list_insert(&s->wins, &w->link);
    wlr_scene_node_raise_to_top(&w->t->node);
    wlr_scene_node_set_enabled(&w->t->node, true);
    xdg_req_on(w);

    if (out) {
        wlr_output_layout_get_box(s->ol, out, &box);
    }

    int ww = win_pref_w(w) > 0 ? win_pref_w(w) : box.width / 2;
    int hh = win_pref_h(w) > 0 ? win_pref_h(w) : box.height / 2;
    int x = box.x + (box.width - ww) / 2;
    int y = box.y + (box.height - hh) / 2;
    win_geo(w, x, y, ww, hh);

    if (foc_new) {
        win_focus(w);
    }
    tile(s);
}

static void win_unmap(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, unmap);
    struct srv *s = w->s;
    if (!w->mapped) {
        return;
    }
    w->mapped = false;
    wl_list_remove(&w->link);
    xdg_req_off(w);
    if (s->foc == w) {
        foc_nil(s);
    }
    tile(s);
    foc_top(s);
}

static void win_commit(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, commit);
    if (!w->xdg || !w->xdg->toplevel) {
        return;
    }
    if (!w->xdg->initialized || w->xdg->configured) {
        return;
    }
    wlr_xdg_toplevel_set_size(w->xdg->toplevel, 0, 0);
}

static void win_die(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, destroy);
    if (w->mapped) {
        wl_list_remove(&w->link);
    }
    lis_drop(&w->map);
    lis_drop(&w->unmap);
    lis_drop(&w->commit);
    wl_list_remove(&w->destroy.link);
    xdg_req_off(w);
#if LWM_HAS_XWAYLAND
    lis_drop(&w->assoc);
    lis_drop(&w->dissoc);
#endif
    if (w->s->foc == w) {
        foc_nil(w->s);
    }
    if (w->s->grab == w) {
        w->s->grab = NULL;
        w->s->mode = CURSOR_PASSTHROUGH;
    }
    free(w);
}

static void win_req_move(struct wl_listener *l, void *data) {
    struct win *w = wl_container_of(l, w, req_move);
    struct srv *s = w->s;
    struct wlr_xdg_toplevel_move_event *ev = data;
    if (!wlr_seat_validate_pointer_grab_serial(s->seat, w->surface, ev->serial)) {
        return;
    }
    win_focus(w);
    grab_move(s, w);
}

static void win_req_resize(struct wl_listener *l, void *data) {
    struct win *w = wl_container_of(l, w, req_resize);
    struct srv *s = w->s;
    struct wlr_xdg_toplevel_resize_event *ev = data;
    if (!wlr_seat_validate_pointer_grab_serial(s->seat, w->surface, ev->serial)) {
        return;
    }
    win_focus(w);
    grab_resize(s, w, ev->edges);
}

static void xdg_req_fs(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, req_fs);
    if (!w->xdg || !w->xdg->toplevel) {
        return;
    }
    win_set_fullscreen(w, w->xdg->toplevel->requested.fullscreen);
}

static void new_xdg(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, new_xdg);
    struct wlr_xdg_toplevel *top = data;
    struct wlr_xdg_surface *xdg = top->base;
    struct win *w = calloc(1, sizeof(*w));

    if (!w) {
        return;
    }
    w->s = s;
    w->ws = -1;
    w->xdg = xdg;
    w->surface = xdg->surface;
    w->t = wlr_scene_xdg_surface_create(s->win, xdg);
    if (!w->t) {
        free(w);
        return;
    }
    w->t->node.data = w;
    wlr_scene_node_set_enabled(&w->t->node, false);

    w->map.notify = win_map;
    wl_signal_add(&xdg->surface->events.map, &w->map);
    w->unmap.notify = win_unmap;
    wl_signal_add(&xdg->surface->events.unmap, &w->unmap);
    w->commit.notify = win_commit;
    wl_signal_add(&xdg->surface->events.commit, &w->commit);
    w->destroy.notify = win_die;
    wl_signal_add(&xdg->events.destroy, &w->destroy);
    w->req_move.notify = win_req_move;
    w->req_resize.notify = win_req_resize;
    w->req_fs.notify = xdg_req_fs;
    win_rule(w);
}

#if LWM_HAS_XWAYLAND
static void xwl_assoc(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, assoc);
    if (!w->xwl || !w->xwl->surface || w->t) {
        return;
    }

    w->surface = w->xwl->surface;
    w->t = wlr_scene_subsurface_tree_create(w->s->win, w->surface);
    if (!w->t) {
        w->surface = NULL;
        return;
    }
    w->t->node.data = w;
    wlr_scene_node_set_enabled(&w->t->node, false);

    w->map.notify = win_map;
    wl_signal_add(&w->surface->events.map, &w->map);
    w->unmap.notify = win_unmap;
    wl_signal_add(&w->surface->events.unmap, &w->unmap);
}

static void xwl_dissoc(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, dissoc);
    if (w->mapped) {
        w->mapped = false;
        wl_list_remove(&w->link);
        if (w->s->foc == w) {
            foc_nil(w->s);
        }
    }
    lis_drop(&w->map);
    lis_drop(&w->unmap);
    win_destroy_scene(w);
    w->surface = NULL;
    tile(w->s);
    foc_top(w->s);
}

static void xwl_req_move(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, req_move);
    if (!w->surface) {
        return;
    }
    win_focus(w);
    grab_move(w->s, w);
}

static void xwl_req_resize(struct wl_listener *l, void *data) {
    struct win *w = wl_container_of(l, w, req_resize);
    struct wlr_xwayland_resize_event *ev = data;
    if (!w->surface) {
        return;
    }
    win_focus(w);
    grab_resize(w->s, w, ev->edges);
}

static void xwl_req_fs(struct wl_listener *l, void *data) {
    (void)data;
    struct win *w = wl_container_of(l, w, req_fs);
    if (!w->xwl) {
        return;
    }
    win_set_fullscreen(w, w->xwl->fullscreen);
}

static void new_xwl(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, new_xwl);
    struct wlr_xwayland_surface *xwl = data;
    struct win *w = calloc(1, sizeof(*w));
    if (!w) {
        return;
    }

    w->s = s;
    w->ws = -1;
    w->xwl = xwl;
    w->flt = xwl->override_redirect;

    w->destroy.notify = win_die;
    wl_signal_add(&xwl->events.destroy, &w->destroy);
    w->assoc.notify = xwl_assoc;
    wl_signal_add(&xwl->events.associate, &w->assoc);
    w->dissoc.notify = xwl_dissoc;
    wl_signal_add(&xwl->events.dissociate, &w->dissoc);
    w->req_move.notify = xwl_req_move;
    wl_signal_add(&xwl->events.request_move, &w->req_move);
    w->req_resize.notify = xwl_req_resize;
    wl_signal_add(&xwl->events.request_resize, &w->req_resize);
    w->req_fs.notify = xwl_req_fs;
    wl_signal_add(&xwl->events.request_fullscreen, &w->req_fs);
    win_rule(w);
}

static void xwl_ready(struct wl_listener *l, void *data) {
    (void)data;
    struct srv *s = wl_container_of(l, s, xwl_ready);
    if (!s->xwl || !s->xwl->display_name) {
        return;
    }
    setenv("DISPLAY", s->xwl->display_name, 1);
    fprintf(stdout, "lwm Xwayland on DISPLAY=%s\n", s->xwl->display_name);
    fflush(stdout);
    start_req(s);
}
#endif

static void lay_map(struct wl_listener *l, void *data) {
    (void)data;
    struct lay *ly = wl_container_of(l, ly, map);
    ly->mapped = true;
    tile(ly->s);
}

static void lay_unmap(struct wl_listener *l, void *data) {
    (void)data;
    struct lay *ly = wl_container_of(l, ly, unmap);
    ly->mapped = false;
    tile(ly->s);
}

static void lay_commit(struct wl_listener *l, void *data) {
    (void)data;
    struct lay *ly = wl_container_of(l, ly, commit);
    if (ly->mapped || lay_ready(ly)) {
        tile(ly->s);
    }
}

static void lay_die(struct wl_listener *l, void *data) {
    (void)data;
    struct lay *ly = wl_container_of(l, ly, destroy);
    wl_list_remove(&ly->map.link);
    wl_list_remove(&ly->unmap.link);
    wl_list_remove(&ly->commit.link);
    wl_list_remove(&ly->destroy.link);
    wl_list_remove(&ly->link);
    free(ly);
}

static void new_lay(struct wl_listener *l, void *data) {
    struct srv *s = wl_container_of(l, s, new_lay);
    struct wlr_layer_surface_v1 *ls = data;
    if (!ls->output) {
        struct mon *m = mon_cur(s);
        ls->output = m ? m->out : act_out(s);
    }

    struct lay *ly = calloc(1, sizeof(*ly));
    if (!ly) {
        return;
    }
    ly->s = s;
    ly->lyr = ls;
    ly->scn = wlr_scene_layer_surface_v1_create(lay_tree(s, ls->pending.layer), ls);
    if (!ly->scn) {
        free(ly);
        return;
    }

    ly->map.notify = lay_map;
    wl_signal_add(&ls->surface->events.map, &ly->map);
    ly->unmap.notify = lay_unmap;
    wl_signal_add(&ls->surface->events.unmap, &ly->unmap);
    ly->commit.notify = lay_commit;
    wl_signal_add(&ls->surface->events.commit, &ly->commit);
    ly->destroy.notify = lay_die;
    wl_signal_add(&ls->events.destroy, &ly->destroy);
    wl_list_insert(&s->lays, &ly->link);
    tile(s);
}

static void new_deco(struct wl_listener *l, void *data) {
    (void)l;
    struct wlr_xdg_toplevel_decoration_v1 *d = data;
    if (!d->toplevel || !d->toplevel->base || !d->toplevel->base->initialized) {
        return;
    }
    wlr_xdg_toplevel_decoration_v1_set_mode(d, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

int main(int argc, char **argv) {
    (void)argc;
    env_apply();
    dbus_boot(argv);
    set_sess_env();

    struct sigaction sa = {0};
    sa.sa_handler = sig_chld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    wlr_log_init(WLR_INFO, NULL);

    struct srv s = {0};
    wl_list_init(&s.wins);
    wl_list_init(&s.lays);
    wl_list_init(&s.kbs);
    wl_list_init(&s.ptrs);
    wl_list_init(&s.mons);
    s.layout = LAYOUT_TILE;
    s.ws = 0;
    s.ratio = main_ratio;
    s.gap = gap_px;
    s.mode = CURSOR_PASSTHROUGH;

    s.dpy = wl_display_create();
    if (!s.dpy) {
        fprintf(stderr, "lwm: wl_display_create failed\n");
        return 1;
    }

    s.be = wlr_backend_autocreate(wl_display_get_event_loop(s.dpy), &s.session);
    if (!s.be) {
        fprintf(stderr, "lwm: wlr_backend_autocreate failed\n");
        wl_display_destroy(s.dpy);
        return 1;
    }

#if LWM_HAS_XWAYLAND
    s.xwl_on = env_on("LWM_XWAYLAND", false);
#endif

    s.rnd = rnd_mk(s.be);
    s.alloc = wlr_allocator_autocreate(s.be, s.rnd);
    if (!s.rnd || !s.alloc) {
        fprintf(stderr, "lwm: renderer/allocator init failed\n");
        srv_fini(&s);
        wl_display_destroy(s.dpy);
        return 1;
    }

    if (!wlr_renderer_init_wl_display(s.rnd, s.dpy)) {
        fprintf(stderr, "lwm: renderer display init failed\n");
        srv_fini(&s);
        wl_display_destroy(s.dpy);
        return 1;
    }

    s.comp = wlr_compositor_create(s.dpy, 6, s.rnd);
    wlr_subcompositor_create(s.dpy);
    wlr_shm_create_with_renderer(s.dpy, 1, s.rnd);
    wlr_viewporter_create(s.dpy);
    wlr_single_pixel_buffer_manager_v1_create(s.dpy);
    if (!wlr_renderer_is_pixman(s.rnd)) {
        wlr_linux_dmabuf_v1_create_with_renderer(s.dpy, 5, s.rnd);
    }
    wlr_data_device_manager_create(s.dpy);

    s.ol = wlr_output_layout_create(s.dpy);
    wlr_xdg_output_manager_v1_create(s.dpy, s.ol);
    s.scn = wlr_scene_create();
    s.sol = wlr_scene_attach_output_layout(s.scn, s.ol);

    s.bg = wlr_scene_tree_create(&s.scn->tree);
    s.bot = wlr_scene_tree_create(&s.scn->tree);
    s.win = wlr_scene_tree_create(&s.scn->tree);
    s.top = wlr_scene_tree_create(&s.scn->tree);
    s.ovr = wlr_scene_tree_create(&s.scn->tree);

    s.xdg = wlr_xdg_shell_create(s.dpy, 6);
    s.lay = wlr_layer_shell_v1_create(s.dpy, 4);
    s.deco = wlr_xdg_decoration_manager_v1_create(s.dpy);
#if LWM_HAS_XWAYLAND
    if (s.xwl_on) {
        s.xwl = wlr_xwayland_create(s.dpy, s.comp, false);
    }
#endif

    s.cur = wlr_cursor_create();
    wlr_cursor_attach_output_layout(s.cur, s.ol);
    s.xcm = wlr_xcursor_manager_create(NULL, 24);
    s.seat = wlr_seat_create(s.dpy, "seat0");

    s.new_out.notify = new_out;
    wl_signal_add(&s.be->events.new_output, &s.new_out);
    s.new_in.notify = new_in;
    wl_signal_add(&s.be->events.new_input, &s.new_in);

    s.req_cur.notify = req_cur;
    wl_signal_add(&s.seat->events.request_set_cursor, &s.req_cur);
    s.req_sel.notify = req_sel;
    wl_signal_add(&s.seat->events.request_set_selection, &s.req_sel);

    s.cur_mot.notify = cur_mot;
    wl_signal_add(&s.cur->events.motion, &s.cur_mot);
    s.cur_abs.notify = cur_abs;
    wl_signal_add(&s.cur->events.motion_absolute, &s.cur_abs);
    s.cur_btn.notify = cur_btn;
    wl_signal_add(&s.cur->events.button, &s.cur_btn);
    s.cur_axis.notify = cur_axis;
    wl_signal_add(&s.cur->events.axis, &s.cur_axis);
    s.cur_frm.notify = cur_frm;
    wl_signal_add(&s.cur->events.frame, &s.cur_frm);

    s.new_xdg.notify = new_xdg;
    wl_signal_add(&s.xdg->events.new_toplevel, &s.new_xdg);
#if LWM_HAS_XWAYLAND
    if (s.xwl) {
        wlr_xwayland_set_seat(s.xwl, s.seat);
        s.xwl_ready.notify = xwl_ready;
        wl_signal_add(&s.xwl->events.ready, &s.xwl_ready);
        s.new_xwl.notify = new_xwl;
        wl_signal_add(&s.xwl->events.new_surface, &s.new_xwl);
    }
#endif
    s.new_lay.notify = new_lay;
    wl_signal_add(&s.lay->events.new_surface, &s.new_lay);
    s.new_deco.notify = new_deco;
    wl_signal_add(&s.deco->events.new_toplevel_decoration, &s.new_deco);

    const char *sock = wl_display_add_socket_auto(s.dpy);
    if (!sock) {
        fprintf(stderr, "lwm: wl_display_add_socket_auto failed\n");
        srv_fini(&s);
        wl_display_destroy(s.dpy);
        return 1;
    }

    setenv("WAYLAND_DISPLAY", sock, 1);
    set_sess_env();
    fprintf(stdout, "lwm on WAYLAND_DISPLAY=%s\n", sock);
#if LWM_HAS_XWAYLAND
    if (!s.xwl_on) {
        fprintf(stdout, "lwm: Xwayland=off (set LWM_XWAYLAND=1 to enable)\n");
    }
#else
    fprintf(stdout, "lwm: built without Xwayland support\n");
#endif
    if (!env_on("LWM_DBUS", false)) {
        fprintf(stdout, "lwm: D-Bus auto-start=off (set LWM_DBUS=1 to enable)\n");
    }
    fflush(stdout);

    if (!wlr_backend_start(s.be)) {
        fprintf(stderr, "lwm: backend start failed\n");
        srv_fini(&s);
        wl_display_destroy(s.dpy);
        return 1;
    }

    if (
#if LWM_HAS_XWAYLAND
        !s.xwl
#else
        true
#endif
    ) {
        start_req(&s);
    }

    wl_display_run(s.dpy);
    wl_display_destroy_clients(s.dpy);
    srv_fini(&s);
    wl_display_destroy(s.dpy);
    return 0;
}
