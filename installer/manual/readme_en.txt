ADS Explorer  -  NTFS Alternate Data Streams (ADS) viewer
                 [Manual install edition]
=========================================================

[What is this?]
A Windows Explorer shell extension that reveals hidden
"alternate data streams (ADS)" on files.
  - Files that have ADS get a purple skull overlay icon.
  - Right-click a file, choose Properties and open the "ADS"
    tab to list each stream (name / size / content). You can
    also delete selected streams there. Ctrl/Shift-click
    selects several, Ctrl+A selects all.

[How this differs from the MSI]
  This edition registers the DLL directly with regsvr32.
    - No Windows restart needed (only Explorer is restarted)
    - It does NOT appear in "Installed apps"
    - The DLL stays in the folder you extracted it to

  The MSI package (ADSExplorer-@VERSION@.msi) is recommended for
  normal use. Pick this edition if you cannot reboot, or want
  to run it temporarily from a USB stick.

[Install]
  1. Extract this zip to a folder it can stay in
     (for example C:\Tools\ADSExplorer\).
     Always run uninstall.bat before moving or deleting it.
  2. Right-click install.bat and choose "Run as administrator".
  3. When asked "Restart Explorer now", press Y.

[Uninstall]
  Right-click uninstall.bat and choose "Run as administrator".
  After that you can delete the folder.

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
    may not show. The "ADS" tab in Properties always works.
  - Changes take effect after restarting Explorer or signing out/in.

[Caution]
  Do not use this alongside the MSI edition. Registering both
  creates two registrations pointing at different paths, and
  uninstalling one leaves the other behind.

---------------------------------------------------------
(C) 2026 HDBENCH.NET    MIT License
