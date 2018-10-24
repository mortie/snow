#!/bin/sh

set -e

strreplace() {
	perl -pe 'BEGIN{($a,$b,@ARGV)=@ARGV}s/\Q$a/$b/g' "$@"
}

cd "$(dirname "$0")/.."

if [ "$(git diff --stat)" != "" ]; then
	echo "Repository dirty. Please commit your changes."
	exit 1;
fi

# Get prev and next version
prevtag="$(git describe --tags --abbrev=0)"
prevversion="$(echo "$prevtag" | sed 's/^v//')"
echo "Current tag: $prevtag ($(git describe --tags --dirty))"
printf "Next tag? "
read tag
version="$(echo "$tag" | sed 's/^v//')"

# Update files
strreplace -i -- \
	"git checkout $prevtag" \
	"git checkout $tag" \
	"test/Makefile"
strreplace -i -- \
	"Snow $prevversion" \
	"Snow $version" \
	"test/expected/commandline-version"
strreplace -i -- \
	"#define SNOW_VERSION \"$prevversion\"" \
	"#define SNOW_VERSION \"$version\"" \
	"snow/snow.h"
git add "test/Makefile" "test/expected/commandline-version" "snow/snow.h"
git diff
git commit -m "release $version"

# Prepare template for tag message
tmp="$(mktemp "/tmp/snow-XXXXXXXX")"
message="Snow $version
#
# Write a message for tag $tag
# Changes since $prevtag:
#
$(git log "$prevtag..HEAD" | sed 's/^/# /')
"
printf "%s" "$message" > "$tmp"
"${EDITOR:-nano}" "$tmp"

# Verify with user
echo "Message:"
cat "$tmp" | grep -v '^#'
echo
echo "Press enter to push, ^C to cancel."
read

git tag -a "$tag" -F "$tmp"
git push --tags
git push

rm "$tmp"
