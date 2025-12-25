ClipShare Desktop Client version <VERSION> with support for protocol versions 1, 2, and 3.

**Notes:**
- To use the ClipShare Client, you will need the server app running on another device. A ClipShare server program is available at [github.com/thevindu-w/clip_share_server/releases/latest](https://github.com/thevindu-w/clip_share_server/releases/latest). Refer to [github.com/thevindu-w/clip_share_server#allow-through-firewall](https://github.com/thevindu-w/clip_share_server#allow-through-firewall) for more details on using the server.
- The `clipshare-desktop.conf` file in assets is a sample. You may need to modify it. (Using the conf file is optional)
- The Windows version is tested on Windows 10 and later.
- There is a `no_ssl` variant for Windows. It is smaller in size but lacks SSL/TLS support.
- There are multiple Linux client versions included in assets in `<FILE_LINUX_AMD64>` and `<FILE_LINUX_ARM64>` archives. They are compiled for various GLIBC versions. You can select the one that is compatible with your system. If none of them are working on your system, you need to compile it from the source. The compiling procedure is described in [README.md#build-from-source](https://github.com/thevindu-w/clip_share_desktop#build-from-source).
- There are two versions for macOS included in assets in the `<FILE_MACOS>` archive. They are compiled for Intel-based Mac and Mac computers with Apple silicon. You can select the one that is compatible with your Mac computer.
  - For Mac computers with Apple silicon, use `clip-share-client-arm64`.
  - For Intel-based Mac, use `clip-share-client-x86_64`.
- Optionally, you may install `clip-share-client` to run on startup. Refer to [README.md#installation](https://github.com/thevindu-w/clip_share_desktop#installation) for more details. Installers do not need admin or superuser privileges to run.

**Changes:**
- Avoid connecting to servers for send methods if data is not available.
- Add an option to build without web mode.
- Other minor fixes.
- Code quality improvements.
