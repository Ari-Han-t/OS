#!/bin/bash

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Theater Booking System - Ubuntu Setup                      ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

if [ ! -f "server.c" ]; then
    echo "✗ ERROR: server.c not found!"
    exit 1
fi

if [ ! -f "index.html" ]; then
    echo "✗ ERROR: index.html not found!"
    exit 1
fi

echo "✓ All files present"
echo ""
echo "[1/2] Compiling C server..."
gcc -o server server.c -lpthread -O2

if [ $? -ne 0 ]; then
    echo ""
    echo "✗ COMPILATION FAILED!"
    echo "Please install build-essential:"
    echo "  sudo apt-get install build-essential"
    exit 1
fi

echo "✓ Compilation successful!"
echo ""
echo "[2/2] Starting server..."
echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  SERVER IS STARTING...                                      ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "📍 Open your browser to: http://localhost:8080"
echo "⏸️  Press Ctrl+C to stop"
echo ""

./server
