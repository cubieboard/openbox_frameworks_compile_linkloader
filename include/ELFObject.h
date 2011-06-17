#ifndef ELF_OBJECT_H
#define ELF_OBJECT_H

#include "ELFTypes.h"
#include "MemChunk.h"
#include "StubLayout.h"

#include "utils/rsl_assert.h"

#include <llvm/ADT/OwningPtr.h>

#include <string>
#include <vector>

template <unsigned Bitwidth>
class ELFObject {
public:
  ELF_TYPE_INTRO_TO_TEMPLATE_SCOPE(Bitwidth);

private:
  llvm::OwningPtr<ELFHeaderTy> header;
  llvm::OwningPtr<ELFSectionHeaderTableTy> shtab;
  std::vector<ELFSectionTy *> stab;

#ifdef __arm__
  StubLayout stubs;
#endif

  MemChunk SHNCommonData;
  unsigned char *SHNCommonDataPtr;
  size_t SHNCommonDataFreeSize;

private:
  ELFObject() : SHNCommonDataPtr(NULL) { }

public:
  template <typename Archiver>
  static ELFObject *read(Archiver &AR);

  ELFHeaderTy const *getHeader() const {
    return header.get();
  }

  ELFSectionHeaderTableTy const *getSectionHeaderTable() const {
    return shtab.get();
  }

  char const *getSectionName(size_t i) const;
  ELFSectionTy const *getSectionByIndex(size_t i) const;
  ELFSectionTy *getSectionByIndex(size_t i);
  ELFSectionTy const *getSectionByName(std::string const &str) const;
  ELFSectionTy *getSectionByName(std::string const &str);

#ifdef __arm__
  StubLayout *getStubLayout() {
    return &stubs;
  }
#endif

  void *allocateSHNCommonData(size_t size, size_t align = 1) {
    rsl_assert(size > 0 && align != 0);

    enum { SHN_COMMON_DATA_SIZE = 0x40000 };

    if (!SHNCommonDataPtr) {
      // FIXME: We should not hard code these number!
      if (!SHNCommonData.allocate(SHN_COMMON_DATA_SIZE)) {
        return NULL;
      }

      SHNCommonDataPtr = SHNCommonData.getBuffer();
      SHNCommonDataFreeSize = SHN_COMMON_DATA_SIZE;
    }

    // Ensure alignment
    size_t rem = ((uintptr_t)SHNCommonDataPtr) % align;
    if (rem != 0) {
      SHNCommonDataPtr += align - rem;
      SHNCommonDataFreeSize -= align - rem;
    }

    // Ensure the free size is sufficient
    if (SHNCommonDataFreeSize < size) {
      return NULL;
    }

    // Allcoate
    void *result = SHNCommonDataPtr;
    SHNCommonDataPtr += size;
    SHNCommonDataFreeSize -= size;

    return result;
  }

  void relocate(void *(*find_sym)(void *context, char const *name),
                void *context);

  void print() const;

  ~ELFObject() {
    for (size_t i = 0; i < stab.size(); ++i) {
      // Delete will check the pointer is nullptr or not by himself.
      delete stab[i];
    }
  }

private:
  void relocateARM(void *(*find_sym)(void *context, char const *name),
                   void *context,
                   ELFSectionRelTableTy *reltab,
                   ELFSectionProgBitsTy *text);

  void relocateX86_32(void *(*find_sym)(void *context, char const *name),
                      void *context,
                      ELFSectionRelTableTy *reltab,
                      ELFSectionProgBitsTy *text);

  void relocateX86_64(void *(*find_sym)(void *context, char const *name),
                      void *context,
                      ELFSectionRelTableTy *reltab,
                      ELFSectionProgBitsTy *text);

};

#include "impl/ELFObject.hxx"

#endif // ELF_OBJECT_H
