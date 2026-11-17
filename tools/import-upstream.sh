#!/bin/sh
# Rebuild the first commit of the `upstream` branch from the install tape.
#
# usage: tools/import-upstream.sh [BRANCH]      (default: upstream-check)
#
# Needs reference/upstream/PL500/PL500.tap.bz2 (tools/fetch-reference.py
# PL500).  Splits the tape under reference/upstream/PL500/tape and commits
# the source trees to BRANCH without touching the working tree.  The result
# has the same tree as the first commit of `upstream`; compare with
#     git diff --stat upstream-check <first upstream commit>
# See docs/upstream.md.
set -e
top=$(cd "$(dirname "$0")/.." && pwd)
branch=${1:-upstream-check}
pl=$top/reference/upstream/PL500
tape=$pl/tape

if [ ! -f "$tape/file8" ]; then
    bunzip2 -kc "$pl/PL500.tap.bz2" > "$pl/PL500.tap"
    python3 -I "$top/tools/untap.py" "$pl/PL500.tap" "$tape"
    rm "$pl/PL500.tap"
fi

msg=$(mktemp)
trap 'rm -f "$msg"' EXIT
echo "Import 2.11BSD patch level 500, unchanged" > "$msg"

cd "$top"
python3 -I tools/tar2git.py --branch "$branch" --message-file "$msg" \
    --name "$(git config user.name)" --email "$(git config user.email)" \
    --symlink sys=usr/src/sys \
    usr/src="$tape/file7" \
    usr/src="$tape/file8" \
    usr/include="$tape/file6:include" |
    git -c core.ignorecase=false fast-import --quiet
git log --oneline -1 "$branch"
