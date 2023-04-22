# 0 "../Shared/AsmUtils.s"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "../Shared/AsmUtils.s"
# 10 "../Shared/AsmUtils.s"
# 1 "../Shared/../Hardware/RSC68k.h" 1
# 11 "../Shared/AsmUtils.s" 2
# 1 "../Shared/../Shared/16550.h" 1
# 12 "../Shared/AsmUtils.s" 2

 .title "AsmUtils.s"

 .global MoveMultipleWrite
 .global MoveMultipleRead
 .global IDESectorMoveRead
 .global IDESectorMoveWrite
 .global RTCGetTickCounts
 .global SRSet
 .global SRGet
 .global SPGet



SRSet:
 movel %sp, %a0
 addl #6, %a0
 movew (%a0), %d0
 movew %d0, %sr
 rts

SRGet:
 movew %sr, %d0
 rts

SPGet:
 movel %sp, %d0
 rts

MoveMultipleWrite:
 moveml %d2-%d7/%a2,-(%sp)
 moveml %d0-%d7,-(%a2)
 moveml (%sp)+,%d2-%d7/%a2
 rts

MoveMultipleRead:
 moveml %d2-%d7/%a2,-(%sp)
 moveml (%a2)+, %d0-%d7
 moveml (%sp)+,%d2-%d7/%a2
 rts







IDESectorMoveRead:
 moveml %d2-%d7/%a2,-(%sp)



 movel %sp, %a0
 addl #4+24+4, %a0
 movel (%a0), %a2

 lea ((((0xff0000) + 0x100) + 0x100) + 0x100), %a0
 movel #32, %a1




 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)
 addal %a1, %a2

 moveml (%a0), %d0-%d7
 moveml %d0-%d7,(%a2)

 moveml (%sp)+,%d2-%d7/%a2
 rts







IDESectorMoveWrite:
 moveml %d2-%d7,-(%sp)



 movel %sp, %a0
 addl #4+24, %a0
 movel (%a0), %a1

 lea ((((0xff0000) + 0x100) + 0x100) + 0x100), %a0




 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%a1)+, %d0-%d7
 moveml %d0-%d7,(%a0)

 moveml (%sp)+,%d2-%d7
 rts







RTCGetTickCounts:
 moveml %d2-%d4,-(%sp)



 bsr RTCGetPowerOnHalfSeconds

 movel %d0, %d3
 movel #0x0, %d4



findEdge:
 bsr RTCGetPowerOnHalfSeconds
 cmp %d0, %d3
 beq findEdge

 movel %d0, %d3



timeCounter:
 addil #1, %d4
 bsr RTCGetPowerOnHalfSeconds
 cmp %d0, %d3
 beq timeCounter

finishCount:



 movel %d4, %d0
 moveml (%sp)+,%d2-%d4
 rts
