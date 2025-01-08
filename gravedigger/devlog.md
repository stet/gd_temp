# Gravedigger Development Log

## Session 2023-10-XX - Initial Setup and Stabilization

### Project Creation
- Successfully compiled bitcoin-toolkit using cosmocc
- Created gravedigger project by copying bitcoin-toolkit codebase
- Initial directory structure established

### Future OpenSSL Integration Path
1. OpenSSL Support in Cosmocc:
   - Cosmocc includes OpenSSL support through its superconfigure system
   - Uses OpenSSL 1.1.1u from source, not system installation
   - Built with specific configurations for static linking and minimal features

2. Isolated OpenSSL Build Process:
   ```makefile
   # Based on superconfigure's BUILD.mk
   OPENSSL_CONFIG_ARGS = no-shared no-asm \
       no-dso no-dynamic-engine no-engine no-pic \
       no-autoalginit no-autoerrinit \
       --with-rand-seed=getrandom \
       --openssldir=/zip/usr/share/ssl \
       CFLAGS="-Os" \
       --prefix=$(COSMOS)
   ```
   Key aspects:
   - Downloads OpenSSL 1.1.1u source directly
   - Builds statically (`no-shared`)
   - Disables unnecessary features (ASM, DSO, engines)
   - Uses minimal optimizations (`-Os`)
   - Installs to project-specific directory
   - Completely isolated from system OpenSSL

3. Integration Steps for Future OpenSSL Support:
   ```makefile
   # 1. Add project-specific OpenSSL paths
   OPENSSL_DIR = $(COSMOCC)/openssl-1.1.1u
   CFLAGS += -I$(OPENSSL_DIR)/include
   
   # 2. Add static library paths
   LDFLAGS += -L$(OPENSSL_DIR)/lib -lcrypto
   
   # 3. Remove the MISSING flags
   # -DEVP_H_MISSING -DPROVIDER_H_MISSING
   ```

4. Benefits of Isolated OpenSSL:
   - Version control: Known, tested OpenSSL version
   - Build consistency: Same configuration across systems
   - Security: No dependency on system OpenSSL patches
   - Portability: Works on systems without OpenSSL
   - Size optimization: Only includes needed features
   - No conflicts with system OpenSSL installation

3. Code Modifications Required:
   - Update `crypto.h` to use conditional compilation:
   ```c
   #ifdef USE_OPENSSL
   #include <openssl/sha.h>
   #include <openssl/ripemd.h>
   #else
   #include "crypto/sha256.h"
   #include "crypto/rmd160.h"
   #endif
   ```
   - Add struct aliases in `crypto.c`:
   ```c
   #ifdef USE_OPENSSL
   typedef SHA256_CTX sha256_context;
   typedef RIPEMD160_CTX rmd160_context;
   #endif
   ```
   - Update function implementations to use appropriate API

4. Build System Changes:
   - Add OpenSSL configuration option:
   ```makefile
   ifdef USE_OPENSSL
   CFLAGS += -DUSE_OPENSSL
   LDFLAGS += -lcrypto
   endif
   ```
   - Create separate build targets for OpenSSL/native versions

5. Benefits of Adding OpenSSL Support:
   - Potential performance improvements
   - Access to additional crypto functions
   - Compatibility with existing OpenSSL-based systems
   - Hardware acceleration support where available

6. Implementation Strategy:
   - Keep native implementation as default
   - Make OpenSSL optional via build flag
   - Maintain both implementations in parallel
   - Add comprehensive testing for both versions
   - Document performance comparisons

### OpenSSL vs Native Implementation Details
1. BTK's Original Approach:
   - BTK uses preprocessor directives to conditionally include crypto implementations:
   ```c
   #ifndef EVP_H_MISSING
   #include <openssl/evp.h>
   #else
   // Use native implementation
   #endif
   ```
   - When OpenSSL is available, BTK defaults to using it
   - Native implementations are only used as fallback when OpenSSL headers are missing

2. Issues Encountered:
   - Even with `-DEVP_H_MISSING` flag, some OpenSSL types were still being referenced
   - The build system would try to link against OpenSSL if available on the system
   - Conditional compilation wasn't completely isolating OpenSSL dependencies

3. Our Solution:
   - Force native implementation by:
     - Adding all OpenSSL-related missing header flags:
       ```makefile
       -DEVP_H_MISSING -DPROVIDER_H_MISSING
       ```
     - Removing any potential OpenSSL include paths
     - Using explicit includes for native implementations:
       ```c
       #include "crypto/sha256.h"
       #include "crypto/rmd160.h"
       ```
   - Modified struct names to match native implementations directly
   - Removed all OpenSSL type references
   - Ensured no linking against system OpenSSL libraries

4. Benefits of Native Implementation:
   - Reduced external dependencies
   - Consistent behavior across systems
   - Smaller binary size
   - Better control over crypto implementation
   - Simplified cross-compilation

### Build System Modifications
1. Updated Makefile configuration:
   - Changed output location to `/home/forge/tools.undernet.work/ape-playground/o/gravedigger/gravedigger.com`
   - Added automatic creation of output directories
   - Updated clean targets for proper artifact removal
   - Renamed main target from `btk-cosmocc` to `gravedigger`

2. Build System Improvements:
   - Streamlined build process
   - Added proper dependency handling
   - Ensured clean builds work correctly

### Modified Files
1. `Makefile` Changes:
   ```diff
   - EXES = btk-cosmocc
   + EXES = gravedigger
   
   - $(BIN)/$@
   + /home/forge/tools.undernet.work/ape-playground/o/gravedigger/gravedigger.com
   ```
   - Added directory creation for output path
   - Updated clean target to remove new output location

2. `src/mods/crypto.h` Changes:
   - Original: Used OpenSSL type definitions
   - Modified: Now uses built-in crypto header types
   ```diff
   - #include <openssl/sha.h>
   - #include <openssl/ripemd.h>
   + #include "crypto/sha256.h"
   + #include "crypto/rmd160.h"
   ```

3. `src/mods/crypto.c` Changes:
   - Original: Relied on OpenSSL crypto functions
   - Modified: Uses built-in implementations
   ```diff
   - struct sha256_ctx
   + sha256_context
   
   - struct rmd160_ctx
   + rmd160_context
   ```

4. `src/mods/privkey.c` Changes:
   - Updated struct definitions to match `privkey.h`
   - Fixed function signatures to handle return values properly
   - Added necessary includes for type definitions

### Code Changes
1. Crypto Implementation:
   - Migrated from OpenSSL to built-in crypto implementations
   - Updated struct names to match builtin crypto headers
   - Fixed function signatures and return types
   - Ensured proper type definitions

2. Bug Fixes:
   - Resolved issues in `privkey.c`
   - Fixed function declarations and implementations
   - Added necessary include statements
   - Corrected type definitions

### Testing & Verification
Successfully tested core functionality:
```bash
# Private key generation
gravedigger.com privkey --create

# Public key derivation
gravedigger.com pubkey <private_key>

# Address generation with QR code
gravedigger.com address --bech32 -Q <public_key>
```

### Backup & Stability
- Created comprehensive backup of working codebase
- Verified clean build process
- Confirmed rebuild stability
- Tested all core functionality after rebuild

### Current Status
- Project builds successfully
- All core functionality working:
  - Private key generation
  - Public key derivation
  - Address generation
  - QR code generation
- Clean build process verified
- Working code backed up
- Using built-in crypto implementations
- No dependency on OpenSSL

### Next Steps
- [ ] Add new features specific to gravedigger project
- [ ] Enhance documentation
- [ ] Add unit tests
- [ ] Consider CI/CD integration 

### Extending the Codebase
1. BTK's Design Philosophy:
   - Modular architecture with clear separation of concerns
   - Core functionality in `src/mods/` directory
   - Command handlers in `src/ctrl_mods/` directory
   - Each feature is self-contained and reusable

2. Extension Strategy:
   ```
   src/
   ├── mods/                 # BTK core modules (unchanged)
   ├── ctrl_mods/           # BTK command handlers
   │   ├── btk_*.{c,h}     # Original BTK handlers
   │   └── gd_*.{c,h}      # New Gravedigger handlers
   └── gravedigger/        # Gravedigger-specific modules
       ├── features/       # New feature implementations
       ├── utils/          # Gravedigger utilities
       └── protocols/      # Custom protocols
   ```

3. Integration Points:
   - Create new command handlers:
   ```c
   // src/ctrl_mods/gd_feature.h
   #include "../mods/address.h"
   #include "../gravedigger/features/feature.h"
   
   int gd_handle_feature(int argc, char **argv);
   ```
   
   - Register commands in main:
   ```c
   // src/btk.c
   #include "ctrl_mods/gd_feature.h"
   
   static const command_t commands[] = {
       // Original BTK commands
       {"privkey", btk_handle_privkey},
       // Gravedigger extensions
       {"feature", gd_handle_feature},
   };
   ```

4. Best Practices:
   - Never modify BTK core modules directly
   - Keep BTK functionality intact and accessible
   - Use BTK's existing utilities and types
   - Follow BTK's error handling patterns
   - Maintain consistent coding style
   - Document all extensions thoroughly

5. Example Extension: Custom Address Format
   ```c
   // src/gravedigger/features/custom_addr.h
   #include "../../mods/address.h"
   
   typedef struct {
       Address base;      // Use BTK's base type
       uint8_t extra[32]; // Add custom fields
   } GDAddress;
   
   int gd_address_create(const Address *base, GDAddress *out);
   ```

6. Testing Extensions:
   - Create separate test suites for new features
   - Verify BTK functionality remains unchanged
   - Test integration points thoroughly
   - Document test coverage
   ```
   test/
   ├── Tests/          # Original BTK tests
   └── GDTests/        # Gravedigger extension tests
   ```

7. Build System Integration:
   ```makefile
   # Add Gravedigger-specific source files
   GD_SRCS = $(wildcard src/gravedigger/*/*.c)
   GD_OBJS = $(GD_SRCS:.c=.o)
   
   # Include in main build
   $(TARGET): $(BTK_OBJS) $(GD_OBJS)
   ```

8. Documentation Strategy:
   - Maintain separate documentation for extensions
   - Cross-reference BTK documentation
   - Provide clear usage examples
   - Document design decisions
   ```
   docs/
   ├── btk/      # Original BTK documentation
   └── gd/       # Gravedigger extension docs
   ``` 

## 2025-01-08 Command Structure Update

The vanity address generator command has been properly integrated into the command system. Here's the correct usage:

```bash
./gravedigger.com vanity [options] [mode] pattern

Options:
  -i            Case-insensitive matching
  -t <threads>  Number of threads (default: 1)

Mode:
  prefix        Match at start of address
  suffix        Match at end of address
  anywhere      Match anywhere in address (default)

Examples:
# Case-sensitive prefix match using 4 threads
./gravedigger.com vanity -t 4 prefix nak

# Case-insensitive match anywhere using default thread count
./gravedigger.com vanity -i anywhere satoshi

# Simple prefix match with defaults
./gravedigger.com vanity prefix 1abc
```

Changes made:
- Integrated vanity command with standard option parsing system
- Added proper handling of -i (case-insensitive) and -t (threads) options
- Maintained support for different pattern matching modes (prefix/suffix/anywhere)
- Improved error handling and user feedback
- Added progress reporting with attempt count and rate