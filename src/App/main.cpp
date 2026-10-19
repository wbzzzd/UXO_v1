// 程序入口：创建应用、初始化、显示主窗口、进入事件循环

#include "App/Application.h"
#include <QDebug>

// === 临时调试探针（.debug-journal.md 会话 2 登记，Phase 9 必须移除）===
// 目的：捕获 SIGSEGV（栈溢出型段错误）时的调用栈，判别雷达页崩溃递归对。
// 要点：栈溢出时主栈已耗尽，handler 必须运行在 sigaltstack 备用信号栈上；
// SA_RESETHAND 使触发一次后恢复默认动作，返回即重故障 → 保留原崩溃语义。
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace {
// 备用信号栈（64KB 静态缓冲）：栈溢出场景下 handler 的立足点
stack_t g_altStack{};

void segvBacktraceHandler(int sig, siginfo_t *info, void *)
{
    char header[256];
    const int n = snprintf(header, sizeof(header),
                           "\n=== SEGV BACKTRACE (sig=%d addr=%p) ===\n",
                           sig, info ? info->si_addr : nullptr);
    if (n > 0) {
        // write 为 async-signal-safe
        (void)!write(STDERR_FILENO, header, static_cast<size_t>(n));
    }
    void *frames[64];
    const int depth = backtrace(frames, 64);
    backtrace_symbols_fd(frames, depth, STDERR_FILENO);
    // 不调用 _exit：SA_RESETHAND 已恢复默认动作，返回后重执行故障指令 → 正常转储/终止
}

// 启动时安装探针；返回 false 表示安装失败（仅告警，不影响运行）
bool installSegvProbe()
{
    // 预热 backtrace：首次调用可能触发动态加载/malloc（非信号安全），提前完成
    void *warm[8];
    (void)backtrace(warm, 8);

    g_altStack.ss_sp = malloc(64 * 1024);
    g_altStack.ss_size = 64 * 1024;
    g_altStack.ss_flags = 0;
    if (g_altStack.ss_sp == nullptr || sigaltstack(&g_altStack, nullptr) != 0) {
        return false;
    }
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = segvBacktraceHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
    return sigaction(SIGSEGV, &sa, nullptr) == 0;
}
} // namespace

int main(int argc, char *argv[])
{
    // 临时探针：最先安装，确保崩溃全程可捕获（Phase 9 移除）
    if (!installSegvProbe()) {
        qWarning() << "[debug-probe] sigaltstack/sigaction 安装失败，崩溃将无 backtrace";
    }

    Application app(argc, argv);

    // 初始化应用：配置、日志、数据库、通信、模块（当前均为占位）
    if (!app.initialize()) {
        qCritical() << "Application initialization failed";
        return -1;
    }

    qInfo() << "Application started successfully";
    // 创建并显示主窗口
    app.run();

    // 进入 Qt 事件循环
    return app.exec();
}
