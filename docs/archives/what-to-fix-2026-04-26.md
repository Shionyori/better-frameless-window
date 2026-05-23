以下是对目前已知问题的分析和修复方案总结，供你参考，你可以按具体情况选择是否采纳。

## 🔧 问题一：`applySystemBackdrop` 切换失效

### ❌ 根因
- **静态变量跨实例污染**：`static bool xxxWasEnabled` 被所有窗口共享，导致状态互相覆盖
- **None 模式未显式清除**：仅当 `WasEnabled==true` 时才执行清除，初始为 None 时永不清除

### ✅ 修复要点
```cpp
// 1. 移除所有静态状态变量 → 改为"每次调用独立应用"
// 2. 先清除所有旧效果（SystemBackdrop/Legacy Mica/Acrylic），再按需应用新效果
// 3. mode==None 时直接返回（已清除完毕），确保能正确关闭效果
// 4. 降级逻辑改为"尝试→失败→降级"的线性流程，避免状态机互相干扰
```

### 📦 配套修复
- `WindowVisualState::buildVisualEffectOptions`：**直接透传** `backdropMode`，不做短路/缓存/降级判断

---

## 🎨 问题二：Backdrop / Opacity / BackgroundMode 冲突

### ❌ 根因
Windows DWM **不支持** `SystemBackdrop` 与 `WindowOpacity` 同时生效，三者无序叠加会导致：
- 双重混合 → 黑边/闪烁/性能下降
- Backdrop 被强制降级为纯色
- 高 DPI 下采样错位

### ✅ 设计原则（优先级自上而下）
```
1. SystemBackdrop (最高) → 非 None 时，强制 BackgroundMode=Transparent + Opacity=1.0
2. BackgroundMode        → 决定应用层是否绘制底色
3. WindowOpacity (最低)  → 仅当 BackgroundMode==Solid 且 Backdrop==None 时生效
```

### 📦 API 改造要点
```cpp
// 1. 新增 BackgroundMode 枚举
enum class BackgroundMode { Auto, Solid, Transparent };

// 2. setWindowOpacity 增加合法性校验 + 返回值
bool setWindowOpacity(qreal opacity);  // 返回 false 表示当前模式不支持
bool isOpacitySupported() const;       // 查询是否允许调节透明度

// 3. 内部状态自动同步（防调用方手动维护）
void setSystemBackdrop(SystemBackdrop type) {
    if (type != None) {
        m_backgroundMode = Transparent;  // 自动切换
        m_opacity = 1.0;                 // 自动锁定
    }
}
```