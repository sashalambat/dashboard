# Dist

Rebuild with `Packaging/linux/make_installer.sh`.

| File | Role |
| --- | --- |
| `SBSWarsInstaller.run` | Full installer (extracts the game, runs setup, creates desktop shortcut) |
| `SBSWarsInstaller` | GUI setup binary (used by the `.run` package) |
| `SBSWarsLauncher` | Application launcher |

Run:

```bash
bash Dist/SBSWarsInstaller.run "$HOME/SBSWars"
```

After setup, double-click **SBS Wars** on the Desktop.
