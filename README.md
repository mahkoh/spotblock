# spotblock

spotblock is a minimal spotify ad blocker daemon for GNU/Linux.

## Installation

### Dependencies

spotblock depends on the following libraries:

* libsystemd
* libpulse

Note that, while libsystemd is a dependency, you can use spotblock even if
you're not using systemd as your init system.

spotblock only works if you're using pulseaudio and requires a running dbus
daemon.

### Building

It's best to install spotblock via packages tailored to your distribution. For
example:

* [Arch Linux](https://aur.archlinux.org/packages/spotblock-git/)

Otherwise you can use the following commands:

```bash
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=/usr ..
$ make
$ sudo make install
```

This installs spotblock under `/usr/bin/spotblock`. Note that using the `/usr`
prefix is required if you want to use spotblock as a systemd service.

## Using

### As a systemd service

If you've installed spotblock under `/usr`, you can use it as a systemd service.
To start the service use

```bash
$ systemctl --user start spotblock
```

To make spotblock run automatically on login use

```bash
$ systemctl --user enable spotblock
```

### As a standalone binary

If you don't want to use spotblock as a systemd service you can also simply
start it by running the spotblock binary. How to make the binary run on login
depends on the distribution you're using.

## Troubleshooting

### spotblock mutes chromium

spotblock mutes some spotify ads which run inside of chromium processes.
Pulseaudio thinks that your chromium browser and those ads are the same kind of
process and so it carries the "muted" property over to your browser. To fix this
you have to give the spotify client a "module-stream-restore.id" that is
different from the one used by chromium. You can do this by writing a small
wrapper around spotify:

```bash
#!/bin/bash

export PULSE_PROP="module-stream-restore.id=spotify"
exec /usr/bin/spotify "$@"
```

### Failed to start dbus.service: Unit dbus.service not found.

spotblock depends on D-BUS as a systemd user service. This requires you to also run the D-BUS server as a systemd service. While this is the default in all newer versions of systemd and D-BUS, older version still run D-BUS through dbus-launch or other means when starting the Desktop environment (e.g. the version that Ubuntu 16.04 uses). 

To run D-BUS as a systemd service you will need to create the following two files. A dbus socket `/etc/systemd/user/dbus.socket`:

```
[Unit]
Description=D-Bus User Message Bus Socket

[Socket]
ListenStream=%t/bus

[Install]
WantedBy=sockets.target
Also=dbus.service
```

... and the associated dbus service `/etc/systemd/user/dbus.service`:

```
[Unit]
Description=D-Bus User Message Bus
Documentation=man:dbus-daemon(1)
Requires=dbus.socket

[Service]
ExecStart=/usr/bin/dbus-daemon --session --address=systemd: --nofork --nopidfile --systemd-activation
ExecReload=/usr/bin/dbus-send --print-reply --session --type=method_call --dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig

[Install]
Also=dbus.socket
```

You will also be required to make sure that the environmental variable `DBUS_SESSION_BUS_ADDRESS` is set properly. You can check if it is set by running `echo $DBUS_SESSION_BUS_ADDRESS` in your shell. To use systemd to do this as well, write the following into `/etc/systemd/system/user@.service.d/dbus.conf`

```
[Service]
Environment=DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/%I/bus
```

All that is left to do after this is to enable the dbus service for all users:

```
systemctl --global enable dbus.socket
```

To avoid having systemd and your desktop enviorment compete for how will get to start dbus will have to disable or remove your desktop environment's call of dbus-lauch. Depending on your `~/.xinitrc` or your systems `/etc/X11/xinit/xinitrc` this has to be done in diffrent ways. On Ubuntu 16.04 for example is enough to remove `use-session-dbus` from `/etc/X11/Xsession.options`.

## Notifications

If you want to be notified whenever spotblock mutes or unmutes spotify, you can
use the dbus service provided by spotblock.

spotblock is reachable under the `org.spotblock` name and provides the
`/org/spotblock` object. This object in turn provides the `org.spotblock`
interface which has the `Muted` property. This property is `true` if spotify is
muted, `false` otherwise. spotblock sends a signal whenever this property
changes.

The following Ruby script listens to this signal and prints the status change:

```ruby
#!/usr/bin/env ruby
require 'dbus'

bus = DBus::SessionBus.instance
proxy = bus.service("org.spotblock")
           .object("/org/spotblock")
proxy.introspect
props = proxy["org.freedesktop.DBus.Properties"]

puts props.Get('org.spotblock', 'Muted')[0]

props.on_signal("PropertiesChanged") do
    puts props.Get('org.spotblock', 'Muted')[0]
end

loop = DBus::Main.new
loop << bus
loop.run
```

## License

spotblock is free software licensed under GPLv3.
