Coding
Personal Coding Guidelines

Version Control
Git Commit Rules
* Command: always use `git commit -s -S` — signs with system credentials, no AI trailer
* Never add Co-Authored-By or any AI signoff lines
* Split commits by feature or logical unit — one concern per commit
* Title format: `type(scope): short imperative description` (conventional commits)
  - Types: feat, fix, refactor, style, docs, chore
  - Example: `feat(camera): add WASD movement and mouse look`
  - Example: `feat(lighting): setup moon and torch spotlights`
* Description: one line max, plain English, says what the commit does (not why)
* Keep titles under 72 chars
Platform Usage
Claude Web
* Artifacts: Code only - no explanations in artifact
* Chat: Step-by-step tutorials, commands, explanations
* Output: Complete files for easy copy-paste
* If file too long: Show entire functions instead of partial snippets
* Skills: Use artifact builder skills when appropriate
  Cursor
* Agent mode: Create and edit files directly
* Commands: Use freely, no manual steps needed
  Version Control
  Git Operations
* Never push unless explicitly requested
* Commit messages: - brief bullet points- what changed- why it changed
  Clean Code Principles
  Code Philosophy
* Production-ready: Code should be clean, maintainable, and deployable
* Clarity over cleverness: Straightforward solutions beat complex ones
* Modular design: Break complex logic into logical, reusable components
* Single responsibility: Each function/method should do one thing well
  Self-Documenting Code
* Descriptive names: Variables and functions should explain their purpose
* Clear intent: Code structure reveals logic flow
* Avoid magic numbers: Use named constants
* Consistent patterns: Similar operations should look similar
  Function Design
* Keep functions focused and small
* Use meaningful parameter names
* Return early to reduce nesting
* Prefer composition over inheritance
  Variable Naming Patterns
  // Good - clear purpose
  userAuthToken, isValidEmail, maxRetryAttempts
  calculateTotalPrice(), fetchUserProfile()

// Bad - unclear or abbreviated
tkn, valid, max, calc(), get()
Code Organization
* Group related functionality together
* Separate concerns (business logic, UI, data access)
* Keep similar abstraction levels in same module
* Use consistent file/folder structure
  Comment Guidelines
  When to Comment
  ✅ DO comment:
* Complex algorithms: Non-obvious logic or mathematical operations
* Business decisions: Why a specific approach was chosen
* Tricky edge cases: Unusual conditions that need handling
* Performance trade-offs: When optimization requires unclear code
* Security considerations: Critical security decisions
* Non-standard patterns: Deviations from common practices
* Integration quirks: Third-party API oddities or workarounds
  ❌ DON'T comment:
* Standard loops or iterations
* Variable assignments
* Self-explanatory function calls
* Obvious conditional logic
* Standard data structure operations
* Getter/setter methods
  Comment Style
* lowercase first letter for all comments
* Keep comments concise and human
* Explain "why", not "what"
* Update comments when code changes
  Comment Examples
# Good - explains WHY
# using binary search here because dataset can be 10M+ records
result = binary_search(data, target)

# prevents race condition in high-concurrency scenarios
with lock:
update_shared_state()

# Bad - explains obvious WHAT
# loop through users
for user in users:

# increment counter
counter += 1
Code Style Specifics
Minimalism
* Avoid over-engineering: Solve current problem, not future hypotheticals
* Remove dead code: Don't comment out - delete and use version control
* No redundant checks: Trust your abstractions
* Minimal dependencies: Only add libraries when necessary
  Readability
* Consistent formatting: Use project's style guide
* Logical grouping: Related code stays together
* Whitespace matters: Separate logical blocks
* Avoid deep nesting: Extract to functions or early returns
  Error Handling
* Handle errors at appropriate level
* Use descriptive error messages
* Don't silently fail
* Log context, not just error message
  Code Output Format
  Claude Web
* Show complete files whenever possible
* If too long: show complete functions (not partial code)
* Avoid regenerating imports/boilerplate repeatedly
  Cursor
* Edit files in place
* Create new files as needed
* No manual intervention required from user
  Best Practices Checklist
  Before submitting code:
* [ ] Variable names clearly indicate purpose
* [ ] Functions do one thing well
* [ ] Complex logic has explanatory comments
* [ ] No obvious/redundant comments
* [ ] Error handling is present where needed
* [ ] Code follows consistent style
* [ ] Magic numbers replaced with named constants
* [ ] Duplication minimized through functions/modules
