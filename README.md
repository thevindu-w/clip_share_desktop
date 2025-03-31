<p align="center" width="100%">
<img src="https://raw.githubusercontent.com/thevindu-w/clip_share_client/master/fastlane/metadata/android/en-US/images/icon.png" alt="ClipShare" height="200" align="center"/>
</p>

# ClipShare - Desktop
### Share Clipboard and Files. Copy on one device. Paste on another device.

ClipShare is a lightweight and cross-platform tool for clipboard sharing. ClipShare enables copying text, files, and images on one device and pasted on another. ClipShare is simple and easy to use while being highly configurable. This is the desktop client of ClipShare that connects to a server running on the other machine to share copied text, files, and images.

## Download

<table>
<tr>
<th style="text-align:center">Server</th>
<td>
Download the server from <a href="https://github.com/thevindu-w/clip_share_server/releases">GitHub Releases</a>.
</td>
</tr>
<tr>
<th style="text-align:center">Mobile Client</th>
<td>
Download the mobile client app
from <a href="https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare">apt.izzysoft.de/fdroid</a>
or from <a href="https://github.com/thevindu-w/clip_share_client/releases">GitHub Releases</a>.
</td>
</tr>
<tr>
<th style="text-align:center">Desktop Client</th>
<td>
Download the desktop client app (this repository)
from <a href="https://github.com/thevindu-w/clip_share_desktop/releases">GitHub Releases</a>.
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
- Once the client starts, open a web browser (ex: Google Chrome, Microsoft Edge, Firefox, or any modern web browser) and visit [http://localhost:8888/](http://localhost:8888/). This should open the ClipShare client web app.
- Using the GUI client is same as using the [mobile client](https://github.com/thevindu-w/clip_share_client#how-to-use) except the desktop client uses the `Send File` button to send both copied files and folders.
- To use the desktop client to send copied text or files, copy the text or files you want to send (just like you would copy them to paste somewhere else). Then press the correct Send button on the web app (either `Send Text` or `Send File`). Then you can paste them on the machine that runs the [ClipShare server](https://github.com/thevindu-w/clip_share_server).
- To use the desktop client to receive copied text, image, or files, copy the text or files on the machine that runs the [ClipShare server](https://github.com/thevindu-w/clip_share_server). Then press the correct Get button on the web app (either `Get Text`, `Get Image`, or `Get File`). Then you can paste them on the machine that runs the client.
- If something goes wrong, it will create a `client_err.log` file. That file will contain what went wrong.

<br>

## Build from Source

**Note:** If you prefer using the pre-built binaries from [Releases](https://github.com/thevindu-w/clip_share_desktop/releases), you can ignore this section and follow the instructions in the [How to Use](#how-to-use) section.

### Build tools

  Compiling ClipShare needs the following tools,

* gcc
* make

<details>
  <summary>Linux</summary>

  On Linux, these tools can be installed with the following command:

* On Debian-based or Ubuntu-based distros,
  ```bash
  sudo apt-get install gcc make
  ```

* On Redhat-based or Fedora-based distros,
  ```bash
  sudo yum install gcc make
  ```

* On Arch-based distros,
  ```bash
  sudo pacman -S gcc make
  ```
</details>

<details>
  <summary>Windows</summary>

  On Windows, these tools can be installed with [MinGW](https://www.mingw-w64.org/).<br>
  In an [MSYS2](https://www.msys2.org/) environment, these tools can be installed using pacman with the following command:
  ```bash
  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
  ```
  You may need to rename (or copy) the `<MSYS2 directory>/mingw64/bin/mingw32-make.exe` to `<MSYS2 directory>/mingw64/bin/make.exe` before running the command `make`
</details>

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

In an [MSYS2](https://www.msys2.org/) environment, these tools can be installed using pacman with the following command:
```bash
pacman -S mingw-w64-x86_64-libunistring
```

However, installing libmicrohttpd from GNU is recommended. You can download the library from [https://ftpmirror.gnu.org/libmicrohttpd/libmicrohttpd-latest-w32-bin.zip](https://ftpmirror.gnu.org/libmicrohttpd/libmicrohttpd-latest-w32-bin.zip) and extract the files in it to correct include and library directories in the MSYS2 environment.
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
