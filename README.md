# PrintPipe

A thread-safe HTTP REST API server for a virtual printer driver with event-driven job management.

## Overview

PrintPipe provides a RESTful interface to manage print jobs through a complete lifecycle. It handles job creation, queuing, scheduling, spooling, and printing with full event tracking and state management.

## Features

- **RESTful API** - JSON-based REST endpoints for job management
- **Thread-Safe Job Queue** - Atomic state management with concurrent access support
- **Event Bus** - Real-time event tracking for all job state transitions
- **Virtual Printer** - TextSpooler for generating print buffers
- **Full Job Lifecycle** - Track jobs from creation through completion
- **Easy Integration** - Header-only dependencies, minimal setup

## Quick Start

### Build

```bash
mkdir -p build && cd build
cmake ..
make printpipe_http_server
