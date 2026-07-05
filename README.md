# ClipTyper

把剪切板里的文字按“键盘逐字输入”的方式打出来，不调用网页粘贴事件。

## 当前已生成的可执行文件

```bash
make
./dist/cliptyper
```

Linux 版依赖 X11/XTest，并通过 `wl-paste`、`xclip` 或 `xsel` 读取剪切板。ASCII 字符会按普通键位输入，中文等 Unicode 字符会使用 Linux 桌面常见的 `Ctrl+Shift+u` 十六进制输入序列。

## Windows 编译

仓库里提供了 `src/cliptyper_windows.c`，使用 Windows API 的 `SendInput` 逐字输入 Unicode 文本。

在 Windows 开发者命令提示符中可用 MSVC 编译：

```bat
cl /O2 /Fe:cliptyper.exe src\cliptyper_windows.c user32.lib
```

或用 MinGW 编译：

```bat
gcc -O2 src\cliptyper_windows.c -o cliptyper.exe -luser32
```

## 使用

1. 复制要输入的纯文本。
2. 打开程序，输入 `1` 并回车。
3. 在 5 秒倒计时内点击目标输入框。
4. 完成后程序会回到操作菜单，可继续复制新文本并再次输入 `1`。需要退出时按 `Ctrl+C` 或直接关闭终端。

请只在你有权限输入内容的页面或软件中使用。
