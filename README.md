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
 * Static wear-leveling
 * User data can be added for files: permissions, owner IDs, etc.
 * At the beginning of flash memory reserved blocks can be defined, which 
are not used by the file system
 * File names are case-sensitive

Limitations:
 * Only one flash chip can be used (one volume is supported)
 * Memory and file system configuration cannot be changed during run-time
 * One directory can only store pre-defined number of files or directories
 * Partial OS support: file system can be used from multiple tasks, but
there is only one working directory for all tasks
 * Incompatible with FAT file system, therefore cannot be used for USB mass
storage devices

Compile
=======
PC demo can be compiled with 'make' and 'gcc'.
STM32 demos can be compiled with 'make' and ARM port of GCC.

Author
======
Copyright (C) Peter Ivanov &lt;ivanovp@gmail.com&gt;, 2017

