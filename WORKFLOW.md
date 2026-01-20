# Automated Changelog and Release Workflow

This document describes the automated changelog management and binary build system for the VulkanW3DViewer project.

## Overview

The workflow automates two key processes:
1. **Changelog updates** when PRs are merged to `dev` branch
2. **Binary builds and releases** when `dev` is merged to `main` branch

## Workflow Details

### 1. Changelog Update on PR Merge (`.github/workflows/changelog-update.yml`)

**Trigger**: When a PR is merged into the `dev` branch

**Process**:
1. Extracts PR information (title, number, author)
2. Determines the changelog category based on conventional commit prefixes:
   - `feat:` → **Added**
   - `fix:` → **Fixed**
   - `remove:` / `delete:` → **Removed**
   - `refactor:` / `perf:` / `style:` → **Changed**
   - Default → **Changed**
3. Adds entry to the `[Unreleased]` section of `CHANGELOG.md`
4. Commits the updated changelog back to `dev` branch

**PR Title Conventions**:
Use conventional commit format for automatic categorization:
- `feat: Add new feature` → Added section
- `fix: Resolve bug` → Fixed section
- `refactor: Improve code structure` → Changed section
- `remove: Delete obsolete code` → Removed section

### 2. Build and Release on Main Merge (`.github/workflows/release.yml`)

**Trigger**: When commits are pushed to `main` branch (typically from merging `dev`)

**Process**:

#### Stage 1: Prepare Release
1. Extracts version number from `CMakeLists.txt`
2. Generates release notes from `[Unreleased]` section in `CHANGELOG.md`

#### Stage 2: Build Binaries
Builds for multiple platforms in parallel:

**Windows Build**:
- Uses `windows-latest` runner
- Installs Vulkan SDK
- Builds with CMake release preset
- Packages: `VulkanW3DViewer-windows-v{version}.zip`
  - Contains: executable + shaders folder

**Linux Build**:
- Uses `ubuntu-latest` runner
- Installs dependencies (Vulkan, GLFW, GLM)
- Builds with CMake release preset
- Packages: `VulkanW3DViewer-linux-v{version}.tar.gz`
  - Contains: executable + shaders folder

#### Stage 3: Create GitHub Release
1. Creates GitHub release with tag `v{version}`
2. Attaches Windows and Linux binary packages
3. Uses generated release notes from changelog
4. Updates `CHANGELOG.md`:
   - Moves `[Unreleased]` entries to versioned section with date
   - Creates new empty `[Unreleased]` section
5. Commits updated changelog to `main` branch

## File Structure

```
.github/
  workflows/
    changelog-update.yml  # Auto-updates changelog on PR merge to dev
    release.yml          # Builds binaries and creates release on main
CHANGELOG.md            # Project changelog
CMakeLists.txt         # Contains project version
```

## Usage Guide

### For Contributors

When creating a PR to `dev`:
1. Use conventional commit format in PR title
2. Merge the PR
3. Changelog is automatically updated with your changes

### For Maintainers

To create a new release:
1. Update version in `CMakeLists.txt` (line 2):
   ```cmake
   project(VulkanW3DViewer VERSION X.Y.Z LANGUAGES CXX)
   ```
2. Merge `dev` branch into `main`:
   ```bash
   git checkout main
   git merge dev
   git push origin main
   ```
3. GitHub Actions automatically:
   - Builds Windows and Linux binaries
   - Creates GitHub release with binaries
   - Updates changelog with version and date
   - Commits changelog back to main

### Manual Trigger

You can also manually trigger the release workflow:
1. Go to Actions tab on GitHub
2. Select "Build and Release" workflow
3. Click "Run workflow"
4. Select `main` branch
5. Click "Run workflow"

## CHANGELOG Format

The `CHANGELOG.md` follows [Keep a Changelog](https://keepachangelog.com/) format:

```markdown
## [Unreleased]

### Added
- New features go here

### Changed
- Changes to existing functionality

### Fixed
- Bug fixes

### Removed
- Removed features

## [1.0.0] - 2026-01-20
- Previous release entries...
```

## Permissions Required

The workflows require the following GitHub permissions:
- `contents: write` - To commit changelog updates and create releases
- `pull-requests: read` - To read PR information

These are automatically granted via the `GITHUB_TOKEN` secret.

## Troubleshooting

### Changelog not updating
- Verify PR was merged (not just closed)
- Check PR title follows conventional commit format
- Review workflow logs in Actions tab

### Build failures
- Ensure CMake presets are configured correctly
- Verify all submodules are properly initialized
- Check platform-specific dependencies

### Release not created
- Confirm version was updated in `CMakeLists.txt`
- Verify push was to `main` branch
- Check CHANGELOG.md has content in Unreleased section

## Future Enhancements

Potential improvements:
- Add macOS build support
- Implement semantic version auto-increment
- Add changelog validation in PRs
- Generate detailed release notes from commit history
- Add checksum files for binaries
