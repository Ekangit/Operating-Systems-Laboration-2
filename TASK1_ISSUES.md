# Task 1 Implementation Issues

## Current State
- `format()` — ✅ **DONE** (commits "Format function" and "Format function working")
- `create()` — ❌ **NEEDS REWRITE**
- `cat()`, `ls()`, `pwd()` — ⏳ **NOT STARTED** (stubs only)

---

## Problems in `create()` (fs.cpp, lines 41-179)

### 1. **Undefined Variables**
- **Line 50:** `cwd` variable not declared (should be class member)
- **Line 80, 172:** `cwb` variable not declared (should be class member)
- These will cause **compilation errors**

### 2. **Invalid C++ Syntax**
- **Line 103:** `vector<char[4096]> totalFileBuffer;`
  - **Error:** Cannot create vector of fixed-size arrays in C++
  - **Fix:** Use `vector<vector<uint8_t>>` or similar

### 3. **String Comparison Bug**
- **Line 92:** `directory_array[i].file_name == filepath`
  - Compares `char[56]` pointer with `string` 
  - Uses pointer comparison, not string content comparison
  - **Fix:** Use `strcmp(directory_array[i].file_name, filepath.c_str()) == 0`

### 4. **Input Format Mismatch**
- **Line 108:** `cin.read(buffer, 4096)` — binary read in 4KB chunks
- **Test uses:** `getline(cin, line)` — line-based text input
- **Result:** Won't work with test_script1.cpp input format
- **Fix:** Use `getline()` loop as shown in test

### 5. **Over-Engineered for Task 1**
- Lines 50-87: Path parsing and subdirectory traversal
  - Task 1 only needs **root directory** ("/")
  - Task 3-4 will handle subdirectories
- **Fix:** Remove path traversal, work directly with root block

### 6. **Missing Constants**
- **Line 127:** `newFile.access_rights = 6;` (hardcoded)
- **Should be:** `READ | WRITE` (from fs.h constants)

---

## What Needs to Be Done for Task 1

### Rewrite `create()` to:
1. ✅ Validate filename length (max 55 chars)
2. ✅ Read stdin with `getline()` until empty line
3. ✅ Read root directory from disk
4. ✅ Check if file exists, find empty slot
5. ✅ Allocate free FAT blocks for data
6. ✅ Write data blocks to disk
7. ✅ Add dir_entry to root
8. ✅ Write root and FAT back to disk

### Then Implement:
- `cat(filepath)` — read file and print
- `ls()` — list directory entries
- `pwd()` — print "/" for root
- Test with `make test1`

### Then Task 2:
- `cp()`, `mv()`, `rm()`, `append()`

---

## Notes
- `format()` is correct and should not be modified
- Class members `cwd` and `cwb` should NOT be added yet (not needed for Task 1)
- Focus on getting test1 to pass before moving to Task 2
