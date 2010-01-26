bmpanel2
========

bmpanel2 is a NETWM Compliant Panel for X11.

* Look & Feel customization via themes.
* Widgets: Desktop Switcher, Taskbar, Launchbar, System Tray, Clock, Decor, empty.
* Pseudo-transparency support.
* Written in C with speed and clarity in mind.
* Small amount of dependencies: glib2, cairo, pango, libX11.
* Small memory footprint (appox. 2mb to 4mb).
* Small executable (80kb at Current Time).

Installation
------------

	  git clone --depth=1 git://github.com/nsf/bmpanel2.git
    cd bmpanel2
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=RELEASE .
    make
    sudo make install

For More Information, Please refer to the Project Wiki, Page: Installation.

Usage
-----
  
    echo "theme xsocam_dark" > ~/.config/bmpanel2/bmpanel2rc
    bmpanel2

By default, The configuration file is stored in ~/.config/bmpanel/bmpanel2rc

For details on usage, Please refer to Unix Manual Pages or Short Help.

Short Help: <code>bmpanel2 --help</code><br/>
Unix Manual: <code>man bmpanel2</code>
 

