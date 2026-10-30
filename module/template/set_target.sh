#!/system/bin/sh
# Usage (as root): sh set_target.sh com.example.game [more.packages ...]
# Rewrites the package= lines in config.txt. Restart the game afterwards.
CFG=/data/adb/modules/zygisk_il2cpp_dumper/config.txt
[ -z "$1" ] && { echo "usage: $0 <package> [package...]"; exit 1; }
grep -v '^package=' "$CFG" > "$CFG.tmp"
for p in "$@"; do echo "package=$p" >> "$CFG.tmp"; done
mv "$CFG.tmp" "$CFG"
chmod 644 "$CFG"
echo "Targets set:"; grep '^package=' "$CFG"
