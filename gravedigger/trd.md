# Gravedigger Technical Requirements Document

## Overview

Gravedigger is a high-performance Bitcoin vanity address generator built on top of the Bitcoin Toolkit (BTK) framework. It leverages BTK's command-line interface and Bitcoin operations while implementing optimized cryptographic operations using Cosmopolitan LibC's native features.

## Architecture

### Component Integration
```
BTK Framework (Keep)        | Gravedigger Extensions
---------------------------|----------------------
Command Processing         | Optimized Key Generation
JSON I/O                  | Pattern Matching Engine
Option Parsing            | Thread Pool Management
Network Operations        | Performance Monitoring
```

### Module Structure
```
src/
├── btk.c (main entry - unchanged)
├── ctrl_mods/
│   ├── btk_*.c (existing BTK modules)
│   └── gd_vanity.c (new high-performance module)
└── mods/
    └── (existing BTK core modules)
```

### Key Components

1. **Command Interface**
   - Utilize BTK's command processing
   - Add new vanity-specific options
   - Maintain JSON input/output compatibility

2. **Cryptographic Operations**
   - Use Cosmopolitan's native crypto:
     - SHA256 for key operations
     - RIPEMD160 for address generation
   - Implement batch key generation
   - Optimize memory usage

3. **Pattern Matching**
   - Efficient regex compilation
   - Early-exit optimizations
   - Memory-efficient pattern storage

4. **Threading Model**
   - Thread pool for key generation
   - Thread-local storage for state
   - Lock-free counter updates
   - Work distribution strategy

5. **Performance Monitoring**
   - Keys/second tracking
   - Pattern match statistics
   - Resource utilization

## Implementation Strategy

1. **Phase 1: Core Integration**
   - Implement basic vanity generation
   - Integrate with BTK framework
   - Establish baseline performance

2. **Phase 2: Optimization**
   - Implement thread pool
   - Add batch key generation
   - Optimize pattern matching

3. **Phase 3: Features**
   - Add advanced pattern support
   - Implement progress saving
   - Add performance reporting

## Current Implementation Status

### Completed Features
1. **Command Interface**
   - ✓ BTK command processing integration
   - ✓ Vanity address generation command
   - ✓ Real-time progress reporting
   - ✓ JSON output compatibility

2. **Cryptographic Operations**
   - ✓ Native crypto implementation
   - ✓ Key generation working
   - ✓ Address generation verified
   - ✓ Memory-efficient operations

3. **Pattern Matching**
   - ✓ Basic pattern support
   - ✓ Case sensitivity options
   - ✓ Early matching verification
   - ✓ Efficient string comparison

4. **Threading Model**
   - ✓ Basic thread pool implementation
   - ✓ Thread-safe progress tracking
   - ✓ Proper thread cleanup
   - ✓ Work distribution

5. **Performance Monitoring**
   - ✓ Keys/second tracking
   - ✓ Attempt counting
   - ✓ Real-time status updates

### Performance Metrics
Current performance on test hardware:
- ~8-10 keys/second baseline
- Linear scaling with threads observed
- Memory usage stable during operation
- Successfully finds 2-char patterns (e.g. "1a") within ~140 attempts

### Binary Outputs
Successfully building three variants:
1. gravedigger.com (1.67 MB) - Main executable
2. gravedigger.aarch64.elf (2.51 MB) - ARM64 binary
3. gravedigger.com.dbg (3.03 MB) - Debug symbols

### Areas for Optimization
1. **Performance Enhancement**
   - Batch key generation
   - SIMD operations
   - Memory access patterns
   - Thread pool tuning

2. **Pattern Matching**
   - Advanced regex support
   - Multiple pattern search
   - Optimized string comparison

3. **Resource Usage**
   - CPU utilization
   - Memory pooling
   - Cache optimization

## Dependencies
- Cosmopolitan LibC (via cosmocc)
- No external libraries
- POSIX-compliant threading

## Performance Targets
- 100k+ keys/second on modern hardware
- Linear scaling with thread count
- Minimal memory footprint

## Testing Strategy
1. **Unit Tests**
   - Key generation correctness
   - Pattern matching accuracy
   - Thread safety verification

2. **Integration Tests**
   - BTK command compatibility
   - JSON input/output validation
   - Resource management

3. **Performance Tests**
   - Throughput benchmarking
   - Memory usage profiling
   - Thread scaling tests
