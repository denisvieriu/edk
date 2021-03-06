;------------------------------------------------------------------------------
;*
;*   Copyright 2006 - 2007, Intel Corporation                                                         
;*   All rights reserved. This program and the accompanying materials                          
;*   are licensed and made available under the terms and conditions of the BSD License         
;*   which accompanies this distribution.  The full text of the license may be found at        
;*   http://opensource.org/licenses/bsd-license.php                                            
;*                                                                                             
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
;*   
;*    bs32.asm
;*  
;*   Abstract:
;*
;------------------------------------------------------------------------------

        .model  small
        .stack
        .486p
        .code

FAT_DIRECTORY_ENTRY_SIZE  EQU     020h
FAT_DIRECTORY_ENTRY_SHIFT EQU     5
BLOCK_SIZE                EQU     0200h
BLOCK_MASK                EQU     01ffh
BLOCK_SHIFT               EQU     9
                                               ; "EFILDR_____"
LOADER_FILENAME_PART1     EQU     04c494645h   ; "EFIL"
LOADER_FILENAME_PART2     EQU     030325244h   ; "DR20"
LOADER_FILENAME_PART3     EQU     020202030h   ; "0___"

        org 0h
Ia32Jump:
  jmp   BootSectorEntryPoint  ; JMP inst                  - 3 bytes
  nop

OemId             db  "INTEL   "    ; OemId               - 8 bytes
; BPB data below will be fixed by tool
SectorSize        dw  0             ; Sector Size         - 16 bits
SectorsPerCluster db  0             ; Sector Per Cluster  - 8 bits
ReservedSectors   dw  0             ; Reserved Sectors    - 16 bits
NoFats            db  0             ; Number of FATs      - 8 bits
RootEntries       dw  0             ; Root Entries        - 16 bits
Sectors           dw  0             ; Number of Sectors   - 16 bits
Media             db  0             ; Media               - 8 bits  - ignored
SectorsPerFat     dw  0             ; Sectors Per FAT     - 16 bits
SectorsPerTrack   dw  0             ; Sectors Per Track   - 16 bits - ignored
Heads             dw  0             ; Heads               - 16 bits - ignored
HiddenSectors     dd  0             ; Hidden Sectors      - 32 bits - ignored
LargeSectors      dd  0             ; Large Sectors       - 32 bits 

;******************************************************************************
;
;The structure for FAT32 starting at offset 36 of the boot sector. (At this point, 
;the BPB/boot sector for FAT12 and FAT16 differs from the BPB/boot sector for FAT32.)
;
;******************************************************************************

SectorsPerFat32   dd  0             ; Sectors Per FAT for FAT32       - 4 bytes
ExtFlags          dw  0             ; Mirror Flag                     - 2 bytes
FSVersion         dw  0             ; File System Version             - 2 bytes
RootCluster       dd  0             ; 1st Cluster Number of Root Dir  - 4 bytes
FSInfo            dw  0             ; Sector Number of FSINFO         - 2 bytes
BkBootSector      dw  0             ; Sector Number of Bk BootSector  - 2 bytes
Reserved          db  12 dup(0)     ; Reserved Field                  - 12 bytes
PhysicalDrive     db  0             ; Physical Drive Number           - 1 byte
Reserved1         db  0             ; Reserved Field                  - 1 byte
Signature         db  0             ; Extended Boot Signature         - 1 byte
VolId             db  "    "        ; Volume Serial Number            - 4 bytes
FatLabel          db  "           " ; Volume Label                    - 11 bytes
FileSystemType    db  "FAT32   "    ; File System Type                - 8 bytes

BootSectorEntryPoint:
        ASSUME  ds:@code
        ASSUME  ss:@code

; ****************************************************************************
; Start Print
; ****************************************************************************
  lea  si, cs:[StartString]
  call PrintString

; ****************************************************************************
; Print over
; ****************************************************************************

  mov   ax,cs         ; ax = 0
  mov   ss,ax         ; ss = 0
  add   ax,1000h
  mov   ds,ax

  mov   sp,07c00h     ; sp = 0x7c00
  mov   bp,sp         ; bp = 0x7c00

  mov   ah,8                                ; ah = 8 - Get Drive Parameters Function
  mov   byte ptr [bp+PhysicalDrive],dl      ; BBS defines that BIOS would pass the booting driver number to the loader through DL
  int   13h                                 ; Get Drive Parameters
  xor   ax,ax                   ; ax = 0
  mov   al,dh                   ; al = dh
  inc   al                      ; MaxHead = al + 1
  push  ax                      ; 0000:7bfe = MaxHead
  mov   al,cl                   ; al = cl
  and   al,03fh                 ; MaxSector = al & 0x3f
  push  ax                      ; 0000:7bfc = MaxSector

  cmp   word ptr [bp+SectorSignature],0aa55h  ; Verify Boot Sector Signature
  jne   BadBootSector
  mov   cx,word ptr [bp+RootEntries]      ; cx = RootEntries
  shl   cx,FAT_DIRECTORY_ENTRY_SHIFT      ; cx = cx * 32 = cx * sizeof(FAT_DIRECTORY_ENTRY) = Size of Root Directory in bytes
  mov   bx,cx                             ; bx = size of the Root Directory in bytes
  and   bx,BLOCK_MASK                     ; See if it is an even number of sectors long
  jne   BadBootSector                     ; If is isn't, then the boot sector is bad.
  mov   bx,cx                             ; bx = size of the Root Directory in bytes
  shr   bx,BLOCK_SHIFT                    ; bx = size of Root Directory in sectors
  mov   al,byte ptr [bp+NoFats]           ; al = NoFats
  xor   ah,ah                             ; ah = 0  ==> ax = NoFats
  mul   word ptr [bp+SectorsPerFat32]     ; ax = NoFats * SectorsPerFat
  add   ax,word ptr [bp+ReservedSectors]  ; ax = NoFats * SectorsPerFat + ReservedSectors = RootLBA
  add   ax,bx                             ; ax = NoFats * SectorsPerFat + ReservedSectors + RootDirSectors = FirstClusterLBA
  mov   word ptr [bp],ax                  ; Save FirstClusterLBA for later use
  
  mov   ax,word ptr [bp+RootCluster]      ; ax = StartCluster of Root Directory
  sub   ax,2                              ; ax = StartCluster - 2
  xor   bh,bh                               
  mov   bl,byte ptr [bp+SectorsPerCluster]; bx = SectorsPerCluster
  mul   bx                                ; ax = (StartCluster - 2) * SectorsPerCluster
  add   ax, word ptr [bp]                 ; ax = FirstClusterLBA + (StartCluster-2)*SectorsPerCluster
  push  ds
  pop   es
  xor   di,di                             ; Store directory in es:di = 1000:0000
  call  ReadBlocks                        ; Read StartCluster of Root Directory

  ; dx - variable storage (initial value is 0)
  ; bx - loader (initial value is 0)
  xor   dx, dx
  xor   bx, bx

FindEFILDR:
  cmp   dword ptr [di],LOADER_FILENAME_PART1         ; Compare to "EFIL"
  jne   FindVARSTORE
  cmp   dword ptr [di+4],LOADER_FILENAME_PART2
  jne   FindVARSTORE
  cmp   dword ptr [di+7],LOADER_FILENAME_PART3
  jne   FindVARSTORE
  mov   bx, word ptr [di+26]              ; bx = Start Cluster for EFILDR  <----------------------------------
  test  dx, dx
  je    FindNext                          ; Efivar.bin is not loaded
  jmp   FoundAll

FindVARSTORE:
  ; if the file is not loader file, see if it's "EFIVAR  BIN"
  cmp   dword ptr [di], 056494645h        ; Compare to "EFIV"
  jne   FindNext
  cmp   dword ptr [di+4], 020205241h      ; Compare to "AR  "
  jne   FindNext
  cmp   dword ptr [di+7], 04e494220h      ; Compare to " BIN"
  jne   FindNext
  mov   dx, di                            ; dx = Offset of Start Cluster for Efivar.bin <---------------------
  add   dx, 26
  test  bx, bx
  je    FindNext                          ; Efildr is not loaded
  jmp   FoundAll
  
FindNext:
  ; go to next find
  add   di,FAT_DIRECTORY_ENTRY_SIZE       ; Increment di
  sub   cx,FAT_DIRECTORY_ENTRY_SIZE       ; Decrement cx
  ; TODO: jump to FindVarStore if ...
  jne   FindEFILDR
  jmp   NotFoundAll

FoundAll:
FoundEFILDR:
  mov     cx,bx                               ; cx = Start Cluster for EFILDR  <----------------------------------
  mov     ax,cs                               ; Destination = 2000:0000
  add     ax,2000h
  mov     es,ax
  xor     di,di
ReadFirstClusterOfEFILDR:
  mov     ax,cx                               ; ax = StartCluster
  sub     ax,2                                ; ax = StartCluster - 2
  xor     bh,bh                               
  mov     bl,byte ptr [bp+SectorsPerCluster]  ; bx = SectorsPerCluster
  push    dx
  mul     bx
  pop     dx                                  ; ax = (StartCluster - 2) * SectorsPerCluster
  add     ax, word ptr [bp]                   ; ax = FirstClusterLBA + (StartCluster-2)*SectorsPerCluster
  xor     bh,bh
  mov     bl,byte ptr [bp+SectorsPerCluster]  ; bx = Number of Sectors in a cluster
  push    es
  call    ReadBlocks
  pop     ax
JumpIntoFirstSectorOfEFILDR:
  mov     word ptr [bp+JumpSegment],ax
JumpFarInstruction:
  db      0eah
JumpOffset:
  dw      0000h
JumpSegment:
  dw      2000h


PrintString:
  mov  ax,0b800h
  mov  es,ax
  mov  ax, 07c0h
  mov  ds, ax
  mov  cx, 7
  mov  di, 160
  rep  movsw
  ret
; ****************************************************************************
; ReadBlocks - Reads a set of blocks from a block device
;
; AX    = Start LBA
; BX    = Number of Blocks to Read
; ES:DI = Buffer to store sectors read from disk
; ****************************************************************************

; cx = Blocks
; bx = NumberOfBlocks
; si = StartLBA

ReadBlocks:
  pusha
  add     eax,dword ptr [bp+LBAOffsetForBootSector]    ; Add LBAOffsetForBootSector to Start LBA
  add     eax,dword ptr [bp+HiddenSectors]    ; Add HiddenSectors to Start LBA
  mov     esi,eax                             ; esi = Start LBA
  mov     cx,bx                               ; cx = Number of blocks to read
ReadCylinderLoop:
  mov     bp,07bfch                           ; bp = 0x7bfc
  mov     eax,esi                             ; eax = Start LBA
  xor     edx,edx                             ; edx = 0
  movzx   ebx,word ptr [bp]                   ; bx = MaxSector
  div     ebx                                 ; ax = StartLBA / MaxSector
  inc     dx                                  ; dx = (StartLBA % MaxSector) + 1
  sub     bx,dx                               ; bx = MaxSector - Sector
  inc     bx                                  ; bx = MaxSector - Sector + 1
  cmp     cx,bx                               ; Compare (Blocks) to (MaxSector - Sector + 1)
  jg      LimitTransfer
  mov     bx,cx                               ; bx = Blocks
LimitTransfer:
  push    cx
  mov     cl,dl                               ; cl = (StartLBA % MaxSector) + 1 = Sector
  xor     dx,dx                               ; dx = 0
  div     word ptr [bp+2]                     ; ax = ax / (MaxHead + 1) = Cylinder  
                                              ; dx = ax % (MaxHead + 1) = Head

  push    bx                                  ; Save number of blocks to transfer
  mov     dh,dl                               ; dh = Head
  mov     bp,07c00h                           ; bp = 0x7c00
  mov     dl,byte ptr [bp+PhysicalDrive]      ; dl = Drive Number
  mov     ch,al                               ; ch = Cylinder
  mov     al,bl                               ; al = Blocks
  mov     ah,2                                ; ah = Function 2
  mov     bx,di                               ; es:bx = Buffer address
  int     013h
  jc      DiskError
  pop     bx
  pop     cx
  movzx   ebx,bx
  add     esi,ebx                             ; StartLBA = StartLBA + NumberOfBlocks
  sub     cx,bx                               ; Blocks = Blocks - NumberOfBlocks
  mov     ax,es
  shl     bx,(BLOCK_SHIFT-4)
  add     ax,bx
  mov     es,ax                               ; es:di = es:di + NumberOfBlocks*BLOCK_SIZE
  cmp     cx,0
  jne     ReadCylinderLoop
  popa
  ret

; ****************************************************************************
; ERROR Condition:
; ****************************************************************************
NotFoundAll:
  ; if we found EFILDR, continue
  test bx,bx
  jne  FoundEFILDR
BadBootSector:
DiskError:
  lea  si, cs:[ErrorString]
  call PrintString
Halt:
  jmp   Halt

StartString:
  db 'B', 0ch, 'S', 0ch, 't', 0ch, 'a', 0ch, 'r', 0ch, 't', 0ch, '!', 0ch
ErrorString:
  db 'B', 0ch, 'E', 0ch, 'r', 0ch, 'r', 0ch, 'o', 0ch, 'r', 0ch, '!', 0ch

; ****************************************************************************
; LBA Offset for BootSector, need patched by tool for HD boot.
; ****************************************************************************

  org 01fah
LBAOffsetForBootSector:
  dd        0h

; ****************************************************************************
; Sector Signature
; ****************************************************************************

  org 01feh
SectorSignature:
  dw        0aa55h      ; Boot Sector Signature

  end 
  
