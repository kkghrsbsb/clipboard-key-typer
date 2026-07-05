#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MIN_CHAR_DELAY_MS 80
#define MAX_CHAR_DELAY_MS 180
#define MIN_LINE_DELAY_MS 350
#define MAX_LINE_DELAY_MS 650
#define UNICODE_STEP_DELAY_MS 45

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

static void append_data(Buffer *buf, const char *chunk, size_t n) {
    if (buf->len + n + 1 > buf->cap) {
        size_t next = buf->cap ? buf->cap * 2 : 4096;
        while (next < buf->len + n + 1) next *= 2;
        char *p = realloc(buf->data, next);
        if (!p) {
            fprintf(stderr, "内存不足\n");
            exit(1);
        }
        buf->data = p;
        buf->cap = next;
    }
    memcpy(buf->data + buf->len, chunk, n);
    buf->len += n;
    buf->data[buf->len] = '\0';
}

static char *read_command_output(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    Buffer buf = {0};
    char chunk[1024];
    while (!feof(fp)) {
        size_t n = fread(chunk, 1, sizeof(chunk), fp);
        if (n > 0) append_data(&buf, chunk, n);
    }
    int rc = pclose(fp);
    if (rc != 0 || !buf.data || buf.len == 0) {
        free(buf.data);
        return NULL;
    }
    return buf.data;
}

static char *read_clipboard(void) {
    const char *commands[] = {
        "wl-paste -n 2>/dev/null",
        "xclip -selection clipboard -o 2>/dev/null",
        "xsel --clipboard --output 2>/dev/null",
        NULL
    };

    for (int i = 0; commands[i]; i++) {
        char *text = read_command_output(commands[i]);
        if (text) return text;
    }
    return NULL;
}

static int random_between(int min, int max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

static useconds_t ms_to_us(int ms) {
    return (useconds_t)ms * 1000;
}

static useconds_t random_char_delay(void) {
    return ms_to_us(random_between(MIN_CHAR_DELAY_MS, MAX_CHAR_DELAY_MS));
}

static useconds_t random_line_delay(void) {
    return ms_to_us(random_between(MIN_LINE_DELAY_MS, MAX_LINE_DELAY_MS));
}

static size_t utf8_count_chars(const char *text) {
    size_t count = 0;
    for (size_t i = 0; text[i] != '\0'; count++) {
        unsigned char ch = (unsigned char)text[i];
        if (ch < 0x80) i += 1;
        else if ((ch & 0xE0) == 0xC0 && text[i + 1]) i += 2;
        else if ((ch & 0xF0) == 0xE0 && text[i + 1] && text[i + 2]) i += 3;
        else if ((ch & 0xF8) == 0xF0 && text[i + 1] && text[i + 2] && text[i + 3]) i += 4;
        else i += 1;
    }
    return count;
}

static void print_clipboard_stats(const char *text) {
    size_t bytes = strlen(text);
    size_t chars = utf8_count_chars(text);
    printf("剪切板读取成功：%zu 字节，约 %zu 个字符。开始模拟输入...\n", bytes, chars);
    fflush(stdout);
}

static void press_key(Display *dpy, KeyCode keycode, int use_shift, useconds_t delay_us) {
    KeyCode shift = XKeysymToKeycode(dpy, XK_Shift_L);
    if (use_shift) XTestFakeKeyEvent(dpy, shift, True, CurrentTime);
    XTestFakeKeyEvent(dpy, keycode, True, CurrentTime);
    XFlush(dpy);
    usleep(ms_to_us(random_between(18, 42)));
    XTestFakeKeyEvent(dpy, keycode, False, CurrentTime);
    if (use_shift) XTestFakeKeyEvent(dpy, shift, False, CurrentTime);
    XFlush(dpy);
    usleep(delay_us);
}

static int find_key_for_char(Display *dpy, char ch, KeyCode *keycode, int *use_shift) {
    int min_keycode, max_keycode;
    XDisplayKeycodes(dpy, &min_keycode, &max_keycode);

    int keysyms_per_keycode = 0;
    KeySym *keysyms = XGetKeyboardMapping(
        dpy,
        (KeyCode)min_keycode,
        max_keycode - min_keycode + 1,
        &keysyms_per_keycode
    );
    if (!keysyms) return 0;

    for (int kc = min_keycode; kc <= max_keycode; kc++) {
        KeySym *row = keysyms + (kc - min_keycode) * keysyms_per_keycode;
        for (int col = 0; col < keysyms_per_keycode && col < 2; col++) {
            if (row[col] == (KeySym)(unsigned char)ch) {
                *keycode = (KeyCode)kc;
                *use_shift = (col == 1);
                XFree(keysyms);
                return 1;
            }
        }
    }

    XFree(keysyms);
    return 0;
}

static unsigned int utf8_next_codepoint(const char *text, size_t *index) {
    const unsigned char *s = (const unsigned char *)text + *index;
    unsigned int cp = 0;
    size_t used = 1;

    if (s[0] < 0x80) {
        cp = s[0];
    } else if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        cp = ((unsigned int)(s[0] & 0x1F) << 6) | (unsigned int)(s[1] & 0x3F);
        used = 2;
    } else if ((s[0] & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
        cp = ((unsigned int)(s[0] & 0x0F) << 12) |
             ((unsigned int)(s[1] & 0x3F) << 6) |
             (unsigned int)(s[2] & 0x3F);
        used = 3;
    } else if ((s[0] & 0xF8) == 0xF0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) {
        cp = ((unsigned int)(s[0] & 0x07) << 18) |
             ((unsigned int)(s[1] & 0x3F) << 12) |
             ((unsigned int)(s[2] & 0x3F) << 6) |
             (unsigned int)(s[3] & 0x3F);
        used = 4;
    } else {
        cp = s[0];
    }

    *index += used;
    return cp;
}

static void type_hex_digit(Display *dpy, char digit, useconds_t delay_us) {
    KeyCode keycode = 0;
    int use_shift = 0;
    if (find_key_for_char(dpy, digit, &keycode, &use_shift) && keycode) {
        press_key(dpy, keycode, use_shift, delay_us);
    }
}

static void type_unicode_codepoint(Display *dpy, unsigned int cp, useconds_t delay_us) {
    KeyCode ctrl = XKeysymToKeycode(dpy, XK_Control_L);
    KeyCode shift = XKeysymToKeycode(dpy, XK_Shift_L);
    KeyCode u = XKeysymToKeycode(dpy, XK_u);
    KeyCode enter = XKeysymToKeycode(dpy, XK_Return);

    XTestFakeKeyEvent(dpy, ctrl, True, CurrentTime);
    XTestFakeKeyEvent(dpy, shift, True, CurrentTime);
    XTestFakeKeyEvent(dpy, u, True, CurrentTime);
    XFlush(dpy);
    usleep(ms_to_us(random_between(18, 42)));
    XTestFakeKeyEvent(dpy, u, False, CurrentTime);
    XTestFakeKeyEvent(dpy, shift, False, CurrentTime);
    XTestFakeKeyEvent(dpy, ctrl, False, CurrentTime);
    XFlush(dpy);
    usleep(ms_to_us(UNICODE_STEP_DELAY_MS));

    char hex[16];
    snprintf(hex, sizeof(hex), "%x", cp);
    for (size_t i = 0; hex[i] != '\0'; i++) {
        type_hex_digit(dpy, hex[i], ms_to_us(UNICODE_STEP_DELAY_MS));
    }

    press_key(dpy, enter, 0, delay_us);
}

static int type_text(const char *text) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "无法连接 X11 显示服务。当前 Linux 可执行版需要 X11，会话为 Wayland 时请安装 wtype/ydotool 或使用 Windows 版源码编译。\n");
        return 1;
    }

    int event_base, error_base, major, minor;
    if (!XTestQueryExtension(dpy, &event_base, &error_base, &major, &minor)) {
        fprintf(stderr, "当前 X11 不支持 XTest 扩展，无法模拟键盘输入。\n");
        XCloseDisplay(dpy);
        return 1;
    }

    for (size_t i = 0; text[i] != '\0';) {
        unsigned int cp = utf8_next_codepoint(text, &i);
        unsigned char ch = (unsigned char)cp;
        KeyCode keycode = 0;
        int use_shift = 0;

        if (cp == '\r') continue;
        if (cp == '\n') {
            keycode = XKeysymToKeycode(dpy, XK_Return);
            press_key(dpy, keycode, 0, random_line_delay());
            continue;
        }
        if (cp == '\t') {
            keycode = XKeysymToKeycode(dpy, XK_Tab);
            press_key(dpy, keycode, 0, random_char_delay());
            continue;
        }

        if (cp > 126) {
            type_unicode_codepoint(dpy, cp, random_char_delay());
            continue;
        }

        if (cp < 32) continue;

        if (!find_key_for_char(dpy, (char)ch, &keycode, &use_shift) || keycode == 0) {
            fprintf(stderr, "无法找到字符 '%c' 对应的键位，已跳过。\n", isprint(ch) ? ch : '?');
            continue;
        }
        press_key(dpy, keycode, use_shift, random_char_delay());
    }

    XCloseDisplay(dpy);
    return 0;
}

static void print_banner(void) {
    puts("====================使用注意事项以及使用说明====================");
    puts("  软件功能为将你粘贴板复制的文字内容模拟键盘打出来，不是简单的复制粘贴！！！");
    puts("  请勿用于非法用途，使用过程中与本软件作者无关");
    puts("");
    puts("                        使用流程");
    puts("  （1）将需要输入的文本（只能为文字）内容复制到剪切板,且将输入法改为英文输入法");
    puts("  （2）输入操作指令开始");
    puts("  （3）将光标放到输入框内即可");
    puts("");
    puts("=========================================================");
    puts("=========================================================");
    puts("   （1）输入指令之前确保已复制完毕且目前为英文输入法");
    puts("   （2）提前打开所需要输入的输入框，输入指令后在5秒内将光标点击到输入框即可");
    puts("");
}

int main(void) {
    srand((unsigned int)time(NULL));
    print_banner();

    char input[32];
    for (;;) {
        printf("    开始：请输入操作指令：1为开始操作，2或其它键为等待下一次操作\n        --->:");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) {
            putchar('\n');
            return 0;
        }

        if (input[0] != '1') {
            puts("===================等待下一次操作===================");
            continue;
        }

        puts("==============5秒后开始粘贴任务==========");
        for (int i = 5; i >= 1; i--) {
            printf("==================倒计时 %d 请将光标点击到输入框==================\n", i);
            fflush(stdout);
            sleep(1);
        }

        char *text = read_clipboard();
        if (!text) {
            fprintf(stderr, "读取剪切板失败。请确认已安装 wl-paste、xclip 或 xsel，并且剪切板里有文字。\n");
            continue;
        }

        print_clipboard_stats(text);
        int rc = type_text(text);
        free(text);

        if (rc == 0) puts("\n===================已完成剪切板内容输入===================");
    }
}
