# Local Development Setup

This project uses environment-specific CMake configurations to handle different build environments.

## How it works

- `CMakeLists.txt` - Main configuration file (committed to git, used by CI/CD)
- `CMakeLists.local.txt` - Local development overrides (not committed, for your local PC)

## For Local Development

The `CMakeLists.local.txt` file contains Windows-specific changes that make the project work on your local PC but aren't needed for CI/CD. This file is automatically included if it exists.

### What's in the local override:

- Uses vcpkg's CONFIG mode for SDL2 detection (works better on local Windows setups)
- Simplified SDL2 linking with proper targets
- Optimized for local development workflow

## For CI/CD

The main `CMakeLists.txt` file is used by GitHub Actions and contains:
- Manual SDL2 path detection for Windows
- More robust fallback mechanisms
- Compatible with CI/CD environments

## Usage

### Local Development
```bash
# Your local changes are automatically applied
cmake -B build
cmake --build build
```

### CI/CD (GitHub Actions)
```bash
# Uses the main CMakeLists.txt without local overrides
cmake -B build
cmake --build build
```

## File Status

- ✅ `CMakeLists.txt` - Committed to git, used by CI/CD
- ✅ `CMakeLists.local.txt` - Local only, ignored by git
- ✅ `.gitignore` - Updated to ignore local overrides

## Troubleshooting

If you need to modify local settings:
1. Edit `CMakeLists.local.txt`
2. The file is ignored by git, so changes stay local
3. CI/CD will use the main `CMakeLists.txt`

If you need to test CI/CD settings locally:
1. Temporarily rename `CMakeLists.local.txt` to `CMakeLists.local.txt.bak`
2. Build will use main `CMakeLists.txt` settings
3. Rename back when done testing 