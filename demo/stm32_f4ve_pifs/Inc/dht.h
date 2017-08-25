/**
 * @file        dht.h
 * @brief       DHT11/DHT22 humidity sensor's prototypes
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created      2017-08-25 17:48:53
 * Last modify: 2017-08-25 18:10:42 ivanovp {Time-stamp}
 * Licence:     GPL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef INCLUDE_DHT_H
#define INCLUDE_DHT_H

#define DHT_ERROR    (0xFF)
#define DHT_TYPE     22

#ifdef __cplusplus
extern "C" {
#endif

bool_t DHT_init(void);
float DHT_getTempCelsius (void);
float DHT_getHumPercent (void);
bool_t DHT_read (void);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_DHT_H
