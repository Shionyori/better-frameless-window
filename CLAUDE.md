# CLAUDE.md

## 工作流

每个任务按以下顺序执行，不得跳过：

1. **新建分支** — 从最新 `main` 拉出，命名见[分支命名](#分支命名)
2. **写测试** — 先写失败的测试，再写实现代码（TDD）
3. **提交** — 遵循[提交规范](#提交规范)，一次只做一件事
4. **本地验证** — 构建 + 全量测试通过后才能推送
5. **推送 + 创建 PR** — 遵循[PR 规范](#pr-规范)
6. **收尾** — 见[PR 合并后](#pr-合并后)

---

## 分支命名

| 前缀 | 用途 | 示例 |
|------|------|------|
| `feature/` | 新功能 | `feature/titlebar-button-visibility` |
| `fix/` | 缺陷修复 | `fix/cross-platform-utils` |
| `docs/` | 文档 | `docs/api-reference` |
| `test/` | 测试 | `test/theme-manager-coverage` |
| `refactor/` | 重构 | `refactor/theme-manager` |
| `chore/` | 构建/工具 | `chore/update-cmake-minimum` |
| `ci/` | CI/CD | `ci/add-macos-build` |
| `perf/` | 性能优化 | `perf/cache-capability-detection` |
| `revert/` | 回退 | `revert/undo-pr-20` |
| `release/` | 发布准备 | `release/v0.2.0` |

**规则：** kebab-case（小写 + 连字符），禁止数字后缀（`fix/foo-2`），起更具体的描述性名称。

---

## 提交规范

### 格式

```
<type>: <简短描述>

<详细说明（可选）>

<关联 issue（可选）>
```

### 10 种 Type

`feat` `fix` `test` `docs` `refactor` `chore` `ci` `perf` `style` `revert`

### 规则

- 英文、小写开头、祈使语气（`add` 而非 `added`）
- 首行 ≤72 字符，末尾不加句号
- 关联 issue 使用 `Closes #N` / `Fixes #N` / `Refs #N`

### 拆分原则

一次 commit 只做一件事。原子、自包含、可独立 revert。

| 场景 | 拆分方式 |
|------|----------|
| 新增功能 + 测试 | `feat:` → `test:` 两个 commit |
| 重构 + 新功能 | `refactor:` → `feat:` 两个 commit |
| 多个独立模块的测试 | 每个模块一个 `test:` commit |
| 基础设施 + 使用它 | `chore:` → `feat:` / `test:` 分层 |

### 提交前检查

- [ ] 无注释代码（commented-out code）
- [ ] 无调试输出（`qDebug()` `std::cout` `printf`）
- [ ] `.gitignore` 未遗漏 `build/` `install/` 等产物目录
- [ ] 没有不必要的抽象或过度设计

---

## PR 规范

### 标题

与 commit 格式相同：`<type>: <描述>`

### 描述模板

```markdown
## Summary
<1-3 条要点，说明做了什么、为什么>

## Test plan
- [ ] 构建通过
- [ ] 测试通过
- [ ] 手动验证步骤

Closes #<issue>
```

### 推送与创建

```bash
git push -u origin <branch>
gh pr create --base main --title "<type>: <描述>" --body "..."
```

### 审查规则

- 不自行合并自己的 PR
- 等 CI 全绿
- 1~2 个 commit → Squash merge，3 个及以上 → Merge commit

### PR 合并后

```bash
git checkout main
git pull origin main
git branch -D <branch>                    # 删本地分支
git push origin --delete <branch>         # 删远程分支
git remote prune origin                   # 清理 stale 引用
```

---

## 代码质量底线

任何改动必须满足：

- 跨平台兼容（Windows/Linux/macOS），平台特定代码用 `#ifdef Q_OS_WIN` 守卫
- 新功能有测试，bug 修复有回归测试
- 测试可在无 GUI 环境运行（CI headless）
- 命名清晰、符合项目现有风格

---

## 技能使用

| 场景 | 触发技能 |
|------|----------|
| 新功能实现 | `brainstorming` → `writing-plans` → `subagent-driven-development` |
| 修复 bug | `systematic-debugging` |
| 任务收尾 | `finishing-a-development-branch` |
| 声称完成前 | `verification-before-completion` |
| 接收代码审查反馈 | `receiving-code-review` |

---

## 项目上下文

- **技术栈：** Qt 6 C++，跨平台 frameless window 库，Windows 为主要目标
- **构建：** `cmake --build build/windows-msvc-local-debug`
- **测试：** `ctest --test-dir build/windows-msvc-local-debug -C Debug`
- **配置：** `cmake --preset windows-msvc-local-debug`
- **详细规范：** `CONTRIBUTING.md`
