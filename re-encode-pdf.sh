#!/bin/bash

gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -o $(basename $1 .pdf)-fixed.pdf $1
