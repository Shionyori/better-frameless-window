# Contributing Guide

## Table of Contents

- [Commit Message Format](#commit-message-format)
- [Splitting Commits](#splitting-commits)
- [Branch Naming](#branch-naming)
- [PR Workflow](#pr-workflow)
- [Code Review Checklist](#code-review-checklist)
- [Examples](#examples)

---

## Commit Message Format

This project follows [Conventional Commits](https://www.conventionalcommits.org/). Every commit message must follow this format:

```
<type>: <short description>

<optional detail>

<optional issue reference — Refs #N only; put the closing Closes #N in the PR description>
```

### Types

| Type | Use | Example |
|------|-----|---------|
| `feat` | New feature | `feat: add public version header` |
| `fix` | Bug fix | `fix: move utils.cpp to cross-platform sources` |
| `test` | Add or modify tests | `test: add ThemeManager unit tests` |
| `chore` | Build, tooling, infrastructure | `chore: add test infrastructure with Qt Test` |
| `docs` | Documentation | `docs: update README with build instructions` |
| `refactor` | Refactor (no behavior change) | `refactor: extract hit-test logic` |
| `ci` | CI/CD config | `ci: add macOS to build matrix` |
| `revert` | Revert a commit | `revert: undo PR #20` |
| `style` | Formatting, code style | `style: apply clang-format` |
| `perf` | Performance improvement | `perf: cache window capabilities detection` |

### Description rules

- Use English, lowercase start
- First line ≤ 72 characters
- Imperative mood (`add`, not `added` or `adds`)
- No trailing period on the first line
- Issue keywords `Closes #14` / `Fixes #28` go in the **PR description, not the commit message** (see [PR Workflow · Issue References](#issue-references)). Use `Refs #N` in a commit if you need to mention an issue.

### Good examples

```bash
git commit -m "feat: add public version header from CMake project version"
```

```bash
git commit -m "$(cat <<'EOF'
fix: move utils.cpp to cross-platform sources

utils.cpp was only compiled on Windows, causing linker errors on Linux
and macOS. The file already uses #ifdef Q_OS_WIN guards for
platform-specific code paths.

Refs #28
EOF
)"

# Put the closing `Closes #N` in the PR description, not the commit:
gh pr create --base main --title "fix: move utils.cpp to cross-platform sources" \
  --body "## Summary
...

Closes #28"
```

### Bad examples

```bash
# Missing type
git commit -m "add version header"

# Past tense
git commit -m "feat: added version header"

# Cramming everything into one commit
git commit -m "feat: add version header, install rules, unit tests"

# Vague description
git commit -m "fix: fix stuff"
git commit -m "feat: update code"
```

---

## Splitting Commits

**One commit should do one thing.** Each commit should be:

1. **Atomic** — independently revertable without breaking the build
2. **Self-contained** — the message fully explains why the change was made
3. **Reviewable** — a reviewer can understand each step

### When to split

| Scenario | Split as |
|----------|----------|
| Tests for multiple independent modules | one `test:` commit per module |
| New feature + tests | `feat:` then `test:` (two commits) |
| Refactor + new feature | `refactor:` then `feat:` (two commits) |
| CI fix + test fix | separate `fix:` commits |
| Infrastructure + its usage | `chore:` then `feat:` / `test:` |

### Example

```bash
# ✓ Correct — 6 independent commits
chore: add test infrastructure with Qt Test and CTest
test: add version header verification test
test: add ThemeManager unit tests
test: add WindowVisualState unit tests
test: add Utils unit tests
test: add WindowHitTest unit tests

# ✗ Wrong — everything in one commit
test: add all unit tests
```

---

## Branch Naming

| Prefix | Use | Example |
|--------|-----|---------|
| `feature/` | New feature | `feature/public-version-header` |
| `fix/` | Bug fix | `fix/cross-platform-utils` |
| `chore/` | Build / tooling | `chore/update-cmake-minimum` |
| `docs/` | Documentation | `docs/api-reference` |
| `refactor/` | Refactor | `refactor/theme-manager` |
| `revert/` | Revert | `revert/undo-pr-20` |
| `release/` | Release prep | `release/v0.2.0` |
| `test/` | Tests | `test/theme-manager-coverage` |
| `ci/` | CI/CD | `ci/add-macos-build` |
| `perf/` | Performance | `perf/cache-capability-detection` |

### Rules

- Use kebab-case (lowercase + hyphens)
- Avoid numeric suffixes (`fix/foo-2`); pick a more specific descriptive name

---

## PR Workflow

### Before opening a PR

- [ ] Build passes locally (`cmake --build`) in **both Debug and Release**
- [ ] All tests pass (`ctest`), 100%; **reproduce CI conditions locally**: the same `QT_QPA_PLATFORM` (use `offscreen` headless; use the real platform for tests that touch native windows/styles) and preferably the same Qt version — version differences surface issues that only CI sees
- [ ] For platform-only issues that can't be reproduced locally (e.g. macOS SDK), verify the environment first (Qt version availability, SDK changes) before pushing — don't push blind
- [ ] No leftover debug code, `cout` / `printf`
- [ ] Commit history is clean and well-split
- [ ] `.gitignore` doesn't miss artifact dirs like `build/`, `install/`

### PR title and description

- Title follows the same `<type>: <description>` format as commits
- Description uses the template:

```markdown
## Summary
<1-3 bullets: what was done and why>

## Test plan
- [ ] Build passes
- [ ] Tests pass
- [ ] Manual verification steps

Closes #<issue>
```

### Issue References

- The only purpose of `Closes #N` / `Fixes #N` is to let GitHub auto-close the issue when the PR merges.
- **Write it only at the end of the PR description** (see template above). GitHub creates one standard close record on merge.
- **Do NOT put it in a commit message**: if that commit is later rewritten by a force-push (the SHA changes), GitHub appends duplicate "referenced this issue from a commit" records to the issue timeline, polluting its history.
- If a commit needs to mention an issue, use `Refs #N` (reference only, does not close), and only in one-time commits.

### Commit history discipline

- **Before opening the PR**: settle the commit history (count, messages, splitting), then push. Rewriting is fine at this stage.
- **After the PR is open**:
  - Prefer **adding new commits** in response to review feedback; don't rewrite history.
  - **Force-push is disabled by default; don't use it unless absolutely necessary** (e.g. pre-merge squash, fixing a serious mistake). If you must, do it **once**, then stop.
  - Repeated force-pushes cost: reviewers can't track changes, review comments lose their anchor, CI re-runs repeatedly, and — if a commit carries an issue keyword — each rewrite appends a duplicate reference to the issue timeline and permanently pollutes the main repository's history.

### PR review rules

- **Don't merge your own PR** (unless an urgent fix with explicit authorization)
- Wait for CI to be green before merging
- Merge method depends on the number of commits in the PR:

| Commits in PR | Merge method | Rationale |
|---------------|--------------|-----------|
| 1–2 | **Squash merge** | Simple change; one clean commit keeps main tidy |
| 3+ | **Merge commit** | Preserve granular history for tracing and reverting |

### PR dependency chain

When multiple PRs depend on each other, create them in order with chained bases:

```
feature/a  → main      # PR #1: base feature
feature/b  → feature/a # PR #2: depends on PR #1
feature/c  → feature/b # PR #3: depends on PR #2
```

Merge in PR number order; rebase subsequent PRs onto main after each merge.

### After the PR merges

- **Delete the remote branch promptly** after merging and confirming
- Clean up stale local branches periodically: `git remote prune origin`

```bash
git checkout main
git pull origin main
git branch -D feature/xxx          # delete local branch
git push origin --delete feature/xxx  # delete remote branch (GitHub usually does this automatically)
```

---

## Code Review Checklist

### Functionality

- [ ] Change implements the full feature / requirement described in the issue
- [ ] No unrequested "drive-by" changes
- [ ] Cross-platform compatible (Windows / Linux / macOS)
- [ ] Edge cases handled (nullptr, empty input, extreme values)

### Code quality

- [ ] No commented-out code
- [ ] No debug output (`qDebug()`, `std::cout`, `printf`)
- [ ] Clear naming, matches project style
- [ ] No unnecessary abstraction or over-engineering

### Tests

- [ ] New features have tests
- [ ] Fixed bugs have regression tests
- [ ] Tests run without a GUI (CI headless)
- [ ] Platform-specific code guarded with `#ifdef Q_OS_WIN` etc.

### CI

- [ ] CI passes on all platforms
- [ ] `ctest` 100% pass
- [ ] No new compiler warnings

---

## Examples

### Full feature PR flow

```bash
# 1. Branch from latest main
git checkout main
git pull origin main
git checkout -b feature/my-feature

# 2. Develop and commit incrementally
# Commit 1: core implementation
git add src/newfile.cpp src/newfile.h CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat: add new feature

<detailed description>
EOF
)"

# Commit 2: tests
git add tests/test_newfile.cpp CMakeLists.txt
git commit -m "test: add unit tests for new feature"

# 3. Push and open a PR
git push -u origin feature/my-feature
gh pr create --base main \
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

# 4. Wait for review and CI
# 5. Reviewer merges once approved

# 6. Cleanup
git checkout main
git pull origin main
git branch -D feature/my-feature
```
