# MythTeatime
Mythteatime is small mythtv plugin which lets you create and edit timed events. Think of it as a poor impementation of cron which runs on top of mythfrontend.

## Features
  - create as many timers as needed
  - define timers by timespan or with a fixed point in time
  - run jumppoints when a timer expires
  - run system key events when a timer expires
  - run custom commands when a timer expires

## Installation
change into the mythteatime directory and run

    git clone git://github.com/tobser/mythteatime.git
    cd mythteatime
    ./configure
    make
    sudo make install

## Configuration
Once the plugin is installed, start mythfrontend. MythTeatime should have
created a new jump point, which you can use to create and modify your timers.
Go to `"Setup"->"Edit Keys"->"JumpPoints"->"Teatimer"` and assign a key to
the `Teatimer` JumpPoint.

If you prefere a menu entry over invoking the jumppoint from your remote
you may simply add a button to your menu (see the [Menu theme development
guide](http://www.mythtv.org/wiki/Menu_theme_development_guide))

    <button>
       <text>Timer</text>
       <action>JUMP Teatimer</action>
    </button>

# Issues
    - you can only set the timer while not in playback
    - to stop a started timer you have to set the invokation time to a time in the past
