CC ?= cc
PKG_CONFIG ?= pkg-config
WAYLAND_SCANNER ?= wayland-scanner

CFLAGS ?= -O2 -pipe -std=c11 -Wall -Wextra -DWLR_USE_UNSTABLE
CPPFLAGS += -I.
WLROOTS_PKG ?= wlroots-0.19
BASE_PKGS := $(WLROOTS_PKG) xkbcommon wayland-server
XWAYLAND_PKG ?= xwayland
# Build stays usable without Xwayland development files.
HAVE_XWAYLAND := $(shell $(PKG_CONFIG) --exists $(XWAYLAND_PKG) && echo 1 || echo 0)

CPPFLAGS += $(shell $(PKG_CONFIG) --cflags $(BASE_PKGS))
LDLIBS += $(shell $(PKG_CONFIG) --libs $(BASE_PKGS))

ifeq ($(HAVE_XWAYLAND),1)
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags $(XWAYLAND_PKG))
LDLIBS += $(shell $(PKG_CONFIG) --libs $(XWAYLAND_PKG))
CPPFLAGS += -DLWM_HAS_XWAYLAND=1
else
CPPFLAGS += -DLWM_HAS_XWAYLAND=0
endif

SRCS := main.c
OBJS := $(SRCS:.c=.o)
PROTOCOLS_DIR := $(shell $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)
XDG_SHELL_XML := $(PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
XDG_SHELL_HDR := xdg-shell-protocol.h
WLR_LAYER_HDR := wlr-layer-shell-unstable-v1-protocol.h

.PHONY: all clean smoke smoke-dbus smoke-rofi smoke-waybar smoke-xwayland

all: lwm

$(XDG_SHELL_HDR): $(XDG_SHELL_XML)
	$(WAYLAND_SCANNER) server-header $< $@

$(WLR_LAYER_HDR): wlr-layer-shell-unstable-v1.xml
	$(WAYLAND_SCANNER) server-header $< $@

lwm: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c config.h $(WLR_LAYER_HDR) $(XDG_SHELL_HDR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o lwm $(WLR_LAYER_HDR) $(XDG_SHELL_HDR)

smoke: lwm
	sh -eu -c 'T=$${T:-3}; set +e; timeout $$T dbus-run-session -- ./lwm; st=$$?; [ $$st -eq 0 ] || [ $$st -eq 124 ]'

smoke-dbus: lwm
	./tools/smoke-dbus.sh

smoke-rofi: lwm
	./tools/smoke-rofi.sh

smoke-waybar: lwm
	./tools/smoke-waybar.sh

smoke-xwayland: lwm
	./tools/smoke-xwayland.sh
