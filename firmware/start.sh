#!/bin/bash
set -e

echo "Starting NeoAuth Server..."
cd "$(dirname "$0")"
npm install
npm start
