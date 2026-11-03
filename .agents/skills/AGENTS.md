# Agent Workflow & Operating Guidelines

## Core Principles

- Never write code before the user explicitly approves the implementation plan.
- Keep structural context fresh: execute workspace indexing before analysis.
- Maintain documentation parity: update `technical.md` alongside any system changes.

---

## Operating Protocol

### Phase 1: Context Synchronization

Before analyzing requests or designing solutions:

- Run `graphify` to refresh the project's folder structure and dependency map.
- Inspect the output to ensure the current workspace tree reflects the active branch state.

### Phase 2: Planning & Review Gate

Before modifying codebase files:

1. Synthesize the user prompt with the refreshed workspace structure.
2. Draft an implementation plan covering:
   - Root cause or objective summary.
   - Proposed file changes and component boundaries.
   - Potential edge cases or breaking changes.
3. Present the plan to the user and halt execution.
4. **Hard Gate:** Do not proceed to code generation without explicit user confirmation or adjustment instructions.

### Phase 3: Implementation

Upon explicit user approval:

- Hand off implementation tasks to `Ponytail` for code generation and refactoring.
- Keep modifications strictly scoped to the approved plan.

### Phase 4: Documentation & Verification

Immediately after code changes are complete:

- Review architectural shifts, new dependencies, or updated interfaces.
- Update `technical.md` within the folder where you work to reflect:
  - New module interfaces, schemas, or altered APIs.
  - Setup requirements or runtime configuration changes.
- Verify that documentation stays aligned with the latest codebase state.
