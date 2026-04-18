# Contributing to Phoc

Thank you for considering contributing to Phoc. See below for
contributing guidelines.

Please make sure to check our [Code of Conduct][coc]. For interactions
in this repository [GNOME's Code of Conduct][gnome-coc] also applies.

## Building

For build instructions, see the [README.md](./README.md)

### Merge requests

Before filing a merge request, run the tests:

```sh
meson test -C _build --print-errorlogs
```

In order to ensure long term maintenance and ease of review
we prefer a recipe-style git commit history in merge requests.
See our [developer guide][submitting] for details.

## Coding

### Coding Style

For coding style see our [developer documentation][coding-style].

## Examples

The `examples/` folder contains Wayland clients that exercise certain
protocols.  This is similar in spirit to [Weston clients][2] or
[wlr-clients][3].

- phosh-private: A client for the [phosh-private protocol](./protocols/phosh-private.xml)
- device-state: A client for the [phoc-device-state-unstable-v1 protocol](./protocols/phoc-device-state-unstable-v1.xml)
- layer-shell-effects: A client for the [phoc-layer-shell-effects-unstable-v1 protocol](./protocols/phoc-layer-shell-effects-unstable-v1.xml)
- ptk-demo: A demo client for multiple use cases. See `ptk-demo
  --list` for the implemented demos.

You can run them against any Phoc instance.

[1]: https://gitlab.gnome.org/GNOME/libhandy/-/blob/main/HACKING.md
[2]: https://gitlab.freedesktop.org/wayland/weston/-/tree/main/clients
[3]: https://gitlab.freedesktop.org/wlroots/wlr-clients
[coc]: https://ev.phosh.mobi/resources/code-of-conduct/
[gnome-coc]: https://conduct.gnome.org/
[submitting]: https://dev.phosh.mobi/docs/development/submitting/
[coding-style]: http://dev.phosh.mobi/docs/development/coding-style/
