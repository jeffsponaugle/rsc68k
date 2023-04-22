#ifndef _RAM_H_
#define _RAM_H_

// Used for doing memory block allocation

extern EGCResultCode CreateMemoryHeap(UINT32 u32Size,
							   SMemoryHeap *psMemoryHeap,
							   UINT8 *pu8HeapName);

extern EGCResultCode AllocateMemoryInternal(SMemoryHeap *psMemoryHeap,
							 UINT32 u32Size,
							 void **ppvMemoryBlock,
							 UINT8 *pu8ModuleName,
							 UINT32 u32LineNumber,
							 BOOL bClearBlock);

#define AllocateMemory(x, y, z) AllocateMemoryInternal(x, y, z, (UINT8 *) __FILE__, (UINT32)__LINE__, TRUE)

extern EGCResultCode FreeMemory(SMemoryHeap *psMemoryHeap,
						 void **ppvMemoryToFree);

extern void SDRAMInit(void *pvSDRAMBase,
					   UINT32 u32SDRAMSize);
extern void HeapStats(FILE *fp);
extern void *SDRAMAllocInternal(UINT32 u32Size, UINT8 *pu8ModuleName, UINT32 u32LineNumber);
#define SDRAMAlloc(x) SDRAMAllocInternal(x, (UINT8 *) __FILE__, (UINT32) __LINE__)
extern void SDRAMFree(void *pvBase);
extern EGCResultCode SDRAMDropAnchor(void);
extern EGCResultCode SDRAMRaiseAnchor(void);
extern void GarbageCollect(SMemoryHeap *psMemoryHeap);
extern void HeapReport(SMemoryHeap *psHeap,
					   UINT8 *pu8Filename);
extern EGCResultCode ReallocMemoryInternal(SMemoryHeap *psMemoryHeap,
										   UINT32 u32Size,
										   void **ppvMemoryBlock,
										   UINT8 *pu8ModuleName,
										   UINT32 u32LineNumber);

#endif // _RAM_H_
