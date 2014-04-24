Network speed indicator for Unity
=================================

![](https://raw.github.com/mgedmin/indicator-netspeed/master/screenshot.png)

Usage
-----

```
sudo apt-get install build-essential libgtop2-dev libgtk-3-dev libappindicator3-dev git-core
git clone git://github.com/kjyv/indicator-netspeed.git
cd indicator-netspeed
make
sudo make install
indicator-netspeed &
```

The indicator will be put left of all your other indicators. If this is undesirable, the ordering
index can be changed in gsettings:/apps/indicators/netspeed (use dconf-editor).


TODO
----

* Configuration options only accessible through dconf-editor, add a simple options menu.
* Allow toggling autostart at login.
* Do some magic when interfaces are added or disappear.
* The Makefile is a bit stupid right now and is probably specific to ubuntu.
* It's not packaged, create deb building scripts.

Credits
-------

Originally written by Marius Gedminas <marius@gedmin.as>

Contributors:

- Tobias Brandt <tob.brandt@gmail.com>
- Stefan Bethge (stefan at lanpartei.de)

