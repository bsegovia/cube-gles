This code is a fork of the CUBE engine. Mostly OGL code has been revamped to
remove dependencies to GLU and dependencies to OGL1.x only features. So, the
code is basically using only gles 2 features making it compatible with webgl.

You need to dowload the data from sourceforge. They are here:

[http://sourceforge.net/projects/cube/] (http://sourceforge.net/projects/cube/)

Native builds on Windows and Linux
-----------------------------------

The code has been compiled with gcc on Linux and visual 2010/2012 on Windows.
On Linux:

`> cd src`

`> make`

On Windows, open the solution and compile


Javascript build
----------------

The code can also be compiled to javascript using emscripten. You need to have
the data in the same directory such emscripten can pre-package the complete
game. This is required to enable synchronous load of assets.

Just type:

`> ./emscripten.sh cube`

This will produce two files: `cube.html` that basically contains the code and
the html glue and `cube.data` which contains the prepackaged data.

The compilation takes a lot of time. Do not worry. The compilation profile uses
asm.js but it does not make a big difference with the latest Firefox nightly
build.

Once compiled, just open `cube.html` with Firefox

Chrome is more a problem. Performance are really low. I did not have time to
investigate why. For Chrome, you need to run a http server.

`python -m SimpleHTTPServer 8888`

and then, on chrome, open:

`localhost:8888/cube.html`

Remarks on JS port
------------------

- glReadPixels does not support depth target. Unfortunately, this is used to get
  the hit points for the main character weapon. So, no collision when you shoot
  yet :-(

- There is a crash when dealing with volume control I was not able to track down.
  So, the sound is not spatialized properly

- No network! Firefox (and partly Chrome) now supports webRTC and data channel.
  I did not have the time to look at it. Emscripten guys wrap sockets around it
  to have out of the box support of posix sockets. It may be interesting to try
  it (even if this may not be really high performance compared to a native use of
  webRTC, since most of the work (MTU handing, congestion control...) is done
  twice

