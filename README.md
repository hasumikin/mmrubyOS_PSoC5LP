## mruby machine on PSoC5LP

### mruby machine

mruby machine is an integration of [mruby/c](https://github.com/mrubyc/mrubyc), [mmrbc](https://github.com/hasumikin/mmruby), shell and peripheral driver.

### Build

#### Prerequisite

You can build this project without things below (except PSoC Creator) if you are knowledgeable about entire concept of them.

- PSoC Creator 4.3+ (compulsory)
- Windows Subsystem for Linux 2 (optional)
  - To host mmruby which contains mmrbc (mini mruby compiler)
  - You must install some essential tools such as make, git, etc.
  - I didn't try though, WSL1 should also work
- Docker Desktop for Windows (optional)
  - To build library files (.a) for PSoC
  - Make sure it is integrated with WSL
  - Find more information from Makefile and docker-compose.yml of [hasumikin/mmruby](https://github.com/hasumikin/mmruby)

#### Steps to build

- on WSL2
  - `git clone https://github.com/hasumikin/mmruby.git`
  - `cd mmruby`
  - `make psoc5lp_lib`
- on PSoC Creator
  - Open dialog from [Project] > [Build Settings...]

    - Add two file names to "Additional Link Files"
      - `\\wsl$\[path\to]\mmruby\build\psoc5lp\lib\libmrubyc.a`
      - `\\wsl$\[path\to]\mmruby\build\psoc5lp\lib\libmmrbc.a`
      - **[path\to]** should be correspond to your environment
   ![](https://raw.githubusercontent.com/hasumikin/mruby_machine_PSoC5LP/master/docs/images/additional-link-files.png)
    
    - As in, add two directory names to "Additional Include Directories"
      - `\\wsl$\[path\to]\mmruby\src`
      - `\\wsl$\[path\to]\mmruby\cli\mmirb_lib`
   ![](https://raw.githubusercontent.com/hasumikin/mruby_machine_PSoC5LP/master/docs/images/additional-include-directories.png)

    - At last, add two items to "Preprocessor Defenitions"
      - `MRBC_USE_HAL_PSOC5LP`
      - `MRBC_CONVERT_CRLF`
   ![](https://raw.githubusercontent.com/hasumikin/mruby_machine_PSoC5LP/master/docs/images/preprocessor-definitions.png)

  - Build and Program to PSoC
- on Terminal emulator like [Tera Term](https://ja.osdn.net/projects/ttssh2/)
  - Connect to PSoC over serial port

  - Serial port settings are shown in this image
  ![](https://raw.githubusercontent.com/hasumikin/mruby_machine_PSoC5LP/master/docs/images/teraterm.png)

Enjoy!

### Contributing to mmruby

You will find there are still many problems to be fixed 😅

Fork, fix, then send a pull request if you are intersted in fixing them!

