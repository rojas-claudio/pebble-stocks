/*
 * Pebble Stocks — local server entry point.
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

/**
 * Runs the API as a long-lived server.
 *
 * server.js only exports the Express app, because the Vercel serverless
 * adapter wants a handler rather than a listening socket. This file is the
 * entry point for everything else — local development, Docker, a VPS.
 */

import app from './server.js';

const port = Number(process.env.PORT) || 3000;

if (!process.env.MONGODB_URI) {
  console.error('MONGODB_URI is not set. Copy .env.example to .env and fill it in.');
  process.exit(1);
}

app.listen(port, () => {
  console.log(`Pebble Stocks API listening on http://localhost:${port}`);
});
