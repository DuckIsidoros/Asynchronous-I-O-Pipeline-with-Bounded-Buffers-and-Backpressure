---
name: graphify
description: Guides the agent to inspect the Java codebase using Graphify knowledge graphs instead of brute-force search.
triggers:
  - when navigating class relationships
  - when inspecting dependencies or call graphs
  - before refactoring or adding new features
---

# Graphify Code Intelligence Skill

## Purpose

Use pre-indexed structural graphs to understand class hierarchies, interface implementations, and call relationships in this Java project without token-heavy grepping.

## Guidelines

1. **Check for Index First:** Verify whether `graphify-out/` or `graph.json` exists in the project root.
2. **Re-index if Stale:** If source files in `src/` were modified and the index is missing or out of date, update the graph:
   ```bash
   npx graphify index .
   ```
