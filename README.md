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

程序会在输入 `1` 后、倒计时开始前读取并缓存剪切板内容，避免目标网页在获得焦点后改写剪切板。默认使用慢速输入：字符间隔约 `130ms ~ 260ms`，每次按键按下时间约 `18ms ~ 65ms`，换行后暂停约 `350ms ~ 650ms`。Windows 版会优先把 ASCII 字符作为真实虚拟键码发送，中文等字符再回退到 Unicode 输入。每次任务开始前会显示读取到的剪切板字节数/字符数，方便判断是剪切板读取问题还是目标输入框拦截问题。

请只在你有权限输入内容的页面或软件中使用。
