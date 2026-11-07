# AGENTS.md — Master Execution Protocol & Operating Guidelines

## Core Principles

- **No Code Without Plan Approval:** Never modify or write codebase files before the user explicitly approves the implementation plan.
- **Surgical Precision & Zero Scope Creep:** Solve only the requested task. Do NOT add unsolicited abstractions, helper wrappers, or refactor working code.
- **Strict Blast Radius Containment:** Changes must be entirely isolated; never touch adjacent modules, shared globals, or unrelated configurations.
- **Fresh Context First:** Run workspace indexing before any code analysis or planning.
- **Documentation & Invariant Parity:** Keep technical documentation strictly synchronized with codebase realities.

---

## Operating Protocol

### Phase 1: Context Synchronization
Before analyzing requests or designing solutions:
1. Run `graphify` to refresh the project's folder structure, dependency graph, and symbol index.
2. Inspect the output to verify the current workspace tree reflects the active branch state.

---

### Phase 2: Planning & Double-Check Gate (Hard Stop)
Before proposing or executing code modifications, draft and output a structured plan containing:

1. **Target Scope & Blast Radius:**
   - Exact files, structs, or functions to be modified/created.
   - Confirmation that adjacent files and caller contracts remain untouched.

2. **Pre-Flight Double Checks (Mandatory Checklist):**
   - **Boundary Conditions:** Array bounds, queue capacity invariants ($0 \le \text{count} \le N$), buffer index wrapping, integer overflows.
   - **Concurrency Hazards:** Lost wakeups, spurious wakeups (must wrap in `while` loop), lock ordering (deadlock prevention), lock contention during I/O.
   - **POSIX Edge Cases:** Short reads/writes looping, `EINTR` signal retries, clean `EOF` handling, `EAGAIN` handling.
   - **Resource Lifecycle:** Single-ownership transfer semantics, zero memory/file-descriptor leaks, clean freelist returns on cancellation.

3. **Verification Strategy:**
   - Exact build commands and automated test targets to prove correctness.

> **CRITICAL HARD GATE:** Halt immediately after outputting this plan. Do NOT generate code, invoke code generation sub-agents, or apply diffs until the user provides explicit written confirmation.

---

### Phase 3: Implementation & Constraints
Upon receiving explicit user approval:
- Hand off implementation tasks to `Ponytail` for code generation.
- **Coding Constraints:**
  - **Minimal Diff:** Edit only the target scope. Do not reformat adjacent lines or reorder unrelated imports.
  - **Single-Line Block Comments:** Every code block must have at most one concise line of comment explaining *why*, not *what*.
  - **No Unsolicited Logic:** Zero speculative features, zero defensive over-engineering beyond POSIX/concurrency invariants.

---

### Phase 4: Verification & Circuit Breaker
Immediately after applying code changes:

1. **Automated Verification:**
   - Execute the project build script/Makefile with strict flags (`-Wall -Wextra -Werror -pedantic -pthread -fsanitize=address,undefined`).
   - Run relevant unit tests, stress benchmarks, or fault injection harnesses.

2. **Failure Recovery (Circuit Breaker):**
   - If a build failure, segmentation fault, or test regression occurs, do NOT attempt blind multi-file hotfixes.
   - Revert modified files (`git restore <file>`) if errors cascade outside the planned scope.
   - Report the exact failure diagnostic back to the user and re-enter Phase 2.

---

### Phase 5: Documentation Parity
Once verification passes cleanly:
- Update `technical.md` within the working directory to document:
  - Added/modified interfaces, data structs, or API behaviors.
  - Updated synchronization invariants, boundary conditions, or environment prerequisites.
- Ensure all technical documentation accurately mirrors the current codebase state.