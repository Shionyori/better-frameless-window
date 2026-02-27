#pragma once

class WindowEffectWin
{
public:
    WindowEffectWin() = default;

    void applyShadow(void *hwnd, bool enabled = true) const;
};
