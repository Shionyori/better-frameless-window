# 提交规范

## 目录

- [Commit Message 格式](#commit-message-格式)
- [拆分原则](#拆分原则)
- [分支命名](#分支命名)
- [PR 工作流](#pr-工作流)
- [代码审查清单](#代码审查清单)
- [示例](#示例)

---

## Commit Message 格式

本项目采用 [Conventional Commits](https://www.conventionalcommits.org/) 规范。每条提交信息必须遵循以下格式：

```
<type>: <简短描述>

<详细说明（可选）>

<关联 issue（可选）>
```

### Type 类型

| Type | 用途 | 示例 |
|------|------|------|
| `feat` | 新功能、新特性 | `feat: add public version header` |
| `fix` | 缺陷修复 | `fix: move utils.cpp to cross-platform sources` |
| `test` | 添加或修改测试 | `test: add ThemeManager unit tests` |
| `chore` | 构建、工具、基础设施 | `chore: add test infrastructure with Qt Test` |
| `docs` | 文档变更 | `docs: update README with build instructions` |
| `refactor` | 重构（不改变功能） | `refactor: extract hit-test logic` |
| `ci` | CI/CD 配置变更 | `ci: add macOS to build matrix` |
| `revert` | 回退提交 | `revert: undo PR #20` |
| `style` | 格式化、代码风格 | `style: apply clang-format` |
| `perf` | 性能优化 | `perf: cache window capabilities detection` |

### 描述规则

- 使用英文、小写开头
- 首行不超过 72 字符
- 使用祈使语气（`add` 而非 `added` 或 `adds`）
- 首行末尾不加句号
- 关联 issue 使用 GitHub 关键字：`Closes #14` / `Refs #6` / `Fixes #28`

### 正确示例

```bash
git commit -m "feat: add public version header from CMake project version"
```

```bash
git commit -m "$(cat <<'EOF'
fix: move utils.cpp to cross-platform sources

utils.cpp was only compiled on Windows, causing linker errors on Linux
and macOS. The file already uses #ifdef Q_OS_WIN guards for
platform-specific code paths.

Fixes #28
EOF
)"
```

### 错误示例

```bash
# 不要省略 type
git commit -m "add version header"

# 不要使用过去式
git commit -m "feat: added version header"

# 不要把所有改动塞进一个 commit
git commit -m "feat: add version header, install rules, unit tests"

# 不要使用模糊的描述
git commit -m "fix: fix stuff"
git commit -m "feat: update code"
```

---

## 拆分原则

**一次提交只做一件事。** 每个 commit 应：

1. **原子性** — 可独立 revert，不破坏构建
2. **自包含** — commit message 完整解释为什么做这个改动
3. **可审查** — reviewer 可以逐个理解每步改动

### 何时拆分

| 场景 | 拆分方式 |
|------|----------|
| 多个独立模块的测试 | 每个模块一个 `test:` commit |
| 新增功能 + 测试 | `feat:` → `test:` 两个 commit |
| 重构 + 新功能 | `refactor:` → `feat:` 两个 commit |
| CI 修复 + 测试修复 | 各自独立的 `fix:` commit |
| 基础设施 + 使用它 | `chore:` → `feat:` / `test:` 分层 |

### 示例：添加单元测试基础设施

```bash
# ✓ 正确 — 6 个独立 commit
chore: add test infrastructure with Qt Test and CTest
test: add version header verification test
test: add ThemeManager unit tests
test: add WindowVisualState unit tests
test: add Utils unit tests
test: add WindowHitTest unit tests

# ✗ 错误 — 全部塞一起
test: add unit test infrastructure with Qt Test (5 modules)
```

---

## 分支命名

| 前缀 | 用途 | 示例 |
|------|------|------|
| `feature/` | 新功能 | `feature/public-version-header` |
| `fix/` | 缺陷修复 | `fix/cross-platform-utils` |
| `chore/` | 构建/工具 | `chore/update-cmake-minimum` |
| `docs/` | 文档 | `docs/api-reference` |
| `refactor/` | 重构 | `refactor/theme-manager` |
| `revert/` | 回退 | `revert/undo-pr-20` |
| `release/` | 发布准备 | `release/v0.2.0` |

### 规则

- 使用 kebab-case（小写 + 连字符）
- 避免数字后缀（`fix/foo-2`），重新 force-push 原分支即可
- 合并后**立即删除远程分支**
- 本地 stale 分支定期清理：`git remote prune origin`

---

## PR 工作流

### 创建 PR 前

- [ ] 在本地确认构建通过（`cmake --build`）
- [ ] 运行全部测试（`ctest`），确保 100% 通过
- [ ] 检查是否有未提交的 debug 代码、`cout`/`printf`
- [ ] 确认 commit history 干净、拆分合理
- [ ] 确认 `.gitignore` 未遗漏 `build/`、`install/` 等产物目录

### PR 标题与描述

- 标题遵循与 commit 相同的 `<type>: <描述>` 格式
- 描述使用模板：

```markdown
## Summary
<1-3 条要点，说明做了什么、为什么>

## Test plan
- [ ] 构建通过
- [ ] 测试通过
- [ ] 手动验证步骤

Closes #<issue>
```

### PR 审查规则

- **不要自行合并自己的 PR**（除非紧急修复且有明确授权）
- 等 CI 全绿再合并
- 合并方式：**Merge commit**（保留 PR 内所有细粒度 commit 原样合入 main，生成 `Merge pull request #X` 记录）
- 不使用 squash merge — 本仓库重视 commit 粒度和历史追溯性，拆分良好的 commit 应该在 main 上保留而非压扁

### PR 依赖链

当多个 PR 有依赖关系时，按顺序创建 base 指向：

```
feature/a  → main          # PR #1: 基础功能
feature/b  → feature/a     # PR #2: 依赖 PR #1
feature/c  → feature/b     # PR #3: 依赖 PR #2
```

合并顺序即为 PR 编号顺序，每个合并后将后续 PR rebase 到 main。

### PR 合并后

```bash
git checkout main
git pull origin main
git branch -D feature/xxx          # 删除本地分支
git push origin --delete feature/xxx  # 删除远程分支（GitHub 通常自动完成）
```

---

## 代码审查清单

### 功能性

- [ ] 改动实现了 issue/需求描述的完整功能
- [ ] 没有引入未在需求中提及的 "顺便修改"
- [ ] 跨平台兼容（Windows / Linux / macOS）
- [ ] 边界条件处理正确（nullptr、空输入、极值）

### 代码质量

- [ ] 无注释代码（commented-out code）
- [ ] 无调试输出（`qDebug()`、`std::cout`、`printf`）
- [ ] 命名清晰、符合项目风格
- [ ] 没有不必要的抽象或过度设计

### 测试

- [ ] 新功能有对应的测试
- [ ] 修复的 bug 有回归测试
- [ ] 测试可在无 GUI 环境运行（CI headless）
- [ ] 平台特定代码使用 `#ifdef Q_OS_WIN` 等守卫

### CI

- [ ] 所有平台 CI 通过
- [ ] `ctest` 100% 通过
- [ ] 无新增编译警告

---

## 示例

### 完整的 feature PR 流程

```bash
# 1. 从最新 main 创建分支
git checkout main
git pull origin main
git checkout -b feature/my-feature

# 2. 逐步开发、逐步提交
# Commit 1: 核心实现
git add src/newfile.cpp src/newfile.h CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat: add new feature

<详细描述>
EOF
)"

# Commit 2: 测试
git add tests/test_newfile.cpp CMakeLists.txt
git commit -m "test: add unit tests for new feature"

# 3. 推送并创建 PR
git push -u origin feature/my-feature
gh pr create --repo owner/repo --base main \
  --title "feat: add new feature" \
  --body "$(cat <<'EOF'
## Summary
...

## Test plan
- [x] build + ctest pass locally
- [ ] CI pending

Closes #N
EOF
)"

# 4. 等待审查和 CI
# 5. 审查通过后由 reviewer 以 merge commit 方式合并

# 6. 清理
git checkout main
git pull origin main
git branch -D feature/my-feature
```
