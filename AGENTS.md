# AGENTS.md - ibus-zhuyin Development Guide

This document provides comprehensive guidelines for coding agents working on the ibus-zhuyin project, a phonetic (Zhuyin/Bopomofo) Chinese input method for IBus.

## Project Overview

ibus-zhuyin is a C-based input method engine that provides Zhuyin (Bopomofo) phonetic input for Traditional Chinese characters. It integrates with the IBus framework and uses GLib/GObject for object-oriented programming.

## Dependencies

Based on `configure.ac`, the following libraries are required:

-   `ibus-1.0` >= 1.3.0
-   `gtk+-3.0` >= 3.0.0
-   `glib-2.0`
-   `gettext` (version 0.19.8 or later recommended)

## Build System & Commands

### Bootstrap & Configuration

```bash
# Initial setup (run once after cloning)
./autogen.sh

# Configure build (development mode with debug symbols)
./configure --prefix=/usr --libexecdir=/usr/lib/ibus CFLAGS=-g CXXFLAGS=-g
```

### Build Commands

```bash
# Full build
make

# Clean build artifacts
make clean

# Install system-wide
sudo make install

# Build distribution tarball
make dist
```

### Test Commands

The project uses GLib's testing framework (`g_test`).

```bash
# Run all tests
make check

# Run specific test file
make check TESTS=test-engine

# Run tests with verbose output
make check TESTS=test-engine VERBOSE=1

# Build tests only (useful for compilation checks without running)
cd tests && make test-engine
```

### Development Workflow

```bash
# Full rebuild after code changes
make clean && make

# Quick rebuild (incremental)
make

# Test specific functionality
cd tests && ./test-engine --verbose

# Start ibus daemon for testing
ibus-daemon -r -d -x
```

## Code Style Guidelines

### File Structure & Headers

-   All C source files include GPL v3 license header with copyright.
-   Header files use `.h` extension, implementation files use `.c`.
-   Include guards use format: `#ifndef FILENAME_H` / `#define FILENAME_H` / `#endif`.
-   Files may contain Emacs or Vim modelines. Respect the existing modeline in the file.
    -   Example Emacs: `/* -*- coding: utf-8; indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*- */`
    -   Example Vim: `# vim:set et ts=4:`

### Naming Conventions

#### Functions

-   Use lowercase with underscores: `function_name()`
-   GObject methods: `ibus_zhuyin_engine_process_key_event()`
-   Static functions: `static void helper_function()`
-   Callback functions end with `_cb`: `ibus_disconnected_cb()`

#### Variables

-   Local variables: `lowercase_with_underscores`
-   Global variables: `g_variable_name` (GObject convention)
-   Struct members: `lowercase_with_underscores`
-   Constants: `UPPERCASE_WITH_UNDERSCORES`

#### Types & Structs

-   Struct names: `CamelCase` (e.g., `IBusZhuyinEngine`)
-   Typedefs: `typedef struct _StructName StructName;`
-   GObject types: `IBUS_TYPE_ZHUYIN_ENGINE`
-   Enums: `IBUS_ZHUYIN_MODE_*`

### Formatting & Indentation

-   **4 spaces** indentation (no tabs).
-   Opening braces on same line as declarations:
    ```c
    void function_name(void) {
        if (condition) {
            do_something();
        }
    }
    ```
-   Function parameters aligned:
    ```c
    static void
    function_name (Type1 param1,
                   Type2 param2,
                   Type3 param3)
    ```
-   Long lines broken after operators:
    ```c
    result = very_long_function_call(param1,
                                     param2,
                                     param3);
    ```

### Includes & Imports

-   System headers first, then GLib/GTK, then local headers.
-   Group related includes together.
-   Use angle brackets for system headers, quotes for local:
    ```c
    #include <glib.h>
    #include <gtk/gtk.h>
    #include <ibus.h>
    #include "engine.h"
    #include "zhuyin.h"
    ```

### Memory Management

-   Use GLib memory functions: `g_new()`, `g_malloc()`, `g_free()`.
-   GObject reference counting: `g_object_ref()`, `g_object_unref()`.
-   Arrays terminated with `NULL` or appropriate sentinel.
-   Check for NULL before freeing:
    ```c
    if (pointer) {
        g_free(pointer);
        pointer = NULL;
    }
    ```

### Error Handling

-   Use `GError` for error reporting:
    ```c
    GError *error = NULL;
    if (!some_operation(&error)) {
        g_warning("Operation failed: %s", error->message);
        g_error_free(error);
        return FALSE;
    }
    ```
-   Return appropriate error codes from main functions.
-   Use `g_assert*()` in tests for validation.

### GObject Patterns

-   Class structure definitions:
    ```c
    typedef struct _ClassName ClassName;
    typedef struct _ClassNameClass ClassNameClass;

    struct _ClassName {
        ParentClass parent;
        /* instance members */
    };

    struct _ClassNameClass {
        ParentClassClass parent;
        /* class methods */
    };
    ```
-   GObject registration macros and boilerplate.
-   Signal connections use `G_CALLBACK()` wrapper.

### Constants & Macros

-   Use `const` for immutable values.
-   Macros for magic numbers and repeated code.
-   Conditional compilation with `#ifdef` guards.
-   Maximum bytes constant: `G_UNICHAR_MAX_BYTES 6`

### Comments & Documentation

-   Function comments use `/** */` style with parameter descriptions.
-   Inline comments explain complex logic.
-   TODO comments for future improvements.
-   License headers on all files.

### Arrays & Data Structures

-   Static arrays use `const` where possible.
-   Dynamic arrays managed with GLib functions.
-   String arrays terminated with `NULL`.
-   Lookup tables for phonetic mappings.

### Testing Patterns

-   Test functions named `test_description()`.
-   Use `g_test_add_func()` to register tests.
-   Assertions with `g_assert_cmpstr()`, `g_assert_cmpint()`.
-   **Mocking:** IBus functions are often mocked in test files (e.g., `tests/test-engine.c`) to isolate the engine logic from the actual IBus daemon. Check `test-engine.c` for examples of `ibus_engine_commit_text` and `ibus_engine_update_preedit_text` mocks.
-   Clean up resources in test teardown.

### Internationalization

-   Use `_()` macro for translatable strings.
-   Text domain setup in main().
-   UTF-8 encoding throughout.

### Platform Considerations

-   Linux-specific (IBus framework).
-   GTK+ GUI components.
-   GLib threading and main loop integration.
-   System locale and encoding handling.

## Quality Assurance

### Pre-commit Checks

```bash
# Build verification
make clean && make

# Run test suite
make check
```

### Common Issues

-   GObject reference leaks (use valgrind).
-   Memory leaks in string handling.
-   IBus integration problems.
-   GTK event loop issues.
-   Encoding problems with Chinese characters.

## Development Tools

### Recommended Editor Configuration

```
File encoding: UTF-8
Indent style: spaces
Indent width: 4
Tab width: 4
Insert spaces: yes
Trim trailing whitespace: yes
```

### Debugging

-   Use `G_DEBUG` environment variables.
-   IBus debugging: `IBUS_DEBUG=1`
-   GTK debugging: `G_DEBUG=fatal-warnings`
-   Valgrind for memory checking.

## File Organization

```
├── src/           # Main source code
│   ├── main.c     # Program entry point
│   ├── engine.c   # IBus engine implementation
│   └── zhuyin.c   # Zhuyin phonetic logic
├── include/       # Header files
│   ├── engine.h   # GObject type declaration for the engine
│   ├── phone.h    # Core dictionary mapping phonetics to characters
│   ├── punctuation.h # Punctuation tables and definitions
│   └── zhuyin.h   # Declarations for core Zhuyin logic functions
├── tests/         # Unit tests
│   ├── test-engine.c # Main engine tests with mocks
├── po/           # Internationalization
├── icons/        # UI assets
└── m4/           # Autotools macros
```

This guide ensures consistent, maintainable code that integrates properly with the IBus ecosystem and follows GNOME/GLib best practices.