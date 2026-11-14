# Windows Shipping presentation check

Use the project containers from the actual submission candidate. Run UnrealPak once for each project .utoc, writing a separate CSV for each. For example, in PowerShell:

```powershell
& $UnrealPak "-ListContainer=$ProjectUtoc" "-CSV=$ListingCsv" -NoLiveCoding -unattended
py -3 check_packages.py $ListingCsv
```

Pass all listing files from that build together when it has multiple chunks. The check compares full cooked asset paths on export bundles. It requires the Galaxy map and the Presentation UI style as controls, then reports the native credits asset and the seven integrated music packages individually. Exit 0 means all eight are listed, 1 means named packages are missing, and 2 means the input or its controls could not be verified. Include the exact build revision with the output.

On main at 942b1d88, FTDCredits loads DA_Credits by name. Config/DefaultGame.ini has eight Presentation primary-asset scans, including TDUIStyle, but no TDCreditsData scan. Its explicit audio-directory rule covers Gameplay/Audio. The new music packages are under a different root, TriadDrift/Audio. An explicit dependency or a narrow cook rule can carry them. Runtime fallback credits can look functional even when DA_Credits is missing.

The older isolated Loading01 Windows Shipping build based on 870a7532 passed both controls and contained zero of these eight newer assets. This tests the inventory reader against a real container. The submission candidate still needs its own inventory and playback. In that build, enter victory from the campaign, find MUSIC and the track titles in the roll, listen to the stinger and ordered credits tracks, and return to the menu.
