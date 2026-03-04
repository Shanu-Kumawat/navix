# Master AI Agent Instructions

1. **Self-Direction & Modularity**: Read `03_action_plan.md` frequently to understand the current priority. Maintain isolated, easily testable logic blocks.
2. **Git History Hygiene**: 
   - After completing significant logic segments or file creations, commit your work immediately with clear conventional commit messages (e.g., `feat:`, `fix:`, `refactor:`).
   - Ensure you run `git status` or `git diff` frequently before committing to ensure irrelevant scripts or artifacts are excluded.
   - Summarize commits in the conversational context to confirm milestones are met.
3. **Decisions Log**: Whenever you make an architectural choice, add an Architectural Decision Record (ADR) into `04_decisions_log.md` detailing the "Context", "Decision", and "Consequences".
4. **Compile-Driven Development**: Always run `cmake --build build` after header or major logic updates. Do not assume syntax without compiling. 
5. **Phase Tracking**: Update the checkboxes in `03_action_plan.md` as soon as a Priority is completed and committed to git.
