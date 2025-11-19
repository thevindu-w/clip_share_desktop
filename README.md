# ClipShare - Desktop
### Share Clipboard and Files. Copy on one device. Paste on another device.

![Build and Test](https://github.com/thevindu-w/clip_share_desktop/actions/workflows/build-test.yml/badge.svg?branch=master)
[![Last commit](https://img.shields.io/github/last-commit/thevindu-w/clip_share_desktop.svg?color=yellow)](https://github.com/thevindu-w/clip_share_desktop/commits/master)
[![License](https://img.shields.io/github/license/thevindu-w/clip_share_desktop.svg?color=blue)](https://www.gnu.org/licenses/gpl-3.0.en.html#license-text)
[![Latest release](https://img.shields.io/github/v/release/thevindu-w/clip_share_desktop?color=teal)](https://github.com/thevindu-w/clip_share_desktop/releases)

<br>

ClipShare is a lightweight and cross-platform tool for clipboard sharing. ClipShare enables copying text, files, and images on one device and pasting on another. ClipShare is simple and easy to use while being highly configurable. This is the desktop client of ClipShare that connects to a server running on the other machine to share copied text, files, and images.

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
Download the Android client app from <a href="https://apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare">apt.izzysoft.de/fdroid/index/apk/com.tw.clipshare</a><br>
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
  - [CLI client](#cli-client)
  - [Installation](#installation)
  - [Create SSL/TLS certificates and key files](#create-ssltls-certificates-and-key-files) (optional)
  - [Configuration](#configuration)
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
- Using the GUI client is similar to using the [mobile client](https://github.com/thevindu-w/clip_share_client#how-to-use) except the desktop client uses the `Send File` button to send both copied files and folders.
- To use the desktop client to send copied text or files, copy the text or files you want to send (just like you would copy them to paste somewhere else). Then press the correct Send button on the web app (either `Send Text` or `Send File`). Then you can paste them on the machine that runs the [ClipShare server](https://github.com/thevindu-w/clip_share_server).
- To use the desktop client to receive copied text, an image, or files, copy the text or files on the machine that runs the [ClipShare server](https://github.com/thevindu-w/clip_share_server). Then press the correct Get button on the web app (either `Get Text`, `Get Image`, or `Get File`). Then you can paste them on the machine that runs the client.
- If something goes wrong, it will create a `client_err.log` file. That file will contain what went wrong.

### CLI client

The CLI client enables using this desktop client with a command. To use the client in this CLI mode, you need to run the client program with the following command line arguments.
```bash
clip-share-client -c COMMAND <server-address-ipv4> [optional args]
```
**Commands available:**
- `sc`&nbsp;&nbsp;: Scan - server address is not needed
- `g `&nbsp;&nbsp;: Get copied text
- `s `&nbsp;&nbsp;: Send copied text
- `fg`&nbsp;&nbsp;: Get copied files
- `fs`&nbsp;&nbsp;: Send copied files
- `i `&nbsp;&nbsp;: Get image
- `ic`&nbsp;&nbsp;: Get copied image
- `is`&nbsp;&nbsp;: Get screenshot - Display number can be used as an optional arg.

**Examples:**
```bash
# Get copied text from the device having IP address 192.168.21.42
clip-share-client -c g 192.168.21.42
```
```bash
# Get a screenshot of screen number 1 from the device having IP address 192.168.21.42
clip-share-client -c is 192.168.21.42 1
```
```bash
# Scan and output IP addresses of available servers.
clip-share-client -c sc
```

<br>

### Installation

**Note:** This section is optional if you prefer manually starting the server over automatically starting on login/reboot.

#### Online installers

Online installer scripts are attached with the releases. They will download the corresponding version during the installation. Therefore, they do not require having the compiled binaries, along with the installer, to run it. However, they require internet access to download the binaries from GitHub during the installation. Run the interactive script and follow the instructions to install ClipShare desktop.

<details>
  <summary>Linux and macOS</summary>

1. Open a terminal in the directory where the `install-linux-mac.sh` installer script is downloaded.
1. Run the install script as shown below, and follow the instructions of it.
```bash
chmod +x install-linux-mac.sh
./install-linux-mac.sh
```
</details>

<details>
  <summary>Windows</summary>

1. Download the `install-windows.bat` file from [release assets](https://github.com/thevindu-w/clip_share_desktop/releases/latest).
1. Double-click on the `install-windows.bat` installer script to run it. It will open a Command Prompt window. Follow the instructions on it to install ClipShare. (If double-clicking did not run the installer, right-click on it and select Run)
</details>
<br>

#### Standalone installers

Standalone installer scripts are available in the archives (`zip` for Windows and macOS, and `tar.gz` for Linux) attached to releases on GitHub. They are also available in the [helper_tools/](https://github.com/thevindu-w/clip_share_desktop/tree/master/helper_tools) directory. You must have the `clip-share-client` (or `clip-share-client.exe` on Windows) executable in the current working directory to run the installer. If you download the archive from releases and extract it, you will have the executable, along with the installer script. Run the interactive script and follow the instructions to install ClipShare.

**Note:** These installers do NOT need admin or superuser privileges to run. Therefore, do not run them with elevated privileges.

<details>
  <summary>Linux and macOS</summary>

1. Open a terminal in the directory where the `clip-share-client` executable is available (the executable name may have suffixes like `_GLIBC*` on Linux or `arm64` or `x86_64` on macOS).
1. Run the install script as shown below, and follow the instructions of it.
```bash
# on Linux
chmod +x install-linux.sh
./install-linux.sh
```
```bash
# on macOS
chmod +x install-mac.sh
./install-mac.sh
```
</details>

<details>
  <summary>Windows</summary>

1. Place the `install-windows.bat` file and the `clip-share-client.exe` executable in the same folder. (the executable name may have suffixes)
1. Double-click on the `install-windows.bat` installer script to run it. It will open a Command Prompt window. Follow the instructions on it to install ClipShare. (If double-clicking did not run the installer, right-click on it and select Run)
</details>

<br>

### Create SSL/TLS certificates and key files

**Note:** This section is optional if you do not need the TLS encrypted mode.

The following files should be created, and their paths should be specified in the configuration file `clipshare.conf`. You may use different file names and paths to store the keys and certificates.

* `client.pfx` &ensp; - &nbsp; SSL/TLS key and certificate file of the client
* `ca.crt` &emsp;&emsp;&ensp; - &nbsp; SSL/TLS certificate of the CA, which signed both the server's and the client's SSL/TLS certificates

Refer to the [Create SSL/TLS certificates and key files](https://github.com/thevindu-w/clip_share_server#create-ssltls-certificates-and-key-files) section for more information on generating the TLS keys and certificates.

<br>

### Auto-send options

The ClipShare desktop client can auto-send text and files when copied. This feature allows you to copy text or files on one device and paste on another device without manually sharing the content through the desktop client's interface. Use the following configuration options to enable this feature.

* `auto_send_text`=`true` &ensp; - &nbsp; To auto-send copied text.
* `auto_send_files`=`true` &ensp; - &nbsp; To auto-send copied files.

It is recommended to set the `cut_sent_files` [configuration of the server](https://github.com/thevindu-w/clip_share_server#configuration) to `true` when using the auto-send feature for files.
Furthermore, use a temporary files directory as the `working_dir` configuration of the server when using auto-send option for files to avoid persisting unintended files on the server. You may use the following directories as the temporary directory, depending on the OS.

| OS | Temporary directory |
|  :----:  | :--------  |
| Windows | `C:\Users\<your_username>\AppData\Local\Temp` |
| macOS | `/tmp/` |
| Linux | `/tmp/` |

Refer to the [Configuration](#configuration) section for more information on the config file and available configuration options.

To use the auto-send feature to bi-directionally share copied text and files, you need to run both the server and the desktop client on both devices.

<br>

### Configuration

The ClipShare desktop client can be configured using a configuration file. The configuration file should be named `clipshare-desktop.conf`.
The application searches for the configuration file in the following paths in the same order until it finds one.
1. Current working directory where the application was started.
1. `$XDG_CONFIG_HOME` directory if the variable is set to an existing directory (on Linux and macOS only).
1. `~/.config/` directory if the directory exists (on Linux and macOS only).
1. Current user's home directory (also called user profile directory on Windows).

If it can't find a configuration file in any of the above directories, it will use the default values specified in the table below.

To customize the client, create a file named &nbsp; `clipshare-desktop.conf` &nbsp; in any of the directories mentioned above and add the following lines to that configuration file. You may omit some lines to keep the default values.
<details>
  <summary>Sample configuration file</summary>

```properties
app_port=4337
app_port_secure=4338
web_port=8888

secure_mode_enabled=true
client_cert=cert/client.pfx
ca_cert=cert/ca.crt
trusted_servers=trusted_servers.txt

working_dir=./path/to/work_dir
bind_address=127.0.0.1
cut_received_files=false

max_text_length=4194304
max_file_size=68719476736

min_proto_version=1
max_proto_version=3

auto_send_text=false
auto_send_files=false

# Windows and macOS only
tray_icon=true
```
</details>

Note that all the lines in the configuration file are optional. You may omit some lines if they need to get their default values.
<br>
<details>
  <summary>Configuration options</summary>

| Property | Description | Accepted values | Default |
|  :----:  | :--------   | :------------   |  :---:  |
| `app_port` | The TCP port on which the server listens for unencrypted TCP connections. (Refer to the corresponding configuration value of the server) | TCP port number used for the server (1 - 65535) | `4337` |
| `app_port_secure` | The TCP port on which the server listens for TLS-encrypted connections. (Refer to the corresponding configuration value of the server) | TCP port number used for the server (1 - 65535) | `4338` |
| `web_port` | The TCP port on which the application listens for connections from the web browser. This setting is used only for the GUI client. (Values below 1024 may require superuser/admin privileges) | Any valid, unused TCP port number (1 - 65535) | `8888` |
| `secure_mode_enabled` | Whether or not the application listens for TLS-encrypted connections. The values `true` or `1` will enable it, while `false` or `0` will disable it. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `client_cert` | The TLS key and certificate store file of the client. This must be specified if the secure mode is enabled. | Absolute or relative path to the client's TLS certificate store PKCS#12 file | \<Unspecified\> |
| `ca_cert` | The TLS certificate file of the CA that signed the TLS certificate of the client. This must be specified if the secure mode is enabled. | Absolute or relative path to the TLS certificate PEM file of the CA | \<Unspecified\> |
| `trusted_servers` | The text file containing a list of trusted servers (Common Name of server certificate), one name per line. This must be specified if the secure mode is enabled. | Absolute or relative path to the trusted-servers file | \<Unspecified\> |
| `working_dir` | The working directory where the application should run. All the files and images, that are fetched from the server, will be saved in this directory. It will follow symlinks if this is a path to a symlink. The user running this application should have write access to the directory | Absolute or relative path to an existing directory | `.` (i.e. Current directory) |
| `bind_address` | The address of the interface to which the application should bind when listening for connections from the web browser in the GUI mode. It will listen on all interfaces if this is set to `0.0.0.0`. (Usually, this should have the loopback address `127.0.0.1` except for some rare cases) | IP address of an interface or wildcard address. IPv4 dot-decimal notation (ex: `192.168.37.5`) or `0.0.0.0` | `127.0.0.1` |
| `max_text_length` | The maximum length of text that can be transferred. This is the number of bytes of the text encoded in UTF-8. | Any integer between 1 and 4294967295 (nearly 4 GiB) inclusive. Suffixes K, M, and G (case insensitive) denote x10<sup>3</sup>, x10<sup>6</sup>, and x10<sup>9</sup>, respectively. | 4194304 (i.e. 4 MiB) |
| `max_file_size` | The maximum size of a single file in bytes that can be transferred. | Any integer between 1 and 9223372036854775807 (nearly 8 EiB) inclusive. Suffixes K, M, G, and T (case insensitive) denote x10<sup>3</sup>, x10<sup>6</sup>, x10<sup>9</sup>, and x10<sup>12</sup>, respectively. | 68719476736 (i.e. 64 GiB) |
| `cut_received_files` | Whether to automatically cut the files into the clipboard on the _Get Files_ and _Get Image_ methods. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `min_proto_version` | The minimum protocol version the client should accept from a server after negotiation. | Any protocol version number greater than or equal to the minimum protocol version the client has implemented. (ex: `1`) | The minimum protocol version the client has implemented |
| `max_proto_version` | The maximum protocol version the client should accept from a server after negotiation. | Any protocol version number less than or equal to the maximum protocol version the client has implemented. (ex: `3`) | The maximum protocol version the client has implemented |
| `auto_send_text` | Whether the application should auto-send the text when copied. The values `true` or `1` will enable auto-sending copied text, while `false` or `0` will disable the feature. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `auto_send_files` | Whether the application should auto-send files when copied. The values `true` or `1` will enable auto-sending copied files, while `false` or `0` will disable the feature. | `true`, `false`, `1`, `0` (Case insensitive) | `false` |
| `tray_icon` | Whether the application should display a system tray icon when running in GUI mode. This option is available only on Windows and macOS. The values `true` or `1` will display the icon, while `false` or `0` will prevent displaying the icon. | `true`, `false`, `1`, `0` (Case insensitive) | `true` |

<br>
<br>

</details>

If you changed the configuration file, you must restart the server to apply the changes.
<br>
<br>

## Build from Source

**Note:** If you prefer using the pre-built binaries from [Releases](https://github.com/thevindu-w/clip_share_desktop/releases), you can ignore this section and follow the instructions in the [How to Use](#how-to-use) section.

### Build tools

  Compiling ClipShare Desktop needs the following tools,

* gcc (or clang on Windows)
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

  On Windows, these tools can be installed with [MSYS2](https://www.msys2.org/) using pacman with the following command:
  ```bash
  pacman -S mingw-w64-clang-x86_64-clang make vim
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
  sudo apt-get install libc6-dev libx11-dev libxmu-dev libxfixes-dev libunistring-dev libmicrohttpd-dev libssl-dev
  ```

* On Redhat-based or Fedora-based distros,
  ```bash
  sudo yum install glibc-devel libX11-devel libXmu-devel libXfixes-devel libunistring-devel libmicrohttpd-devel openssl-devel
  ```

* On Arch-based distros,
  ```bash
  sudo pacman -S libx11 libxmu libxfixes libunistring libmicrohttpd openssl
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
* [libunistring](https://packages.msys2.org/package/mingw-w64-clang-x86_64-libunistring?repo=clang64)
* [libssl](https://github.com/openssl/openssl/releases/) (provided by OpenSSL)

In an [MSYS2](https://www.msys2.org/) environment, these libraries can be installed using pacman with the following command:
```bash
pacman -S mingw-w64-clang-x86_64-libunistring
```

**Note:** However, to avoid DLL loading issues, it is recommended to compile openssl from its source with the following configuration and use that instead of the `mingw-w64-clang-x86_64-openssl` packaged version.
```bash
OPENSSL_PATH='<Use a suitable existing empty directory path here>'
CC=clang ./Configure zlib no-zlib-dynamic no-brotli no-zstd no-shared no-tests no-docs --prefix="$OPENSSL_PATH" mingw64
make
make install
```
To use it, set the following environment variables in the shell used to build ClipShare.
```bash
export CPATH="${OPENSSL_PATH}/include"
export LIBRARY_PATH="${OPENSSL_PATH}/lib64"
```

Installing libmicrohttpd from GNU is recommended. You can download the library from [ftpmirror.gnu.org/libmicrohttpd/libmicrohttpd-latest-w32-bin.zip](https://ftpmirror.gnu.org/libmicrohttpd/libmicrohttpd-latest-w32-bin.zip) and extract the files in it to the correct include and library directories in the MSYS2 environment. Alternatively, you may extract the library to a separate directory and set the `CPATH` and `LIBRARY_PATH` environment variables to the include and link-library paths, respectively.

Additionally, [PatchPE](https://github.com/datadiode/PatchPE) should be available, and the PATH environment variable should be set accordingly. Download PatchPE from [github.com/datadiode/PatchPE/releases/download/2.04/PatchPE.exe](https://github.com/datadiode/PatchPE/releases/download/2.04/PatchPE.exe).
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
