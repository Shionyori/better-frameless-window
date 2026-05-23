# API 命名审查与暂定改名提案（2026-04-26）

> 说明
- 本文档只给出暂定改名方案与功能描述，不做代码修改决策。
- 目标：让接口“一眼看懂做什么”，同时避免过度改名导致维护成本增加。

## 命名评估规则
- 动词 + 对象：`setXxx` / `isXxxEnabled` / `xxxMode`。
- 优先业务语义，减少平台术语泄漏到对外 API。
- 同一层级命名风格统一：不要同时出现 `Native`、`Immersive`、`System` 混用而无说明。
- 兼容性优先：仅建议改“明显歧义”的名字。

## 你提到的重点：`immersiveDarkMode` 是什么
- 当前语义：它控制 **Windows 原生非客户区（标题栏/边框）深色属性**，底层对应 `DWMWA_USE_IMMERSIVE_DARK_MODE`。
- 它不直接控制内容区控件主题；内容区主题主要由 `setThemeMode(...)` + 样式表决定。
- 命名问题：`Immersive` 对业务使用者不直观，确实容易看不懂。

---

## A. FramelessWindow（对外主入口）

### A1. 建议改名（高优先）
| 当前名称 | 建议名称 | 功能描述 | 建议原因 |
|---|---|---|---|
| `setImmersiveDarkModeEnabled(bool)` | `setNativeDarkModeEnabled(bool)` | 开关系统标题栏/边框的深色属性 | “Immersive”不直观，`NativeDarkMode`更直白 |
| `isImmersiveDarkModeEnabled()` | `isNativeDarkModeEnabled()` | 查询系统标题栏/边框深色开关状态 | 与 setter 对齐 |

### A2. 可选改名（中优先）
| 当前名称 | 建议名称 | 功能描述 | 备注 |
|---|---|---|---|
| `setNativeBackdropPreference(...)` | `setNativeMaterialPreference(...)` | 设置原生材质偏好（Mica/Acrylic/None） | `Backdrop`是实现术语，`Material`更产品语义 |
| `nativeBackdropPreference()` | `nativeMaterialPreference()` | 获取当前原生材质偏好 | 与上保持一致 |
| `setWindowOpacityLevel(...)` | `setWindowOpacity(...)` | 设置窗口透明度 | `Level`可省略，更简洁 |
| `windowOpacityLevel()` | `windowOpacity()` | 获取窗口透明度 | 与 Qt 习惯更接近 |

### A3. 建议保持不改（已清晰）
- `setNativeEffectsEnabled(...)` / `isNativeEffectsEnabled()`
- `setRoundedCornersEnabled(...)` / `isRoundedCornersEnabled()`
- `setShadowEnabled(...)` / `isShadowEnabled()`
- `setWindowSizeLimits(...)` / `minimumWindowSize()` / `maximumWindowSize()`
- `setThemeMode(...)` / `themeMode()`
- `setAccentColor(...)` / `accentColor()`
- `setBackgroundMode(...)` / `backgroundMode()`
- `setCentralWidget(...)` / `centralWidget()` / `takeCentralWidget()`
- `addTitleBarWidget(...)` / `clearTitleBarWidgets()`
- `setDiagnosticsEnabled(...)` / `isDiagnosticsEnabled()`

---

## B. WindowEffectWin（平台效果封装）

### B1. 建议改名（高优先）
| 当前名称 | 建议名称 | 功能描述 | 建议原因 |
|---|---|---|---|
| `applyImmersiveDarkMode(...)` | `applyNativeDarkMode(...)` | 应用系统非客户区深色属性 | 与上层语义统一，避免“Immersive”术语 |
| `VisualEffectOptions::immersiveDarkModeEnabled` | `VisualEffectOptions::nativeDarkModeEnabled` | 原生深色开关参数 | 与函数名统一 |

### B2. 可选改名（中优先）
| 当前名称 | 建议名称 | 功能描述 | 备注 |
|---|---|---|---|
| `BackdropPreference` | `MaterialPreference` | 材质偏好枚举 | 若希望弱化 Win32 术语，可改 |
| `applyNativeBackdropEffects(...)` | `applyNativeMaterialEffects(...)` | 应用 Mica/Acrylic/None 材质 | 与上项配套改名 |

### B3. 建议保持不改
- `applyVisualEffects(...)`
- `applyShadow(...)`
- `applyRoundedCorners(...)`
- `applyBorderColor(...)`

---

## C. ThemeManager

### 建议保持不改（整体清晰）
- `ThemeMode`
- `setThemeMode(...)` / `themeMode()`
- `setAccentColor(...)` / `accentColor()`
- `setBackgroundMode(...)` / `backgroundMode()`
- `isDarkMode()`
- `buildStyleSheet(...)`

---

## D. TitleBar

### 建议保持不改（整体清晰）
- `setMaximized(...)`
- `addCenterWidget(...)` / `clearCenterWidgets()`
- `hitRegionAt(...)`
- `minimizeRequested()` / `maximizeRestoreRequested()` / `closeRequested()`
- `systemMoveRequested()` / `systemMenuRequested(...)`

---

## E. Core / Platform 辅助模块

### 建议改名（低优先，仅内部一致性）
| 当前名称 | 建议名称 | 备注 |
|---|---|---|
| `supportsImmersiveDarkMode`（WinUtils::WindowsCapabilities） | `supportsNativeDarkMode` | 跟对外术语统一，纯内部改动 |
| `shouldUseDarkMode(...)`（WindowVisualState） | `shouldUseNativeDarkMode(...)` | 若强调这是原生框架深色判断，可改；若表示主题语义可保留 |

---

## 暂定实施顺序（如果你确认要改）
1. 先改高优先：`ImmersiveDarkMode` -> `NativeDarkMode`（上层 + 封装层 + 文档 + 示例）。
2. 再决定是否做中优先：`Backdrop` -> `Material`。
3. 最后做内部低优先统一（`supportsImmersiveDarkMode` 等）。

## 风险提示
- 公开 API 改名会影响现有调用方；建议一次性改并同步 README 与示例。
- 如果你希望平滑迁移，可先保留旧名一段时间并标记 Deprecated（当前仅提案，不执行）。
