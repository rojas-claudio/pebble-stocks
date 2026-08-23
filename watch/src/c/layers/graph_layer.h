/*
 * Pebble Stocks — price history graph layer.
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
#include "../stocks.h"

typedef Layer GraphLayer;

GraphLayer *graph_layer_create(GRect bounds);
void        graph_layer_set_data(GraphLayer *layer, int32_t *closes,
                                 int count, const char *timeframe);
void        graph_layer_destroy(GraphLayer *layer);
