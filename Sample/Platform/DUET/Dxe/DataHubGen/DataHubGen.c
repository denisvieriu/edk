/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DataHubGen.c

Abstract:

--*/

#include "DataHubGen.h"
#include "PeiHob.h"
#include "EfiHobLib.h"
#include "EfiDriverLib.h"
#include "EfiUiLib.h"
#include EFI_GUID_DEFINITION     (Hob)
#include EFI_GUID_DEFINITION     (DataHubRecords)
#include EFI_GUID_DEFINITION     (SmBios)
#include EFI_PROTOCOL_DEFINITION (DataHub)
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#include "UefiIfrLibrary.h"
#include EFI_PROTOCOL_DEFINITION (HiiDatabase)
EFI_HII_DATABASE_PROTOCOL        *gHiiDatabase;
#else
#include "IfrLibrary.h"
#include EFI_PROTOCOL_DEFINITION (Hii)
EFI_HII_PROTOCOL                 *gHii;
#endif

extern UINT8                DataHubGenStrings[];

EFI_DATA_HUB_PROTOCOL       *gDataHub;
EFI_HII_HANDLE              gStringHandle;

VOID *
GetSmbiosTablesFromHob (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  UINTN                       Size;
  EFI_PHYSICAL_ADDRESS       *Table;

  //
  // Get Hob List
  //
  
  Status = EfiLibGetSystemConfigurationTable (&gEfiHobListGuid, (VOID *) &HobList);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // If there is a SMBIOS table in the HOB add it to the EFI System table
  //
  Status = GetNextGuidHob (&HobList, &gEfiSmbiosTableGuid, &Table, &Size);
  if (!EFI_ERROR (Status)) {
    return (VOID *)(UINTN)*Table;
  }

  return NULL;
}

EFI_STATUS
PrepareHiiPackage (
  VOID
  )
{
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  EFI_HANDLE                        DriverHandle;
  EFI_HII_PACKAGE_LIST_HEADER       *PackageList;
  EFI_STATUS                        Status;

  CreateHiiDriverHandle (&DriverHandle);
  PackageList = PreparePackageList (1, &gMiscProducerGuid, DataHubGenStrings);

  Status = gHiiDatabase->NewPackageList (gHiiDatabase, PackageList, DriverHandle, &gStringHandle);
  gBS->FreePool (PackageList);

  return Status;
#else
  EFI_HII_PACKAGES                  *PackageList;
  EFI_STATUS                        Status;

  PackageList   = PreparePackages (1, &gMiscProducerGuid, DataHubGenStrings);
  Status        = gHii->NewPack (gHii, PackageList, &gStringHandle);

  return Status;
#endif
}

EFI_SUBCLASS_TYPE1_HEADER mCpuDataRecordHeader = {
  EFI_PROCESSOR_SUBCLASS_VERSION,       // Version
  sizeof (EFI_SUBCLASS_TYPE1_HEADER),   // Header Size
  0,                                    // Instance, Initialize later
  EFI_SUBCLASS_INSTANCE_NON_APPLICABLE, // SubInstance
  0                                     // RecordType, Initialize later
};

VOID
InstallProcessorDataHub (
  IN VOID                  *Smbios
  )
{
  EFI_STATUS                        Status;
  SMBIOS_STRUCTURE_POINTER          SmbiosTable;
  EFI_CPU_DATA_RECORD               DataRecord;
  CHAR8                             *AString;
  CHAR16                            *UString;
  STRING_REF                        Token;

  //
  // Processor info (TYPE 4)
  // 
  SmbiosTable = GetSmbiosTableFromType ((SMBIOS_TABLE_STRUCTURE *)Smbios, 4, 0);
  if (SmbiosTable.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "SmbiosTable: Type 4 (Processor Info) not found!\n"));
    return ;
  }

  //
  // Record Header
  //
  EfiCopyMem (&DataRecord, &mCpuDataRecordHeader, sizeof (DataRecord.DataRecordHeader));

  //
  // Record Type 1
  //
  DataRecord.DataRecordHeader.RecordType = ProcessorCoreFrequencyRecordType;
  DataRecord.VariableRecord.ProcessorCoreFrequency.Value = SmbiosTable.Type4->CurrentSpeed;
  DataRecord.VariableRecord.ProcessorCoreFrequency.Exponent = 6;

  Status = gDataHub->LogData (
                       gDataHub,
                       &gProcessorSubClassName,
                       &gMiscProducerGuid,
                       EFI_DATA_RECORD_CLASS_DATA,
                       &DataRecord,
                       sizeof (DataRecord.DataRecordHeader) + sizeof (DataRecord.VariableRecord.ProcessorCoreFrequency)
                       );
  //
  // Record Type 3
  //
  AString = GetSmbiosString (SmbiosTable, SmbiosTable.Type4->ProcessorVersion);
  UString = EfiLibAllocateZeroPool ((EfiAsciiStrLen(AString) + 1) * sizeof(CHAR16));
  ASSERT (UString != NULL);
  Ascii2Unicode (UString, AString);

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  Status = IfrLibNewString (gStringHandle, &Token, UString);
#else
  Token  = 0;
  Status = gHii->NewString (gHii, NULL, gStringHandle, &Token, UString);
#endif
  if (EFI_ERROR (Status)) {
    gBS->FreePool (UString);
    return ;
  }
  gBS->FreePool (UString);

  DataRecord.DataRecordHeader.RecordType = ProcessorVersionRecordType;
  DataRecord.VariableRecord.ProcessorVersion  = Token;

  Status = gDataHub->LogData (
                       gDataHub,
                       &gProcessorSubClassName,
                       &gMiscProducerGuid,
                       EFI_DATA_RECORD_CLASS_DATA,
                       &DataRecord,
                       sizeof (DataRecord.DataRecordHeader) + sizeof (DataRecord.VariableRecord.ProcessorVersion)
                       );

  return ;
}

VOID
InstallCacheDataHub (
  IN VOID                  *Smbios
  )
{
  return ;
}

EFI_SUBCLASS_TYPE1_HEADER mMemorySubclassDriverDataHeader = {
  EFI_MEMORY_SUBCLASS_VERSION,          // Version
  sizeof (EFI_SUBCLASS_TYPE1_HEADER),   // Header Size
  0,                                    // Instance, Initialize later
  EFI_SUBCLASS_INSTANCE_NON_APPLICABLE, // SubInstance
  0                                     // RecordType, Initialize later
};

VOID
InstallMemoryDataHub (
  IN VOID                  *Smbios
  )
{
  EFI_STATUS                        Status;
  SMBIOS_STRUCTURE_POINTER          SmbiosTable;
  EFI_MEMORY_SUBCLASS_DRIVER_DATA   DataRecord;

  //
  // Generate Memory Array Mapped Address info (TYPE 19)
  //
  SmbiosTable = GetSmbiosTableFromType ((SMBIOS_TABLE_STRUCTURE *)Smbios, 19, 0);
  if (SmbiosTable.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "SmbiosTable: Type 19 (Memory Array Mapped Address Info) not found!\n"));
    return ;
  }

  //
  // Record Header
  //
  EfiCopyMem (&DataRecord, &mMemorySubclassDriverDataHeader, sizeof (DataRecord.Header));
  
  //
  // Record Type 4
  //
  DataRecord.Header.RecordType = EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER;
  DataRecord.Record.ArrayStartAddress.MemoryArrayStartAddress = LShiftU64(SmbiosTable.Type19->StartingAddress, 10);
  DataRecord.Record.ArrayStartAddress.MemoryArrayEndAddress = LShiftU64((UINT64) SmbiosTable.Type19->EndingAddress + 1, 10) - 1;
  
  DataRecord.Record.ArrayStartAddress.PhysicalMemoryArrayLink.ProducerName = gMemoryProducerGuid;
  DataRecord.Record.ArrayStartAddress.PhysicalMemoryArrayLink.Instance = 0;
  DataRecord.Record.ArrayStartAddress.PhysicalMemoryArrayLink.SubInstance = EFI_SUBCLASS_INSTANCE_NON_APPLICABLE;
  DataRecord.Record.ArrayStartAddress.MemoryArrayPartitionWidth = (UINT16)(SmbiosTable.Type19->PartitionWidth); 

  Status = gDataHub->LogData (
                       gDataHub,
                       &gEfiMemorySubClassGuid,
                       &gMiscProducerGuid,
                       EFI_DATA_RECORD_CLASS_DATA,
                       &DataRecord,
                       sizeof (DataRecord.Header) + sizeof (DataRecord.Record.ArrayStartAddress)
                       );

  return ;
}

EFI_SUBCLASS_TYPE1_HEADER mMiscSubclassDriverDataHeader = {
  EFI_MISC_SUBCLASS_VERSION,            // Version
  sizeof (EFI_SUBCLASS_TYPE1_HEADER),   // Header Size
  0,                                    // Instance, Initialize later
  EFI_SUBCLASS_INSTANCE_NON_APPLICABLE, // SubInstance
  0                                     // RecordType, Initialize later
};

VOID
InstallMiscDataHub (
  IN VOID                  *Smbios
  )
{
  EFI_STATUS                        Status;
  SMBIOS_STRUCTURE_POINTER          SmbiosTable;
  EFI_MISC_SUBCLASS_DRIVER_DATA     DataRecord;
  CHAR8                             *AString;
  CHAR16                            *UString;
  STRING_REF                        Token;

  //
  // BIOS information (TYPE 0)
  // 
  SmbiosTable = GetSmbiosTableFromType ((SMBIOS_TABLE_STRUCTURE *)Smbios, 0, 0);
  if (SmbiosTable.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "SmbiosTable: Type 0 (BIOS Information) not found!\n"));
    return ;
  }

  //
  // Record Header
  //
  EfiCopyMem (&DataRecord, &mMiscSubclassDriverDataHeader, sizeof (DataRecord.Header));

  //
  // Record Type 2
  //
  AString = GetSmbiosString (SmbiosTable, SmbiosTable.Type0->BiosVersion);
  UString = EfiLibAllocateZeroPool ((EfiAsciiStrLen(AString) + 1) * sizeof(CHAR16) + sizeof(FIRMWARE_BIOS_VERSIONE));
  ASSERT (UString != NULL);
  EfiCopyMem (UString, FIRMWARE_BIOS_VERSIONE, sizeof(FIRMWARE_BIOS_VERSIONE));
  Ascii2Unicode (UString + sizeof(FIRMWARE_BIOS_VERSIONE) / sizeof(CHAR16) - 1, AString);

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  Status = IfrLibNewString (gStringHandle, &Token, UString);
#else
  Token  = 0;
  Status = gHii->NewString (gHii, NULL, gStringHandle, &Token, UString);
#endif
  if (EFI_ERROR (Status)) {
    gBS->FreePool (UString);
    return ;
  }
  gBS->FreePool (UString);

  DataRecord.Header.RecordType = EFI_MISC_BIOS_VENDOR_RECORD_NUMBER;
  DataRecord.Record.MiscBiosVendor.BiosVendor  = 0;
  DataRecord.Record.MiscBiosVendor.BiosVersion = Token;
  DataRecord.Record.MiscBiosVendor.BiosReleaseDate  = 0;
  DataRecord.Record.MiscBiosVendor.BiosStartingAddress  = 0;
  DataRecord.Record.MiscBiosVendor.BiosPhysicalDeviceSize.Value  = 0;
  DataRecord.Record.MiscBiosVendor.BiosPhysicalDeviceSize.Exponent  = 0;
//  DataRecord.Record.MiscBiosVendor.BiosCharacteristics1  = {0};
//  DataRecord.Record.MiscBiosVendor.BiosCharacteristics2  = {0};
  DataRecord.Record.MiscBiosVendor.BiosMajorRelease  = 0;
  DataRecord.Record.MiscBiosVendor.BiosMinorRelease  = 0;
  DataRecord.Record.MiscBiosVendor.BiosEmbeddedFirmwareMajorRelease  = 0;
  DataRecord.Record.MiscBiosVendor.BiosEmbeddedFirmwareMinorRelease  = 0;

  Status = gDataHub->LogData (
                       gDataHub,
                       &gMiscSubClassName,
                       &gMiscProducerGuid,
                       EFI_DATA_RECORD_CLASS_DATA,
                       &DataRecord,
                       sizeof (DataRecord.Header) + sizeof (DataRecord.Record.MiscBiosVendor)
                       );

  //
  // System information (TYPE 1)
  // 
  SmbiosTable = GetSmbiosTableFromType ((SMBIOS_TABLE_STRUCTURE *)Smbios, 1, 0);
  if (SmbiosTable.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "SmbiosTable: Type 1 (System Information) not found!\n"));
    return ;
  }

  //
  // Record Type 3
  //
  AString = GetSmbiosString (SmbiosTable, SmbiosTable.Type1->ProductName);
  UString = EfiLibAllocateZeroPool ((EfiAsciiStrLen(AString) + 1) * sizeof(CHAR16) + sizeof(FIRMWARE_PRODUCT_NAME));
  ASSERT (UString != NULL);
  EfiCopyMem (UString, FIRMWARE_PRODUCT_NAME, sizeof(FIRMWARE_PRODUCT_NAME));
  Ascii2Unicode (UString + sizeof(FIRMWARE_PRODUCT_NAME) / sizeof(CHAR16) - 1, AString);

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  Status = IfrLibNewString (gStringHandle, &Token, UString);
#else
  Token  = 0;
  Status = gHii->NewString (gHii, NULL, gStringHandle, &Token, UString);
#endif
  if (EFI_ERROR (Status)) {
    gBS->FreePool (UString);
    return ;
  }
  gBS->FreePool (UString);

  DataRecord.Header.RecordType = EFI_MISC_SYSTEM_MANUFACTURER_RECORD_NUMBER;
  DataRecord.Record.MiscSystemManufacturer.SystemManufacturer  = 0;
  DataRecord.Record.MiscSystemManufacturer.SystemProductName = Token;
  DataRecord.Record.MiscSystemManufacturer.SystemVersion  = 0;
  DataRecord.Record.MiscSystemManufacturer.SystemSerialNumber  = 0;
//  DataRecord.Record.MiscSystemManufacturer.SystemUuid  = {0};
  DataRecord.Record.MiscSystemManufacturer.SystemWakeupType  = 0;
  DataRecord.Record.MiscSystemManufacturer.SystemSKUNumber  = 0;
  DataRecord.Record.MiscSystemManufacturer.SystemFamily  = 0;

  Status = gDataHub->LogData (
                       gDataHub,
                       &gMiscSubClassName,
                       &gMiscProducerGuid,
                       EFI_DATA_RECORD_CLASS_DATA,
                       &DataRecord,
                       sizeof (DataRecord.Header) + sizeof (DataRecord.Record.MiscSystemManufacturer)
                       );

  return ;
}

EFI_DRIVER_ENTRY_POINT (DataHubGenEntrypoint);

EFI_STATUS
EFIAPI
DataHubGenEntrypoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;
  VOID                    *Smbios;

  EfiInitializeDriverLib (ImageHandle, SystemTable);

  Smbios = GetSmbiosTablesFromHob ();
  if (Smbios == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDataHubProtocolGuid,
                  NULL,
                  &gDataHub
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  &gHiiDatabase
                  );
#else
  Status = gBS->LocateProtocol (
                  &gEfiHiiProtocolGuid,
                  NULL,
                  &gHii
                  );
#endif
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrepareHiiPackage ();

  InstallProcessorDataHub (Smbios);
  InstallCacheDataHub     (Smbios);
  InstallMemoryDataHub    (Smbios);
  InstallMiscDataHub      (Smbios);

  return EFI_SUCCESS;
}

//
// Internal function
//

UINTN
SmbiosTableLength (
  IN SMBIOS_STRUCTURE_POINTER SmbiosTable
  )
{
  CHAR8  *AChar;
  UINTN  Length;

  AChar = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Hdr->Length);
  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++;
  }
  Length = ((UINTN)AChar - (UINTN)SmbiosTable.Raw + 2);
  
  return Length;
}

SMBIOS_STRUCTURE_POINTER
GetSmbiosTableFromType (
  IN SMBIOS_TABLE_STRUCTURE  *Smbios,
  IN UINT8                   Type,
  IN UINTN                   Index
  )
{
  SMBIOS_STRUCTURE_POINTER SmbiosTable;
  UINTN                    SmbiosTypeIndex;
  
  SmbiosTypeIndex = 0;
  SmbiosTable.Raw = (UINT8 *)(UINTN)Smbios->TableAddress;
  if (SmbiosTable.Raw == NULL) {
    return SmbiosTable;
  }
  while ((SmbiosTypeIndex != Index) || (SmbiosTable.Hdr->Type != Type)) {
    if (SmbiosTable.Hdr->Type == 127) {
      SmbiosTable.Raw = NULL;
      return SmbiosTable;
    }
    if (SmbiosTable.Hdr->Type == Type) {
      SmbiosTypeIndex ++;
    }
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosTableLength (SmbiosTable));
  }

  return SmbiosTable;
}

CHAR8 *
GetSmbiosString (
  IN SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING       String
  )
{
  CHAR8      *AString;
  UINT8      Index;

  Index = 1;
  AString = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Hdr->Length);
  while (Index != String) {
    while (*AString != 0) {
      AString ++;
    }
    AString ++;
    if (*AString == 0) {
      return AString;
    }
    Index ++;
  }

  return AString;
}
