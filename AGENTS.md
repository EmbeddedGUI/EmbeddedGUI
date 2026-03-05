# AGENTS.md

本仓库的 AI 协作规范以 `CLAUDE.md` 为唯一配置源（Single Source of Truth）。

## 规则

1. 后续所有 AI（含 Codex/Claude/其他代理）统一遵循 `CLAUDE.md`。
2. `AGENTS.md` 不再维护独立规则，只做入口说明，避免多处配置不一致。
3. 若 `AGENTS.md` 与 `CLAUDE.md` 出现冲突，以 `CLAUDE.md` 为准。
4. 需要调整规范时，只修改 `CLAUDE.md`。
5. 执行任务时需优先读取并使用项目内 `.claude/` 目录内容，特别是 `.claude/skills/` 下的 skills 与其中定义的 workflow。
