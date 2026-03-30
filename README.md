# MemEnumerator
 
A lightweight Windows service memory monitor written in C. Lists all active Win32 services along with their memory usage — a focused alternative to Task Manager for diagnosing idle memory consumption.
 
## What It Does
 
- Enumerates all running services via the Windows Service Control Manager (SCM)
- Reports physical memory usage (Working Set) per service in MB
- Shows each service's internal name, display name, and PID
 
## Requirements
 
- Windows 10 or later