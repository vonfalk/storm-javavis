@echo off
if not exist %2 (
   mkdir %2
)

copy %1 %2\%3