# Hopscotch Hash Table Implementation

## Overview
A robust implementation of a hash table using Hopscotch Hashing for collision resolution. This implementation provides efficient key-value storage with O(1) average-case complexity for insertions, deletions, and lookups.

## Features
- Custom implementation of Hopscotch Hashing
- Dynamic resizing with load factor management
- Efficient collision resolution
- Comprehensive memory management
- Thread-safe operations
- MurmurHash3 for hash generation

## Technical Details

### Core Components
- `dictionary_t`: Main hash table structure
- Load factor (α) management with 0.55 threshold
- Neighborhood size: 32 entries
- Bitmap-based collision tracking

### Key Operations
- `dictionary_put`: Insert or update key-value pairs
- `dictionary_get`: Retrieve values by key
- `dictionary_delete`: Remove key-value pairs
- `dictionary_pop`: Remove and return value
- `dictionary_contains`: Check key existence

### Performance
- Space Complexity: O(n)
- Average Time Complexity:
  - Insert: O(1)
  - Lookup: O(1)
  - Delete: O(1)

## Test Results
The implementation successfully passes a comprehensive suite of tests, including:
- Basic operations (create, put, get, delete)
- Edge cases handling
- Memory management
- Random sequence insertions and deletions
- Stress testing with large datasets

Example of memory test results:
```
=== Memory leak test ===
Total bytes allocated: 102292645904
Total bytes freed: 102292645904
Memory difference: 0 bytes
Malloc calls: 4194467
Free calls: 4194467
```

All tests pass successfully with zero memory leaks, demonstrating robust memory management even under heavy load.
```

## Implementation Details

### Collision Resolution
The Hopscotch Hashing algorithm maintains a neighborhood of size H (32 in this implementation) for each entry. When inserting a new item that collides, the algorithm attempts to find an empty slot within H-1 positions of the ideal slot.

### Resizing Strategy
The table automatically resizes when:
- Load factor exceeds 0.55
- No empty slots are available in the neighborhood
- Collision resolution fails

### Memory Management
- Custom memory management with proper cleanup
- Bitmap tracking for efficient space utilization
- Automatic garbage collection for removed entries

## Build and Run

### Prerequisites
- GCC compiler
- Make build system
- Docker (optional)

### Building and Running
```bash
make local    # Compile and run locally
```

### Using Docker
```bash
docker build -t hopscotch-hash .
docker run hopscotch-hash
```

## Usage Example
```c
dictionary_t *dict = dictionary_create(free);
dictionary_put(dict, "key1", value1);
void *value = dictionary_get(dict, "key1", &err);
dictionary_delete(dict, "key1");
dictionary_destroy(dict);
```

## Contributing
Feel free to submit issues and enhancement requests.

## License
This project is licensed under the MIT License.
