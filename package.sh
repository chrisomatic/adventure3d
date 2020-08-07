#!/bin/sh
rm -f release.7z
7z a -- release.7z adventure server.sh client.sh client_local.sh README.md fonts/ models/ shaders/ textures/
