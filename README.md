Network speed indicator for Unity
=================================

![](https://github.com/mgedmin/indicator-netspeed/blob/master/screenshot.png)


Usage
-----

```
sudo apt-get install build-essential libgtop2-dev libgtk-3-dev libappindicator3-dev git-core
git clone git://github.com/mgedmin/indicator-netspeed.git
cd indicator-netspeed
make
./indicator-netspeed
```


Bugs
----

The indicator keeps changing size to match the text, which pushes other
indicators left and right all the time, which makes it very annoying and
unusable.

It's not packaged, so installation procedure is very inconvenient.


Credits
-------

Written by Marius Gedminas <marius@gedmin.as>

Contributors:

- Tobias Brandt <tob.brandt@gmail.com> 
