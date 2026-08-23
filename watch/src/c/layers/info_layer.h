/*
 * Pebble Stocks — info bar layer: clock, market phase, list position.
 * Copyright (C) 2026 Claudio Rojas
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License, version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include <pebble.h>

typedef Layer InfoLayer;

InfoLayer *info_layer_create(GRect bounds);
void info_layer_set_data(InfoLayer *layer,
                         struct tm *time, int market_hours, int index, int total);
void info_layer_destroy(InfoLayer *layer);