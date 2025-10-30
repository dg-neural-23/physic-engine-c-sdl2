#!/bin/bash
# Build script for Windows cross-compilation using Docker

echo "🐳 Building Docker image..."
docker build -t engine3d-windows-builder .

echo ""
echo "🔨 Creating Windows build..."
mkdir -p windows-build
docker run --rm -v "$(pwd)/windows-build:/build/output" engine3d-windows-builder

echo ""
echo "✅ Build complete!"
echo ""
echo "📦 Windows build files are in: ./windows-build/"
ls -lh windows-build/

echo ""
echo "🎮 To run on Windows:"
echo "   1. Copy windows-build/ folder to Windows PC"
echo "   2. Double-click engine3d.exe"
echo ""
