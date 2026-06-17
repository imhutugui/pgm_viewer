```markdown
# pgm_viewer Development Patterns

> Auto-generated skill from repository analysis

## Overview
This skill teaches you the core development patterns used in the `pgm_viewer` Python repository. You'll learn about file organization, code style, import/export conventions, and how to work with and write tests in this codebase. The repository does not use a specific framework, focusing on clean, idiomatic Python with a preference for relative imports and snake_case naming.

## Coding Conventions

### File Naming
- **Snake_case** is used for all file and module names.
  - Example: `image_loader.py`, `pgm_parser.py`

### Import Style
- **Relative imports** are preferred within the package.
  - Example:
    ```python
    from .utils import read_pgm
    ```

### Export Style
- **Named exports** are used to explicitly control what is available from a module.
  - Example:
    ```python
    __all__ = ['PGMViewer', 'load_image']
    ```

### General Code Style
- Follows standard Python idioms (PEP8).
- Functions and variables use snake_case.
- Classes use PascalCase.
- Commit messages are freeform, often starting with a title and averaging around 60 characters.

## Workflows

### Adding a New Feature
**Trigger:** When you need to implement a new capability in the viewer  
**Command:** `/add-feature`

1. Create a new Python file using snake_case if needed.
2. Implement your feature using relative imports for internal modules.
3. Add named exports in your module via `__all__`.
4. Write or update tests in a corresponding `*.test.*` file.
5. Commit your changes with a descriptive, title-style message.

### Fixing a Bug
**Trigger:** When you discover a bug in the codebase  
**Command:** `/fix-bug`

1. Locate the relevant module and identify the problematic code.
2. Apply your fix, maintaining code style and import conventions.
3. Update or add a test in the relevant `*.test.*` file to cover the bug.
4. Commit your changes with a clear, concise message describing the fix.

### Running Tests
**Trigger:** To verify your changes or before merging code  
**Command:** `/run-tests`

1. Identify all test files matching the `*.test.*` pattern.
2. Run tests using your preferred Python test runner (e.g., `pytest` or `unittest`).
3. Ensure all tests pass before pushing or merging.

## Testing Patterns

- Test files follow the pattern `*.test.*` (e.g., `image_loader.test.py`).
- The specific testing framework is not enforced; use standard Python testing tools.
- Tests are typically placed alongside the modules they test or in a dedicated test directory.
- Write clear, focused test cases for each function or class.

**Example test file:**
```python
# image_loader.test.py

from .image_loader import load_image

def test_load_image_valid():
    img = load_image('test.pgm')
    assert img.width == 100
    assert img.height == 100
```

## Commands
| Command      | Purpose                                      |
|--------------|----------------------------------------------|
| /add-feature | Scaffold and implement a new feature         |
| /fix-bug     | Locate, fix, and test a bug                 |
| /run-tests   | Run all tests in the repository              |
```