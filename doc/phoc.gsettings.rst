.. _phoc.gsettings(5):

==============
phoc.gsettings
==============

----------------------
Gsettings used by phoc
----------------------

DESCRIPTION
-----------

Phoc uses gsettings for most of its configuration. You can use the ``gsettings(1)`` command
to change them or to get details about the option. To get more details about an option use
`gsettings describe`, e.g.:

::

   gsettings describe sm.puri.phoc auto-maximize

These are the currently used gsettings schema and keys:

GSettings
~~~~~~~~~

These gsettings are used by ``phoc``:

- `mobi.phosh.phoc`:

    - `focus-frame`: Whether to show a focus frame around focused toplevels

- Input sources: `org.gnome.desktop.input-sources`

    - `sources`: Current input sources
    - `xkb-options`: XKB Options

- Interface: `org.gnome.desktop.interface`

    - `enable-animations`
    - `cursor-size`: Cursor size
    - `cursor-theme` : Cursor theme

- Touchscreen: `org.gnome.desktop.peripherals.touchscreen`

    - `output`: EDID information of the output the touchscreen is mapped to. This is a
       relocatable schema at `/org/gnome/desktop/peripherals/`.

- Pointer: `org.gnome.desktop.peripherals.mouse`

    - `middle-click-emulation`
    - `natural-scroll`
    - `speed`

- Tablet: `org.gnome.desktop.peripherals.tablet`

    - `output`: EDID information of the output the tablet is mapped to. This is a
       relocatable schema at `/org/gnome/desktop/peripherals/`.

- Touchpad: `org.gnome.desktop.peripherals.touchpad`

    - `disable-while-typing`
    - `edge-scrolling-enabled`
    - `left-handed`
    - `middle-click-emulation`
    - `speed`
    - `tan-and-drag-lock`
    - `tap-and-drag`
    - `tap-to-click`
    - `two-finger-scrolling-enabled`

- Keyboard: `org.gnome.desktop.peripherals.keyboard`

    - `repeat`: Enable key repeat
    - `repeat-interval`: Delay between repeats in milliseconds
    - `delay`: Initial key repeat delay in milliseconds

- Keybindings: `org.gnome.desktop.wm.keybindings`

    - `always-on-top`
    - `close`
    - `cycle-windows-backward`
    - `cycle-windows-backward`
    - `cycle-windows`
    - `maximize`
    - `move-monitor-up`, `move-monitor-down`, `move-monitor-left`, `move-monitor-right`,
    - `move-to-corner-ne`, `move-to-corner-nw`, `move-to-corner-se`, `move-to-corner-sw`
    - `switch-applications-backward`
    - `switch-applications`
    - `switch-input-source`
    - `switch-to-workspace-left`, `switch-to-workspace-right`
    - `switch-to-workspace-1`, `switch-to-workspace-2`, …, `switch-to-workspace-6`
    - `toggle-fullscreen`
    - `toggle-maximzed`
    - `toggle-tiled-left`
    - `toggle-tiled-right`
    - `unmaximize`

- `org.gnome.desktop.wm.preferences`

    - `num-workspaces`

- Legacy schema: `sm.puri.phoc`

    - `automaximize`
    - `scale-to-fit`

See also
--------

``phoc(1)`` ``phoc.ini(5)`` ``gsettings(1)``
