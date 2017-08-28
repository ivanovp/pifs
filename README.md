pifs - Pi file system 
=====================
File system for embedded system with NOR flash media

Features:
 * Small memory footprint
 * Files can be opened for update ("r+", "w+", "a+" modes are supported)
 * Size of logical page is user-defined
 * Cache buffer for page (currently only one page is cached)
 * Directory handling
 * Dynamic wear-leveling
 * Static wear-leveling (limited, work-in-progress)
 * User data can be added for files: permissions, owner IDs, etc.
 * At the beginning of flash memory reserved blocks can be defined, which 
are not used by the file system

Limitations:
 * Only one flash chip can be used (one volume is supported)
 * Memory and file system configuration cannot be changed during run-time
 * One directory can only store pre-defined number of files or directories
 * No OS support yet, file system can be used only from one task
 * Incompatible with FAT file system, therefore cannot be used for USB mass
storage devices

Compile
=======
PC demo can be compiled with 'make' and 'gcc'.
STM32 demos can be compiled with 'make' and ARM port of GCC.

Author
======
Copyright (C) Peter Ivanov &lt;ivanovp@gmail.com&gt;, 2017
