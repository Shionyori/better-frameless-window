# Better Frameless Window - 进度交接文档

更新时间：2026-03-19

## 0) 本次重构增量（2026-03-19）
- 完成目录级分层落地：
  - `src/core`: `windowvisualstate.*`, `visualrefreshcoordinator.*`
  - `src/platform/win32`: `winnativemessagerouter.*`, `windowhittestwin.*`, `systemmenuwin.*`, `windowframewin.*`, `windowcommandwin.*`
- `FramelessWindow::nativeEvent` 已瘦身为单点转发到 `NativeWindowsMessageRouter`。
- 非客户区命中逻辑已下沉到 `WindowHitTestWin`，窗口类仅做 `TitleBar` 命中区域映射。
- 视觉刷新状态机已迁到 `VisualRefreshCoordinator`，窗口类仅触发 `requestRefresh`。
- 系统菜单、帧同步、DWM 强刷、最大化命令已下沉到平台层，窗口类不再直接发送 `WM_SYSCOMMAND`。
- setter 视觉更新路径已收敛到单入口 `requestVisualRefresh`，减少 `applyTheme/applyVisualEffects` 分散调用带来的时序偏差风险。
- 视觉刷新内部重复逻辑已收敛到 `performVisualRefreshPass`，并清理未使用状态字段 `m_lastTranslucentBackground`。
- 构建状态：Debug 构建通过。

## 0.1) 重构完成状态（2026-03-19）
- 架构重构范围已完成（结构拆分与责任边界已落地）。
- Release 构建已通过：`cmake --build build/windows-msvc-local-release --config Release`。
- 完成报告：`workfiles/refactor-completion-report-2026-03-19.md`。
- 后续仅剩人工回归验收项（Snap/系统菜单状态/多 DPI/主题与特效切换）。

## 1) 当前结论（可快速阅读）
- 项目已完成 **Phase 1 核心交互稳定版**。
- `F12`（最大化/还原动画样式位兼容）已完成。
- `F13`（Win11 Snap Layout 支持）已完成。
- `F14`（Win11 圆角支持）已完成。
- `F15`（Win11 深色模式支持）已完成。
- `F16`（Win11 边框颜色支持）已完成。
- `F17`（Win11 Mica 背景支持）已完成。
- `F20`（视觉特效优先级与降级策略）已完成（Mica 新旧 API + 关闭路径）。
- `F18`（Win10/11 Acrylic 背景支持）已完成（`SetWindowCompositionAttribute` 动态加载）。
- `F19`（Win7 Aero Blur，可选）已完成（`DwmEnableBlurBehindWindow` 开关接入）。
- `F21`（主题系统：浅色/深色/强调色）已完成（`ThemeManager` + 运行时主题 API）。
- `F22`（标题栏按钮状态样式规范）已完成（normal/hover/pressed/disabled 统一到主题样式）。
- `F23`（标题栏可插槽化）已完成（支持运行时注入自定义控件）。
- `F24`（背景渲染策略）已完成（纯色/渐变/图片三模式）。
- `F26`（Windows 版本与能力检测工具）已完成（能力层并入 `WinUtils`，避免过度颗粒化）。
- `F27`（调试日志与诊断开关）已完成（统一诊断模块 + 关键 Win32/DWM 失败路径日志）。
- `F28`（Demo 示例页与人工回归场景）已完成（改为外置示例，不再内置在主窗口基类中）。
- `F11`（原生阴影）已完成（DWM 低侵入接入，最大化/最小化自动关闭）。
- `F25`（运行时能力开关 API）已完成（shadow/backdrop/corner/dark）。
- `F13` 相关顽固现象（非最大化状态下 Snap 悬停不弹）已**暂缓处理**，当前基线以“稳定 + 逻辑清晰”为优先。
- 结构重构已完成：视觉特效层已从 `FramelessWindow` 剥离到 `WindowEffectWin`，职责边界更清晰。
- 结构重构（续）：命中测试与原生样式辅助逻辑已抽离到 `WinUtils`，`FramelessWindow` 进一步瘦身为流程编排层。
- 结构重构主线已落地：视觉特效层（`WindowEffectWin`）与 Win32/命中辅助层（`WinUtils`）已接入主流程并构建通过。
- 注意：2026-03-01 会话中执行过 `git checkout -- src/framelesswindow.cpp src/framelesswindow.h`，当前以已提交基线为准继续迭代。

## 1.1 今日增量（2026-03-01）
- 完成状态核对：`src/framelesswindow.cpp/.h`、`src/winutils.cpp/.h`、`CMakeLists.txt` 与文档对齐检查。
- 命名收敛已确认：辅助层统一命名为 `WinUtils`（不再使用泛化 `Utils`）。
- 完成 `FramelessWindow` 冗余视觉包装函数清理：移除未使用的 `applyRoundedCorners/applyNativeShadow/applyImmersiveDarkMode/applyBackdropEffects/applyBorderColor`，保留 `applyVisualEffects()` 统一入口。
- 完成 `nativeEvent` 结构拆分：按消息提取为 `handleNativeWindowsMessage` 与子处理函数（命中测试/最大化按钮点击/系统菜单/工作区），行为不变，可读性提升。
- 完成 `F19`：`WindowEffectWin` 新增 `Aero` 背景模式与 Win7 构建号分支（`7600~9199`）；`FramelessWindow` 新增运行时开关 `setAeroBlurEnabled/isAeroBlurEnabled`。
- 完成 `F21`：新增 `ThemeManager`，`FramelessWindow` 新增 `setThemeMode/setAccentColor` 与 `themeMode/accentColor`，样式切换与特效深色判定联动。
- 完成 `F22`：标题栏三按钮状态样式统一由主题层输出，补齐 `pressed/disabled/nativeHover` 规则，减少局部样式分散。
- 完成 `F23`：`TitleBar` 新增中心插槽容器与 `addCenterWidget/clearCenterWidgets`；`FramelessWindow` 暴露 `addTitleBarWidget/clearTitleBarWidgets` 透传 API。
- 完成 `F24`：`ThemeManager` 新增背景模式 `Solid/Gradient/Image` 与图片路径配置；`FramelessWindow` 暴露 `setBackgroundMode/setBackgroundImagePath/backgroundMode/backgroundImagePath` 运行时 API。
- 完成 `F26`：在 `WinUtils` 中新增 build 号探测 + 能力矩阵，`WindowEffectWin::selectBackdropMode` 已改为走统一能力判断，避免版本条件分散在业务逻辑。
- 完成 `F27`：新增 `Diagnostics` 模块（默认关闭），`FramelessWindow` 暴露 `setDiagnosticsEnabled/isDiagnosticsEnabled`，并在 `nativeEvent`、`WinUtils::syncNativeWindowStyles`、`WindowEffectWin` 关键失败路径输出可控告警日志。
- 完成 `F28`：`FramelessWindow` 内容区升级为 Demo 回归面板，覆盖窗口状态切换、主题/背景切换、特效开关、诊断开关与回归检查提示，便于人工回归。
- 基线调整（复用优先）：已移除 `FramelessWindow` 内置 Demo 控件，主工程仅保留“标题栏 + 内容容器”基础壳；新增 `setContentWidget()/contentWidget()` 供外部项目注入业务页面。
- 分层现状：
  - `FramelessWindow` 负责窗口流程编排与 Qt 事件连接。
  - `WinUtils` 负责 Win32 命中/样式同步等辅助逻辑。
  - `WindowEffectWin` 负责 DWM/Mica/Acrylic/边框等视觉效果实现。
- 编译验证：当前 Debug 构建通过。

## 2) 已完成功能（本次会话累计）
- 无边框窗口基础能力：
  - `WM_NCHITTEST` 命中测试
  - 8方向边缘缩放
  - 标题栏拖拽移动
  - `WM_NCCALCSIZE` 去白边
  - 最大化工作区避让（`WM_GETMINMAXINFO`）
- 标题栏能力：
  - 最小化/最大化/关闭
  - 双击最大化/还原
  - 右键系统菜单
  - 按钮状态同步（避免偶发变灰）
- 系统行为兼容：
  - `F12`：使用 `WM_SYSCOMMAND(SC_MAXIMIZE/SC_RESTORE)` 路径，提升原生动画一致性
  - `F13`：最大化按钮命中返回 `HTMAXBUTTON`，支持 Win11 Snap Layout
  - `F14`：`DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_ROUND`（非最大化/非最小化时）
  - `HTMAXBUTTON` 交互策略：
    - 悬停阶段返回 `HTMAXBUTTON`，保留 Win11 Snap Layout 弹出。
    - 点击阶段拦截 `WM_NCLBUTTONDOWN/WM_NCLBUTTONDBLCLK(HTMAXBUTTON)`，转为应用内 `toggleMaximizeRestore()`，避免系统额外控件抢占点击。
    - 最大化按钮补充 `nativeHover` 视觉态，避免仅系统命中时丢失 hover 高亮。
  - 状态残留防护：
    - 在 `WM_CAPTURECHANGED/WM_KILLFOCUS/WM_ACTIVATE(WA_INACTIVE)/WM_SIZE` 主动清理最大化按钮 `nativeHover`。
    - `TitleBar::leaveEvent` 在左键未按下时复位拖拽状态，防止中断交互后残留 pressed/drag 标志。
- 视觉特效策略：
  - 统一入口 `applyBackdropEffects()` + `selectBackdropMode()`。
  - 优先级：`MicaSystem(DWMWA_SYSTEMBACKDROP_TYPE)` → `MicaLegacy(DWMWA_MICA_EFFECT)` → `Acrylic(SetWindowCompositionAttribute)` → `Aero(DwmEnableBlurBehindWindow)` → `None`。
  - 在最大化/最小化状态主动关闭背景特效，避免异常观感与冲突。
  - 运行时开关：
    - 新增 `setShadowEnabled/setBackdropEnabled/setRoundedCornersEnabled/setImmersiveDarkModeEnabled`。
    - 新增 `setAeroBlurEnabled`（Win7 仅在支持时生效）。
    - 新增 `setThemeMode/setAccentColor`（主题实时切换）。
    - 开关变更即时生效，不依赖重启窗口。
  - 阴影策略：
    - 使用 `DwmIsCompositionEnabled + DWMWA_NCRENDERING_POLICY + DwmExtendFrameIntoClientArea`。
    - 仅在普通窗口状态启用，最大化/最小化状态关闭。
- 系统菜单优化：
  - 菜单项按窗口状态动态启用/禁用（如最大化后禁用“最大化”、启用“还原”）
  - `SC_MOVE/SC_SIZE` 走 `SendMessage` + `ReleaseCapture()`，修复“移动/大小”菜单行为

## 3) 关键回退与约束
- 阴影相关尝试（DWM + Qt DropShadow）已回退，原因：
  - 在当前环境效果不稳定或与拖拽交互冲突。
- 当前策略：
  - 保持主交互稳定优先；
  - 阴影接口保留占位（`windoweffectwin`），后续独立实现。

## 4) 当前代码状态（重要）
- 主实现文件：
  - `src/framelesswindow.cpp`
  - `src/framelesswindow.h`
  - `src/titlebar.cpp`
  - `src/titlebar.h`
- 视觉特效层（已接入构建主流程）：
  - `src/windoweffectwin.h`
  - `src/windoweffectwin.cpp`
- Win32/HitTest 辅助层（已接入构建主流程）：
  - `src/winutils.h`
  - `src/winutils.cpp`
- 本轮结构收口结果：
  - `FramelessWindow` 已完成视觉包装函数收口，特效调用路径统一到 `WindowEffectWin::applyVisualEffects`。
  - `nativeEvent` 已拆分为小函数，后续扩展 Win 消息时可单点维护。
- 计划清单：
  - `whattodo.md`

## 5) 已知风险 / 待关注点
- 暂缓项（本次明确冻结）：
  - 非最大化状态下，最大化按钮悬停触发 Snap Layout 在部分环境仍不稳定。
  - 当前已回退到单一路径命中逻辑，避免多重兜底带来的可维护性下降。
- 需要继续观察高 DPI 场景下命中判定的一致性。
- 已知视觉现象（非功能缺陷）：
  - 上/左边缘拉伸时，偶发可见对侧边缘 1 帧级“瞬跳后恢复”。
  - 当前判断为 Windows 自定义无边框窗口常见伪影（多应用可复现），非交互失效。
  - 验收口径：不影响拖拽/缩放连续性、无卡死、无错误命中、无尺寸错乱。
- 已暂缓问题（2026-03-01）：
  - 快速上下缩放时仍可观察到轻微闪烁。
  - 已尝试多种路径（消息拦截、缩放期更新控制、背景手绘切换），未获得稳定收益，部分方案在特定环境下会加重闪烁。
  - 当前已回退到“主线稳定且代码无污染”版本，后续作为独立专题排查。
- 若后续重新接入阴影，需重点验证：
  - 与拖拽/缩放是否冲突
  - 最大化状态是否异常
  - 是否影响 Snap Layout

## 6) 下次建议从这里开始
结构收口已完成，可直接进入功能主线：
1. 进入 `F29`（README/API/兼容矩阵文档）完善二次接入说明。
2. 进入 `F30`（发布前回归清单与已知问题清单）固化交付口径。
3. 基于 Demo 面板整理最小回归脚本（拖拽/缩放/最大化/菜单/Snap/特效开关）。

## 7) 快速验证清单（下次开工前 2 分钟）
- 启动程序后验证：
  - 标题栏拖拽是否稳定
  - 四边四角缩放是否稳定
  - 上/左拉伸时是否仅存在轻微瞬跳且不影响功能连续性
  - 最大化按钮悬停是否有高亮且能触发 Snap Layout
  - 最大化/最小化按钮单击是否一次生效（不出现额外弹出控件）
  - 最大化状态悬停后点击还原，hover 是否立即消失（无残留）
  - Alt+Tab 切换焦点后，最大化按钮 hover 是否被正确清理
  - 最大化后右键系统菜单是否全区域可用
  - 系统菜单中“移动/大小”是否可触发
  - Win11 最大化按钮悬停是否触发 Snap Layout

## 8) 构建说明
- 当前使用 CMake Tools 构建（Debug）通过。
- 可执行产物：
  - `build/windows-msvc-local-debug/Debug/better-frameless-window.exe`
