# Windows Shipping presentation check

Use the project containers from the actual submission candidate. Run UnrealPak once for each project .utoc, writing a separate CSV for each. For example, in PowerShell:

```powershell
& $UnrealPak "-ListContainer=$ProjectUtoc" "-CSV=$ListingCsv" -NoLiveCoding -unattended
py -3 check_packages.py $ListingCsv
```

Pass all listing files from that build together when it has multiple chunks. The check compares full cooked asset paths on export bundles. It requires the Galaxy map and the Presentation UI style as controls, then reports 12 packages individually. These include native credits, the full waves 3–5 combat Wave, the separate tutorial MetaSound and continuous Wave, and the shared Music sound class. Exit 0 means all 12 are listed, 1 means named packages are missing, and 2 means the input or its controls could not be verified. Include the build revision and configuration with the output.

On main at 942b1d88, FTDCredits loads DA_Credits by name. Config/DefaultGame.ini has eight Presentation primary-asset scans, including TDUIStyle, but no TDCreditsData scan. Its explicit audio-directory rule covers Gameplay/Audio. The new music packages are under a different root, TriadDrift/Audio. An explicit dependency or a narrow cook rule can carry them. Runtime fallback credits can look functional even when DA_Credits is missing.

The older isolated Loading01 Windows Shipping build based on 870a7532 passed both controls and contained zero of the original eight requested packages. The submission candidate needs its own inventory and playback. In that build, enter victory from the campaign, find MUSIC and the track titles in the roll, listen to the stinger and ordered credits tracks, and return to the menu. During waves 3–5, also check that combat intensity changes the authored track and that starting a dedicated score releases the combat voice.

Historical check against the Windows package at `7ef3321`: both controls passed and 3 of 12 requested exports were present. They were the tutorial MetaSound, its continuous Wave and the Music sound class. The full waves 3–5 Wave and the eight later packages were absent. This result is tied to that older package.
