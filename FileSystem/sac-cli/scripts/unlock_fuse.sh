#!/bin/bash
cd "$(dirname "$0")"
cd ../bin
fusermount -uz ./tmp
