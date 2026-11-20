#!/bin/sh
# Rebuild the import commits of the `upstream` branch from the install tape.
#
# usage: tools/import-upstream.sh [BRANCH]      (default: upstream-check)
#
# Needs reference/upstream/PL500/PL500.tap.bz2 (tools/fetch-reference.py
# PL500).  Splits the tape under reference/upstream/PL500/tape and commits
# to BRANCH, which must not exist yet, without touching the working tree:
# first the source trees, and then, on top of them, what of /usr has no
# source in the tree.  The two have the same trees as the first two
# commits of `upstream`; compare with
#     git diff --stat upstream-check~1 <first upstream commit>
#     git diff --stat upstream-check <second upstream commit>
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

cd "$top"
import() {
    python3 -I tools/tar2git.py --branch "$branch" --message-file "$msg" \
        --name "$(git config user.name)" --email "$(git config user.email)" \
        --symlink sys=usr/src/sys "$@" \
        usr/src="$tape/file7" \
        usr/src="$tape/file8" \
        usr/include="$tape/file6:include" |
        git -c core.ignorecase=false fast-import --quiet
}

echo "Import 2.11BSD patch level 500, unchanged" > "$msg"
import

# What of /usr, file 6, has no source in the tree.  Not the PDP-11's
# programs and libraries, nor what is made from usr/src: the manual's
# formatted pages, the games' files, /usr/share.  Of /usr/dict, not
# spell's hashed lists, which spellin makes, nor spellhist, a link to
# /dev/null; not refer's indexes, Ind.*, which its indxbib, or runinv,
# makes of the references beside them; of /usr/adm, only cron's scripts,
# not the logs.
echo "Import from 2.11BSD's /usr what has no source in usr/src" > "$msg"
import --parent "refs/heads/$branch^0" \
    --exclude usr/dict/hlista --exclude usr/dict/hlistb \
    --exclude usr/dict/hstop --exclude usr/dict/spellhist \
    --exclude usr/dict/papers/Ind.ia --exclude usr/dict/papers/Ind.ib \
    --exclude usr/dict/papers/Ind.ic --exclude usr/doc/run/Ind.ia \
    --exclude usr/doc/run/Ind.ib --exclude usr/doc/run/Ind.ic \
    --exclude usr/ingres/bin --exclude usr/ingres/lib \
    usr/dict="$tape/file6:dict" \
    usr/doc="$tape/file6:doc" \
    usr/ingres="$tape/file6:ingres" \
    usr/pub="$tape/file6:pub" \
    usr/adm/daily="$tape/file6:adm/daily" \
    usr/adm/weekly="$tape/file6:adm/weekly" \
    usr/adm/monthly="$tape/file6:adm/monthly"
git log --oneline -2 "$branch"
