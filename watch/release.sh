#!/usr/bin/env bash
#
# Builds a release .pbw against the real API endpoint without leaving the
# modified source behind.
#
# src/js/api.js ships a placeholder endpoint (see the README), so a release
# build has to rewrite it. That rewrite must never survive the build, or it
# ends up committed. The restore below runs on success, failure and Ctrl-C
# alike.
#
# Endpoint comes from $STOCKS_API_BASE, else watch/.release-endpoint
# (gitignored). If neither is set this refuses to build, so a release can
# never silently ship the placeholder.
#
#     STOCKS_API_BASE=https://api.example.com ./release.sh
#
set -euo pipefail
cd "$(dirname "$0")"

API=src/js/api.js
PLACEHOLDER="http://127.0.0.1:3000"

endpoint="${STOCKS_API_BASE:-}"
if [ -z "$endpoint" ] && [ -f .release-endpoint ]; then
  endpoint="$(tr -d '[:space:]' < .release-endpoint)"
fi
if [ -z "$endpoint" ]; then
  echo "error: no endpoint configured." >&2
  echo "  set STOCKS_API_BASE=https://... or write it to watch/.release-endpoint" >&2
  exit 1
fi
case "$endpoint" in
  http://*|https://*) ;;
  *) echo "error: endpoint must start with http:// or https:// (got: $endpoint)" >&2; exit 1 ;;
esac
if [ "$endpoint" = "$PLACEHOLDER" ]; then
  echo "error: that is the placeholder, not a real endpoint" >&2; exit 1
fi

# Restore on ANY exit path — success, build failure, or interrupt.
backup="$(mktemp)"
cp "$API" "$backup"
restore() {
  trap - EXIT                     # disarm: never restore twice
  [ -f "$backup" ] && cp "$backup" "$API"
  rm -f "$backup" "$API.bak"
}
# Only EXIT restores. INT/TERM exit instead, which triggers EXIT once --
# trapping them directly would run the handler and then resume the script.
trap restore EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

# -i.bak rather than -i '' so this works with both BSD and GNU sed
sed -i.bak "s|^var BASE_URL = .*|var BASE_URL = '${endpoint}';|" "$API"
rm -f "$API.bak"
if ! grep -q "var BASE_URL = '${endpoint}';" "$API"; then
  echo "error: substitution did not apply — has the BASE_URL line changed shape?" >&2
  exit 1
fi

pebble build

# The bundle is what actually ships, so verify that rather than the source.
if ! grep -q -- "$endpoint" build/pebble-js-app.js; then
  echo "error: endpoint missing from build/pebble-js-app.js" >&2
  exit 1
fi
if grep -q -- "$PLACEHOLDER" build/pebble-js-app.js; then
  echo "error: placeholder still present in the bundle" >&2
  exit 1
fi

echo
echo "release build OK"
echo "  endpoint: $endpoint"
echo "  bundle:   watch/build/watch.pbw"
