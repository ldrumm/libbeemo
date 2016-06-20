#ifndef BMO_ATOMIC_INTRINSICS
#define BMO_ATOMIC_INTRINSICS
#ifdef _MSC_VER
// http://msdn.microsoft.com/en-us/library/windows/desktop/2ddez55b(v=vs.85).aspx
// Windows XP and later: The return value is the resulting incremented value.
// Therefore we subtract 1 from the result to align with the GCC macro
#define BMO_ATOM_INC(xp) (InterlockedIncrement64((xp)) - 1)

#define BMO_ATOM_DEC(xp) (InterlockedDecrement64((xp)) - 1)

#define BMO_ATOM_CAS_POINTER(xp, old, new)                                     \
    (InterlockedCompareExchangePointer((xp), (new), (old)) == (old))
#elif __GNUC__
// http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
// These builtins perform the operation suggested by the name, and returns the
// value that had previously been in memory. That is,
//          { tmp = *ptr; *ptr op= value; return tmp; }
// Therefore the return value is the original value before add.
// With GCC, the atomic intrinsics accept any

#define BMO_ATOM_INC(xp) __sync_fetch_and_add((xp), 1)

#define BMO_ATOM_DEC(xp) __sync_fetch_and_add((xp), -1)

#define BMO_ATOM_CAS_POINTER(xp, old, new)                                     \
    __sync_bool_compare_and_swap((xp), (old), (new))
#endif
#endif

