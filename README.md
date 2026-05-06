# Linux Lockdep & Mini-htop Project

## Overview
This project is part of the **Advanced Operating Systems** course at Cairo University.  
It demonstrates **deadlock detection in the Linux kernel** and **system monitoring in user space**.

The project is divided into three main parts:

1. **Lockdep Deadlock Demonstration (Kernel Module)**
2. **Mini-Lockdep Implementation (Kernel Module)**
3. **Mini-htop System Monitoring Tool (User Space)**

All components were developed and tested using a **Linux Virtual Machine environment**.

---

## Objectives

- Understand **kernel-level synchronization and deadlocks**
- Explore the Linux kernel’s **Lockdep mechanism**
- Implement a simplified **dependency graph for lock tracking**
- Build a **real-time system monitoring tool** using `/proc`

---

## Project Structure
.
├── part1_deadlock/
│   └── deadlock_module.c
│
├── part2_mini_lockdep/
│   └── mini_lockdep.c
│
├── part3_mini_htop/
│   └── mini_htop.c
│  
├── images/
│   └── dmesg-output.png
│   └── mini-htop.png
│
├── Makefile
└── README.md

---

## ⚙️ Environment & Tools

- **OS:** Linux (Ubuntu VM recommended)
- **Kernel Development:** Linux Kernel Modules (LKM)
- **Language:** C
- **Tools Used:**
  - `gcc`
  - `make`
  - `dmesg`
  - `/proc` filesystem
- **Platform:** Virtual Machine (VirtualBox / VMware)

---

# Part 1: Deadlock Demonstration using Lockdep

## Description
A Linux kernel module that intentionally creates a **deadlock scenario** using spinlocks.

## Key Idea
Two threads acquire locks in **reverse order**, leading to:

Thread 1: lock A → lock B

Thread 2: lock B → lock A

This creates a **circular wait condition → DEADLOCK**

## What Happens
- Lockdep detects incorrect lock ordering
- Kernel logs (`dmesg`) show warnings

## ▶️ How to Run

```bash
make
sudo insmod deadlock_module.ko
dmesg | tail
sudo rmmod deadlock_module
```
# Part 2: Mini-Lockdep (Custom Implementation)
## Description

A simplified version of Linux Lockdep, implemented from scratch.

## Features
Tracks locks per thread
Builds a dependency graph
Detects deadlocks using DFS cycle detection

## Core Concepts

### Dependency Graph

Each lock is a node:
Lock A → Lock B

### Cycle Detection

If a cycle exists:
A → B → C → A

## ▶️ How to Run

```bash
make
sudo insmod mini_lockdep.ko
dmesg | tail
sudo rmmod mini_lockdep
```

# Part 3: Mini-htop (User-Space Monitoring Tool)
## Description

A lightweight htop-like system monitor for embedded Linux.

## Features
- CPU usage (Delta method from /proc/stat)
  
- Memory usage (/proc/meminfo)
  
- Process listing (/proc/[pid])
  
- Uptime display
  
- Real-time refresh (every 1 second)
  
- Exit with q
  
- Terminal UI with progress bars

## ▶️ How to Run

```bash
gcc mini_htop.c -o mini_htop
./mini_htop
```

Press q to exit.


















