# ClipShare - Desktop
### Share Clipboard and Files. Copy on one device. Paste on another device.

ClipShare is a lightweight and cross-platform tool for clipboard sharing. ClipShare enables copying text, files, and images on one device and pasted on another. ClipShare is simple and easy to use while being highly configurable. This is the desktop client of ClipShare that connects to a server running on the other machine to share copied text, files, and images.

## Download

### Server

<table>
<tr>
<th style="text-align:center">Desktop</th>
</tr>
<tr>
<td align="center">
<a href="https://github.com/thevindu-w/clip_share_server/releases"><img src="https://raw.githubusercontent.com/thevindu-w/clip_share_client/master/fastlane/metadata/android/en-US/images/icon.png" alt="Get it on GitHub" width="100"/></a><br>
Download the server from <a href="https://github.com/thevindu-w/clip_share_server/releases">Releases</a>.
</td>
</tr>
</table>

### Client

<table>
<tr>
<th style="text-align:center">Android</th>
<th style="text-align:center">Desktop</th>
</tr>
<tr>
<td align="center">
<a href="https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare"><img src="https://gitlab.com/IzzyOnDroid/repo/-/raw/master/assets/IzzyOnDroid.png" alt="Get it on IzzyOnDroid" width="250"/></a><br>
Download the Android client app
from <a href="https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare">apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare</a>.<br>
or from <a href="https://github.com/thevindu-w/clip_share_client/releases">GitHub Releases</a>.
</td>
<td align="center">
<a href="https://github.com/thevindu-w/clip_share_desktop/releases"><img src="https://raw.githubusercontent.com/thevindu-w/clip_share_client/master/fastlane/metadata/android/en-US/images/icon.png" alt="Get it on GitHub" width="100"/></a><br>
Download the desktop client from <a href="https://github.com/thevindu-w/clip_share_desktop/releases">Releases</a>.
</td>
</tr>
</table>

<br>

## Table of Contents

- [How to Use](#how-to-use)
  - [GUI client](#gui-client)
- [Build from Source](#build-from-source)
  - [Build tools](#build-tools)
  - [Dependencies](#dependencies)
  - [Compiling](#compiling)

<br>

## How to Use

### GUI client

The GUI client provides a web interface similar to the mobile client app. The client app acts as a bridge to use the web interface. To use the client in this web GUI mode, you need to run the client program.
- You can run the client from a terminal or by double-clicking on the program (if the file manager supports executing programs in that way).
- When the client starts, it will not display any visible window or produce any output.
- Once the client starts, open a web browser (ex: Google Chrome, Microsoft Edge, Firefox, or any modern web browser) and visit [http://localhost:8888/](http://localhost:8888/). This should open the ClipShare client web app. (The port 8888 can be changed in the configuration file)
- Using the GUI client is same as using the [mobile client](https://github.com/thevindu-w/clip_share_client#how-to-use) except the desktop client uses the `Send File` button to send both copied files and folders.
- To use the desktop client to send copied text or files, copy the text or files you want to send (just like you would copy them to paste somewhere else). Then press the correct Send button on the web app (either `Send Text` or `Send File`). Then you can paste them on the machine that runs the [ClipShare server](https://github.com/thevindu-w/clip_share_server).
- To use the desktop client to receive copied text, image, or files, copy the text or files on the machine that runs the [ClipShare server](https://github.com/thevindu-w/clip_share_server). Then press the correct Get button on the web app (either `Get Text`, `Get Image`, or `Get File`). Then you can paste them on the machine that runs the client.
- If something goes wrong, it will create a `client_err.log` file. That file will contain what went wrong.

<br>

## Build from Source

**Note:** If you prefer using the pre-built binaries from [Releases](https://github.com/thevindu-w/clip_share_desktop/releases), you can ignore this section and follow the instructions in the [How to Use](#how-to-use) section.

### Build tools

  Compiling ClipShare Desktop needs the following tools,

* gcc
* make
* xxd

<details>
  <summary>Linux</summary>

  On Linux, these tools can be installed with the following command:

* On Debian-based or Ubuntu-based distros,
  ```bash
  sudo apt-get install gcc make xxd
  ```

* On Redhat-based or Fedora-based distros,
  ```bash
  sudo yum install gcc make xxd
  ```

* On Arch-based distros,
  ```bash
  sudo pacman -S gcc make tinyxxd
  ```
</details>

<details>
  <summary>Windows</summary>

  On Windows, these tools can be installed with [MinGW](https://www.mingw-w64.org/).<br>
  In an [MSYS2](https://www.msys2.org/) environment, these tools can be installed using pacman with the following command:
  ```bash
  pacman -S mingw-w64-x86_64-gcc make vim
  ```
</details>

<details>
  <summary>macOS</summary>

  On macOS, these tools are installed with Xcode Command Line Tools.
</details>

<br>

### Dependencies

<details>
  <summary>Linux</summary>

  The following development libraries are required.

* libc
* libx11
* libxmu
* libunistring
* libmicrohttpd

  They can be installed with the following command:

* On Debian-based or Ubuntu-based distros,
  ```bash
  sudo apt-get install libc6-dev libx11-dev libxmu-dev libunistring-dev libmicrohttpd-dev
  ```

* On Redhat-based or Fedora-based distros,
  ```bash
  sudo yum install glibc-devel libX11-devel libXmu-devel libunistring-devel libmicrohttpd-devel
  ```

* On Arch-based distros,
  ```bash
  sudo pacman -S libx11 libxmu libunistring libmicrohttpd
  ```

  glibc should already be available on Arch distros. But you may need to upgrade it with the following command. (You need to do this only if the build fails)

  ```bash
  sudo pacman -S glibc
  ```

</details>

<details>
  <summary>Windows</summary>

  The following development libraries are required.

* [libmicrohttpd](https://ftpmirror.gnu.org/libmicrohttpd/)
* [libunistring](https://packages.msys2.org/package/mingw-w64-x86_64-libunistring?repo=mingw64)

In an [MSYS2](https://www.msys2.org/) environment, these libraries can be installed using pacman with the following command:
```bash
pacman -S mingw-w64-x86_64-libunistring
```

However, installing libmicrohttpd from GNU is recommended. You can download the library from [ftpmirror.gnu.org/libmicrohttpd/libmicrohttpd-latest-w32-bin.zip](https://ftpmirror.gnu.org/libmicrohttpd/libmicrohttpd-latest-w32-bin.zip) and extract the files in it to the correct include and library directories in the MSYS2 environment. Alternatively, you may extract the library to a separate directory and set the `CPATH` and `LIBRARY_PATH` environment variables to the include and link-library paths, respectively.
</details>

<details>
  <summary>macOS</summary>

The following development libraries are required.

* [libmicrohttpd](https://ftpmirror.gnu.org/libmicrohttpd/)
* [libunistring](https://formulae.brew.sh/formula/libunistring)

These libraries can be installed using [Homebrew](https://brew.sh) with the following command:
```bash
brew install libunistring
```

However, installing libmicrohttpd from GNU source is recommended. You can download the library source from [ftpmirror.gnu.org/libmicrohttpd/libmicrohttpd-latest.tar.gz](https://ftpmirror.gnu.org/libmicrohttpd/libmicrohttpd-latest.tar.gz) and compile it with the following commands.
```bash
tar -xzf libmicrohttpd-latest.tar.gz
cd libmicrohttpd*
LIBMICROHTTPD_PATH=~/libmicrohttpd # This is where the compiled library and header files are installed. Change the path as neccessary
mkdir -p "$LIBMICROHTTPD_PATH"
./configure --without-gnutls --prefix="$LIBMICROHTTPD_PATH"
make -j4
make install # If you selected a system path above for LIBMICROHTTPD_PATH, this command may need sudo
```

Then, set the `CPATH` and `LIBRARY_PATH` environment variables, respectively, to the include and link-library paths in the directory where libmicrohttpd is installed (value of `LIBMICROHTTPD_PATH` in above commands). Delete the dylib file in the libmicrohttpd installation if you want to static link libmicrohttpd.
</details>

<br>

### Compiling

1. Open a terminal / command prompt / Powershell in the project directory

    This can be done using the GUI or the `cd` command.

1. Run the following command to make the executable file

    ```bash
    make
    ```
    This will generate the executable named clip-share-client (or clip-share-client.exe on Windows).
