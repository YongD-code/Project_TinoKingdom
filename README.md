# Project_TinoKingdom

Unreal Engine C++ team project.

## Team Git Rules

- Work on feature branches, not directly on `main`.
- Use Pull Requests to merge into `main`.
- Sync latest `main` before starting work.
- Do not edit the same `.umap` or `.uasset` at the same time.
- Use Git LFS for Unreal binary assets.

## First Setup

```bash
git lfs install
git clone <repository-url>
```

Open `Project_TinoKingdom.uproject`. If Visual Studio files are missing, regenerate project files from Unreal or the `.uproject` context menu.

## Branch Example

```bash
git checkout main
git pull origin main
git checkout -b feature/player-movement
```

After work:

```bash
git add .
git commit -m "Add player movement"
git push -u origin feature/player-movement
```

Then open a Pull Request on GitHub.
