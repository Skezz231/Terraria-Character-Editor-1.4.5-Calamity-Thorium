# Contributing

Thank you for your interest in contributing to `Terraria 1.4.5+ Inventory & Character Studio`.

## Before you start

- Search existing issues before opening a new one.
- Keep changes focused and minimal.
- For gameplay-related changes, describe the Terraria version and any mod context.
- For save-related changes, always consider backward compatibility and safe fallback behavior.

## Development setup

### Requirements
- `CMake 3.20+`
- `Visual Studio 2022` or another compiler with `C++20` support
- Internet connection for the first dependency fetch

### Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Contribution types

### Bug fixes
Please include:
- what is broken
- how to reproduce it
- expected result
- actual result
- affected Terraria version
- whether a modded player file is involved

### Features
Please explain:
- the use case
- the user-facing behavior
- whether it affects serialization or save compatibility
- whether UI changes are required

## Coding guidelines

- Follow the existing code style in the repository.
- Prefer small, targeted changes.
- Avoid unrelated refactoring.
- Do not introduce new dependencies unless necessary.
- Preserve existing save data where possible.
- Keep UI behavior consistent with the current `ImGui` layout.

## Pull request process

1. Fork the repository.
2. Create a feature branch.
3. Make your changes.
4. Verify the project builds successfully.
5. Update documentation if behavior changed.
6. Open a pull request with a clear summary.

## Branch naming

Suggested branch names:
- `fix/player-serialization`
- `feat/journey-research-tools`
- `docs/github-cleanup`
- `ui/storage-panel`

## Commit message examples

- `fix: handle missing recent player json safely`
- `feat: add journey research bulk unlock action`
- `docs: improve release and contribution documentation`
- `ui: refine inventory panel spacing`

## Pull request checklist

Before submitting, confirm that:
- [ ] The project builds successfully.
- [ ] The change is limited to the intended scope.
- [ ] Documentation was updated if needed.
- [ ] No generated build files were added.
- [ ] Save handling remains safe and predictable.

## Reporting security issues

Please do not open public issues for security-sensitive reports. Follow `SECURITY.md` instead.
