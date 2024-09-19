# Simple Garbage Collector in C++

This project demonstrates a simple garbage collector implemented in C++, inspired by the tri-color marking technique. Originally developed as part of my work on a custom scripting language, this standalone demo provides an easy-to-understand view of how garbage collection works behind the scenes. The goal is to help developers learn about memory management techniques in C++ through a practical example.

## Table of Contents
- [Introduction](#introduction)
- [How It Works](#how-it-works)
- [Key Features](#key-features)
- [Installation](#installation)

## Introduction

Garbage collection is a crucial aspect of programming, especially in languages like C++ where memory management is manual. This project simplifies the concept of garbage collection using marking techniques, providing a clear and educational approach for developers who want to understand how memory can be managed automatically.

## How It Works

The garbage collector uses a simplified tri-color marking technique:
- **Marked (Gray):** Objects being processed or checked, acting as a worklist.
- **Unmarked (White):** Objects that haven't been processed yet.
- **Collected (Black):** Objects confirmed to be in use and retained.

The collector automatically manages the lifecycle of variables and objects, freeing unused memory and preventing leaks. This approach demonstrates fundamental garbage collection concepts without the full complexity of a traditional tri-color system.

## Key Features
- **Memory Management:** Automatic handling of object allocation and deallocation.
- **Simple API:** Easy-to-understand functions for creating and managing objects.
- **Educational Value:** Designed as a learning tool for developers interested in garbage collection and memory management in C++.

## Installation

To build the project, you'll need CMake and a C++ compiler installed on your system.

```bash
# Clone the repository
git clone https://github.com/akadjoker/simplesgc.git

# Navigate into the project directory
cd simplesgc

# Build the project using CMake
cmake .
make

