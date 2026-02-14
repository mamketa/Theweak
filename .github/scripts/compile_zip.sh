#!/usr/bin/env bash
# shellcheck disable=SC2035

set -euo pipefail

# Ensure running in GitHub Actions
if [[ -z "${GITHUB_WORKSPACE:-}" ]]; then
    echo "This script should only run on GitHub Action!" >&2
    exit 1
fi

cd "$GITHUB_WORKSPACE" || {
    echo "Unable to cd to GITHUB_WORKSPACE" >&2
    exit 1
}

# Required integrity files
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


# Version Info
if [[ ! -f version ]]; then
    echo "version file not found!" >&2
    exit 1
fi

version="$(< version)"
git_commit_count="$(git rev-list HEAD --count)"
git_short_hash="$(git rev-parse --short HEAD)"
version_code="$git_commit_count"
release_code="${git_commit_count}-${git_short_hash}-beta"

# Update module.prop
sed -i "s/^version=.*/version=${version} (${release_code})/" module/module.prop
sed -i "s/^versionCode=.*/versionCode=${version_code}/" module/module.prop

# Compile Gamelist
if [[ ! -f gamelist.txt ]]; then
    echo "gamelist.txt not found!" >&2
    exit 1
fi

paste -sd '|' - < "gamelist.txt" > "module/gamelist.txt"

# Prepare libs directory
rm -rf module/libs
mkdir -p module/libs

# Copy Build Outputs
copy_libs() {
    local source_dir="$1"
    local name="$2"

    if [[ -d "$source_dir/libs" ]]; then
        echo "Copying ${name} libs..."
        cp -rn "$source_dir/libs/"* module/libs/
    else
        echo "${name} libs not found!" >&2
        exit 1
    fi
}

copy_libs "daemon" "daemon"
copy_libs "profiler" "profiler"

# Copy module files
cp -r scripts/* module/system/bin/
cp -r preloader/* module/system/bin/
cp LICENSE module/

# Remove .sh extension from scripts
find module/system/bin -maxdepth 1 -type f -name "*.sh" \
    -exec bash -c 'mv -- "$1" "${1%.sh}"' _ {} \;

# Prepare zip name
zipName="Nusantara-${version}-${release_code}-beta.zip"
echo "zipName=${zipName}" >> "$GITHUB_OUTPUT"

# Generate SHA256 for integrity check
for file in "${need_integrity[@]}"; do
    if [[ -e "$file" ]]; then
        bash .github/scripts/gen_sha256sum.sh "$file"
    else
        echo "Warning: $file not found for integrity check" >&2
        exit 1
    fi
done

# Create ZIP
cd module || {
    echo "Unable to cd to ./module" >&2
    exit 1
}

zip -r9 "../${zipName}" . -x "*placeholder*" "*.map" ".shellcheckrc"

zip -z "../${zipName}" <<EOF
${version}-${release_code}
Build Date $(date +"%a %b %d %H:%M:%S %Z %Y")
EOF

echo "Build completed: ${zipName}"
