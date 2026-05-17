#!/bin/bash
set -e

# Set version
VERSION=${VERSION:-1.0.1}

echo "🔨 Building ADBKing DMG for macOS..."

# Build the project first
echo "📦 Building project..."
rm -rf build && mkdir build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 \
  -DCMAKE_PREFIX_PATH="/usr/local/opt/sdl2" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
make -j$(sysctl -n hw.ncpu)
cd ..

# Create DMG structure
mkdir -p dist/imgui_folder/ADBKing.app/Contents/{MacOS,Resources}

# Copy the binary from the correct location
cp build/imgui_app.app/Contents/MacOS/imgui_app dist/imgui_folder/ADBKing.app/Contents/MacOS/ADBKing
chmod +x dist/imgui_folder/ADBKing.app/Contents/MacOS/ADBKing

# Verify static linking (no external dependencies)
echo "🔍 Verifying static linking..."
otool -L dist/imgui_folder/ADBKing.app/Contents/MacOS/ADBKing | grep -v "@rpath" | grep -v "/System" | grep -v "/usr/lib" || echo "✅ No external dynamic dependencies found"

cp desktop-icons/icon-mac.icns dist/imgui_folder/ADBKing.app/Contents/Resources/

cat << EOF > dist/imgui_folder/ADBKing.app/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>ADBKing</string>
    <key>CFBundleIdentifier</key>
    <string>com.adbking.app</string>
    <key>CFBundleName</key>
    <string>ADBKing</string>
    <key>CFBundleDisplayName</key>
    <string>ADBKing</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleIconFile</key>
    <string>icon-mac.icns</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>12.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>CFBundleSupportedPlatforms</key>
    <array>
        <string>MacOSX</string>
    </array>
</dict>
</plist>
EOF

# Optional: entitlements for static builds (minimal)
cat << 'EOF' > entitlements.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
  <true/>
</dict>
</plist>
EOF

# Ad-hoc sign (minimal signing for static builds)
codesign --force --deep --options runtime --entitlements entitlements.plist --timestamp=none --sign - dist/imgui_folder/ADBKing.app

# Remove quarantine with error handling
echo "🔧 Removing quarantine attributes..."
xattr -dr com.apple.quarantine dist/imgui_folder/ADBKing.app 2>/dev/null || echo "⚠️ Could not remove quarantine attributes (this is normal in CI environment)"

# Create Applications link
ln -sf /Applications dist/imgui_folder/Applications

DMG_PATH="dist/ADBKing-${VERSION}.dmg"
[ -f "$DMG_PATH" ] && rm -f "$DMG_PATH"
hdiutil detach "/Volumes/ADBKing" || true

# Create DMG
create-dmg \
  --volname "ADBKing" \
  --window-pos 200 120 \
  --window-size 600 400 \
  --icon-size 100 \
  --icon "ADBKing.app" 150 200 \
  --icon "Applications" 350 200 \
  --hide-extension "ADBKing.app" \
  --app-drop-link 350 200 \
  "$DMG_PATH" \
  "dist/imgui_folder"

echo "✅ DMG created successfully: $DMG_PATH"
ls -lh "$DMG_PATH" 