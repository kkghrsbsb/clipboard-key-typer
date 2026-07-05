#define UNICODE
#define _UNICODE

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

#define MIN_CHAR_DELAY_MS 80
#define MAX_CHAR_DELAY_MS 180
#define MIN_LINE_DELAY_MS 350
#define MAX_LINE_DELAY_MS 650

static int random_between(int min, int max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

static DWORD random_char_delay(void) {
    return (DWORD)random_between(MIN_CHAR_DELAY_MS, MAX_CHAR_DELAY_MS);
}

static DWORD random_line_delay(void) {
    return (DWORD)random_between(MIN_LINE_DELAY_MS, MAX_LINE_DELAY_MS);
}

static wchar_t *read_clipboard_text(void) {
    if (!OpenClipboard(NULL)) return NULL;

    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
    if (!handle) {
        CloseClipboard();
        return NULL;
    }

    wchar_t *src = (wchar_t *)GlobalLock(handle);
    if (!src) {
        CloseClipboard();
        return NULL;
    }

    size_t len = wcslen(src);
    wchar_t *copy = (wchar_t *)calloc(len + 1, sizeof(wchar_t));
    if (copy) wcscpy(copy, src);

    GlobalUnlock(handle);
    CloseClipboard();
    return copy;
}

static void print_clipboard_stats(const wchar_t *text) {
    size_t chars = wcslen(text);
    printf("剪切板读取成功：约 %zu 个字符。开始模拟输入...\n", chars);
    fflush(stdout);
}

static void send_unicode_char(wchar_t ch, DWORD delay_ms) {
    INPUT input[2];
    ZeroMemory(input, sizeof(input));

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wScan = ch;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wScan = ch;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    SendInput(1, &input[0], sizeof(INPUT));
    Sleep((DWORD)random_between(18, 42));
    SendInput(1, &input[1], sizeof(INPUT));
    Sleep(delay_ms);
}

static void send_enter(DWORD delay_ms) {
    INPUT input[2];
    ZeroMemory(input, sizeof(input));
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_RETURN;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_RETURN;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input[0], sizeof(INPUT));
    Sleep((DWORD)random_between(18, 42));
    SendInput(1, &input[1], sizeof(INPUT));
    Sleep(delay_ms);
}

static void type_text(const wchar_t *text) {
    for (size_t i = 0; text[i] != L'\0'; i++) {
        if (text[i] == L'\r') continue;
        if (text[i] == L'\n') {
            send_enter(random_line_delay());
            continue;
        }
        send_unicode_char(text[i], random_char_delay());
    }
}

static void print_banner(void) {
    SetConsoleOutputCP(CP_UTF8);
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
            Sleep(1000);
        }

        wchar_t *text = read_clipboard_text();
        if (!text || text[0] == L'\0') {
            fprintf(stderr, "读取剪切板失败，请确认剪切板里有文字。\n");
            free(text);
            continue;
        }

        print_clipboard_stats(text);
        type_text(text);
        free(text);

        puts("\n===================已完成剪切板内容输入===================");
    }
}
