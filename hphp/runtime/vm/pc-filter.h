/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2013 Facebook, Inc. (http://www.facebook.com)     |
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef incl_HPHP_PC_FILTER_H_
#define incl_HPHP_PC_FILTER_H_

#include "hphp/runtime/vm/unit.h"

namespace HPHP {
////////////////////////////////////////////////////////////////////////////////

// Map which holds a set of PCs and supports reasonably fast addition and
// lookup. Used to decide if a given PC falls within an interesting area, e.g.,
// for breakpoints and stepping.
struct PCFilter {
private:
  // Radix-tree implementation of pointer map
  struct PtrMapNode;
struct PtrMap {
#define PTRMAP_PTR_SIZE       (sizeof(void*) * 8)
#define PTRMAP_LEVEL_BITS     8LL
#define PTRMAP_LEVEL_ENTRIES  (1LL << PTRMAP_LEVEL_BITS)
#define PTRMAP_LEVEL_MASK     (PTRMAP_LEVEL_ENTRIES - 1LL)

    PtrMap() {
      static_assert(PTRMAP_PTR_SIZE % PTRMAP_LEVEL_BITS == 0,
                    "PTRMAP_PTR_SIZE must be a multiple of PTRMAP_LEVEL_BITS");
      m_root = nullptr;
    }
    ~PtrMap() { clear(); }
    PtrMap(const PtrMap&) = delete;
    PtrMap& operator=(const PtrMap&) = delete;

    void setPointer(void* ptr, void* val);
    void* getPointer(void* ptr) const {
      if (!m_root) return nullptr;
      return getPointerImpl(ptr);
    }
    void clear();
    void swap(PtrMap& other) {
      std::swap(m_root, other.m_root);
    }
    bool isNull() { return m_root == nullptr; }
  private:
    void* getPointerImpl(void* ptr) const;
    void* m_root;
  };

  PtrMap m_map;

public:
  PCFilter() {}

  // Filter function to exclude opcodes when adding ranges.
  using OpcodeFilter = std::function<bool(Op)>;

  // Add/remove offsets, either individually or by range. By default allow all
  // opcodes.
  void addRanges(const Unit* unit, const OffsetRangeVec& offsets,
                 OpcodeFilter isOpcodeAllowed = [] (Op) { return true; });
  void removeRanges(const Unit* unit, const OffsetRangeVec& offsets,
                    OpcodeFilter isOpcodeAllowed = [] (Op) { return true; });
  void removeOffset(const Unit* unit, Offset offset);

  // Add/remove/check explicit PCs.
  void addPC(const unsigned char* pc) {
    m_map.setPointer((void*)pc, (void*)pc);
  }
  void removePC(const unsigned char* pc) {
    m_map.setPointer((void*)pc, nullptr);
  }
  bool checkPC(const unsigned char* pc) {
    return m_map.getPointer((void*)pc) == (void*)pc;
  }

  bool isNull() { return m_map.isNull(); }

  void swap(PCFilter& other) {
    m_map.swap(other.m_map);
  }

  void clear() {
    m_map.clear();
  }
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // incl_HPHP_PC_FILTER_H_
