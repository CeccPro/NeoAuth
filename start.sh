#!/bin/bash
set -e

echo "Starting NeoAuth Server..."
cd server
npm install
npm start
