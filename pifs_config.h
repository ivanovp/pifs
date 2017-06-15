/**
 * @file        pifs_config.h
 * @brief       Pi file system's configuration
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created:     2017-06-11 09:10:19
 * Last modify: 2017-06-15 14:35:45 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef _INCLUDE_PIFS_CONFIG_H_
#define _INCLUDE_PIFS_CONFIG_H_

#define PIFS_OPEN_FILE_NUM_MAX      8       /**< Maximum length of file name */
#define PIFS_FILENAME_LEN_MAX       4       /**< Maximum number of opened file */
#define PIFS_OBJECT_NUM_MAX         255     /**< Maximum number of files and directories */

#define PIFS_PACKED_ATTRIBUTE    __attribute__((packed))

#endif /* _INCLUDE_PIFS_CONFIG_H_ */
