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
