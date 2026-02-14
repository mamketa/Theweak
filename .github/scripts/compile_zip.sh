#!/bin/env bash
# shellcheck disable=SC2035

set -euo pipefail

if [ -z "${GITHUB_WORKSPACE:-}" ]; then
    echo "This script should only run on GitHub action!" >&2
    exit 1
fi

cd "$GITHUB_WORKSPACE" || {
    echo "Unable to cd to GITHUB_WORKSPACE" >&2
    exit 1
}

# CRITICAL FILES
need_integrity=(
    "module/velocity.banner.avif"
    "module/velocitytoast.apk"
    "module/system/bin"
    "module/libs"
    "module/META-INF"
    "module/service.sh"
    "module/uninstall.sh"
    "module/action.sh"
    "module/module.prop"
    "module/gamelist.txt"
)


# VERSION INFO
version="$(cat version)"
version_code="$(git rev-list HEAD --count)"
release_code="$(git rev-list HEAD --count)-$(git rev-parse --short HEAD)-beta"
sed -i "s/version=.*/version=$version ($release_code)/" module/module.prop
sed -i "s/versionCode=.*/versionCode=$version_code/" module/module.prop

# COMPILE GAMELIST
paste -sd '|' - <"$GITHUB_WORKSPACE/gamelist.txt" \
    >"$GITHUB_WORKSPACE/module/gamelist.txt"

# PREPARE STRUCTURE
mkdir -p module/libs/armeabi-v7a
mkdir -p module/libs/arm64-v8a
mkdir -p module/system/bin

# COPY DAEMON BINARIES
if [ -d "daemon/libs" ]; then
    [ -d "daemon/libs/armeabi-v7a" ] && \
        cp -rf daemon/libs/armeabi-v7a/* module/libs/armeabi-v7a/ || true

    [ -d "daemon/libs/arm64-v8a" ] && \
        cp -rf daemon/libs/arm64-v8a/* module/libs/arm64-v8a/ || true
fi

# COPY PROFILER BINARIES
if [ -d "profiler/libs" ]; then
    [ -d "profiler/libs/armeabi-v7a" ] && \
        cp -rf profiler/libs/armeabi-v7a/* module/libs/armeabi-v7a/ || true

    [ -d "profiler/libs/arm64-v8a" ] && \
        cp -rf profiler/libs/arm64-v8a/* module/libs/arm64-v8a/ || true
fi

# COPY NUSANTARA_UTILITY.SH
if [ -f "scripts/nusantara_utility.sh" ]; then
    cp scripts/nusantara_utility.sh module/system/bin/
fi

# COPY PRELOADER
if [ -d "./preloader" ]; then
    cp -rf ./preloader/* module/system/bin/
fi


# REMOVE .SH EXTENSION
find module/system/bin -maxdepth 1 -type f -name "*.sh" \
    -exec sh -c 'mv -- "$1" "${1%.sh}"' _ {} \;

# LICENSE
cp LICENSE ./module

# ZIP BUILD
zipName="Nusantara-$version-$release_code.zip"
echo "zipName=$zipName" >>"$GITHUB_OUTPUT"

for file in "${need_integrity[@]}"; do
    bash .github/scripts/gen_sha256sum.sh "$file"
done

cd ./module || {
    echo "Unable to cd to ./module" >&2
    exit 1
}

zip -r9 "../$zipName" . \
    -x "*placeholder*" "*.map" ".shellcheckrc"

zip -z "../$zipName" <<EOF
$version-$release_code
Build Date $(date +"%a %b %d %H:%M:%S %Z %Y")
EOF

echo "Build completed: $zipName"
