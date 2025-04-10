.. _phoc(1):

====
phoc
====

---------------------------------------
A Wayland compositor for mobile devices
---------------------------------------

SYNOPSIS
--------
|   **phoc** [OPTIONS...]


DESCRIPTION
-----------

``phoc`` is a Wayland compositor for mobile devices using the
``wlroots`` library. It's often used with the ``phosh`` mobile shell
but works perfectly fine on its own.

OPTIONS
-------

``-h``, ``--help``
   Print help and exit
``-C``, ``--config FILE``
   Path to the configuration file. (default: phoc.ini).
``-E``, ``--exec EXECUTABLE``
   Executable (session) that will be run at startup
``--socket``
   Wayland socket to listen on for client connections
``-S``, ``--shell``
   Whether to expect a shell to attach
``-X``, ``--xwayland``
   Whether to start XWayland
``--version``
   Show version information

CONFIGURATION
-------------

Configuration is read from ``phoc.ini``, ``hwdb`` and ``gsettings``.
For details on output configuration see ``phoc.ini(5)``, for details
on wakeup key configuration via ``hwdb`` see ``gmobile.udev(5)``, for details
on the gsettings handled by phoc see ``phoc.gsettings(5)``.

ENVIRONMENT VARIABLES
---------------------

``phoc`` honors the following environment variables:

- ``WLR_BACKENDS``: The backends the wlroots library should use when phoc launches. See
  https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/docs/env_vars.md
- ``WLR_RENDERER``: The renderer the wlroots library should use when phoc launches. See
  https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/docs/env_vars.md
- ``G_MESSAGES_DEBUG``, ``G_DEBUG`` and other environment variables supported
  by glib. https://docs.gtk.org/glib/running.html
- ``PHOC_DEBUG``: Comma separated list of debugging flags:

      - ``help``: Show a list of available debug flags
      - ``auto-maximize``: Maximize toplevels
      - ``damage-tracking``: Debug damage tracking
      - ``no-quit``: Don't quit when session ends
      - ``touch-points``: Debug touch points
      - ``layer-shell``: Debug layer shell
      - ``cutouts``: Debug display cutouts and notches
      - ``disable-animations``: Disable animations
      - ``force-shell-reveal``: Always reveal shell over fullscreen apps

UDEV PROPERTIES
---------------

Apart from the udev properties for wakeup keys documented in ``gmobile.udev(5)`` phoc uses
``ID_INPUT_KEYBOARD`` to identify hardware keyboards. You can use ``hwdb`` to override these.

DBUS INTERFACE
--------------

``phoc`` allows to enable and disable certain debug flags at runtime via DBus. To see a list of
available flags use:

::

  busctl --user introspect mobi.phosh.Phoc.DebugControl /mobi/phosh/Phoc/DebugControl mobi.phosh.Phoc.DebugControl

To toggle individual values:

::

  busctl --user set-property mobi.phosh.Phoc.DebugControl /mobi/phosh/Phoc/DebugControl mobi.phosh.Phoc.DebugControl TouchPoints b true
  busctl --user set-property mobi.phosh.Phoc.DebugControl /mobi/phosh/Phoc/DebugControl mobi.phosh.Phoc.DebugControl DamageTracking b true
  busctl --user set-property mobi.phosh.Phoc.DebugControl /mobi/phosh/Phoc/DebugControl mobi.phosh.Phoc.DebugControl LogDomains as 1 all
  busctl --user set-property mobi.phosh.Phoc.DebugControl /mobi/phosh/Phoc/DebugControl mobi.phosh.Phoc.DebugControl LogDomains as 2 phoc-seat phoc-layer-surface

Note that the flags are not considered stable API so can change
between releases.

See also
--------

``phoc.ini(5)`` ``phoc.gsettings(5)`` ``gmobile.udev(5)`` ``phosh(1)`` ``busctl``
