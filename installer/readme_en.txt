ADS Explorer  -  NTFS Alternate Data Streams (ADS) viewer
=========================================================

[What is this?]
A Windows Explorer shell extension that reveals hidden
"alternate data streams (ADS)" on files.
  - Files that have ADS get a purple skull overlay icon.
  - Right-click a file and choose "Show ADS" to open a dialog
    listing each stream (name / size / content). You can also
    delete selected streams from that dialog. Ctrl/Shift-click
    selects several, Ctrl+A selects all.

[Install]
  Double-click ADSExplorer-@VERSION@.msi and follow the wizard.
  (You will be prompted to elevate to administrator.)
  The setup UI is shown in Japanese or English automatically,
  matching your Windows display language.

  You are asked to restart Windows at the end of setup.
  The overlay icons appear after that restart.

  Unattended install from a command line:
    msiexec /i ADSExplorer-@VERSION@.msi /qn
  (returns exit code 3010 = restart required)

[Uninstall]
  Settings -> Apps -> Installed apps -> "ADS Explorer" -> Uninstall.
  A restart is required here too: the DLL is in use by Explorer,
  so it is deleted on the next restart.
  Or from a command line:
    msiexec /x ADSExplorer-@VERSION@.msi /qn

[Requirements]
  - 64-bit Windows 10 / 11
  - NTFS drive
    (ADS does not exist on FAT/exFAT or network drives)

[Language]
  The UI switches automatically between Japanese and English
  based on the Windows display language.

[If the overlay icon does not appear]
  - Windows allows only 15 icon-overlay handlers system-wide.
    If OneDrive or TortoiseGit/SVN use up the slots, the skull
    may not show. The right-click "Show ADS" menu always works.
  - Changes take effect after restarting Explorer or signing out/in.

---------------------------------------------------------
(C) 2026 HDBENCH.NET    MIT License
