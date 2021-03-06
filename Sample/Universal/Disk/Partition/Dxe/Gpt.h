/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Gpt.h
  
Abstract:

  Data Structures required for detecting GPT Partitions
  
Revision History

--*/

#ifndef _GPT_H_
#define _GPT_H_

#pragma pack(1)

#define PRIMARY_PART_HEADER_LBA 1

#define EFI_PTAB_HEADER_ID      "EFI PART"

//
// EFI Partition Attributes
//
#define EFI_PART_REQUIRED_TO_FUNCTION 0x0000000000000001

//
// GPT Partition Table Header
//
typedef struct {
  EFI_TABLE_HEADER  Header;
  EFI_LBA           MyLBA;
  EFI_LBA           AlternateLBA;
  EFI_LBA           FirstUsableLBA;
  EFI_LBA           LastUsableLBA;
  EFI_GUID          DiskGUID;
  EFI_LBA           PartitionEntryLBA;
  UINT32            NumberOfPartitionEntries;
  UINT32            SizeOfPartitionEntry;
  UINT32            PartitionEntryArrayCRC32;
} EFI_PARTITION_TABLE_HEADER;

//
// GPT Partition Entry
//
typedef struct {
  EFI_GUID  PartitionTypeGUID;
  EFI_GUID  UniquePartitionGUID;
  EFI_LBA   StartingLBA;
  EFI_LBA   EndingLBA;
  UINT64    Attributes;
  CHAR16    PartitionName[36];
} EFI_PARTITION_ENTRY;

//
// GPT Partition Entry Status
//
typedef struct {
  BOOLEAN OutOfRange;
  BOOLEAN Overlap;
} EFI_PARTITION_ENTRY_STATUS;

#pragma pack()

#endif
