/*+++

Copyright (c) 2004 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WinNtBusDriver.c

Abstract:

This following section documents the envirnoment variables for the Win NT 
build.  These variables are used to define the (virtual) hardware 
configuration of the NT environment

A ! can be used to seperate multiple instances in a variable. Each 
instance represents a seperate hardware device. 

EFI_WIN_NT_PHYSICAL_DISKS - maps to drives on your system
EFI_WIN_NT_VIRTUAL_DISKS  - maps to a device emulated by a file
EFI_WIN_NT_FILE_SYSTEM    - mouts a directory as a file system
EFI_WIN_NT_CONSOLE        - make a logical comand line window (only one!)
EFI_WIN_NT_UGA            - Builds UGA Windows of Width and Height
EFI_WIN_NT_GOP            - Builds GOP Windows of Width and Height
EFI_WIN_NT_SERIAL_PORT    - maps physical serial ports

 <F>ixed       - Fixed disk like a hard drive.
 <R>emovable   - Removable media like a floppy or CD-ROM.
 Read <O>nly   - Write protected device.
 Read <W>rite  - Read write device.
 <block count> - Decimal number of blocks a device supports.
 <block size>  - Decimal number of bytes per block.

 NT envirnonment variable contents. '<' and '>' are not part of the variable, 
 they are just used to make this help more readable. There should be no 
 spaces between the ';'. Extra spaces will break the variable. A '!' is  
 used to seperate multiple devices in a variable.

 EFI_WIN_NT_VIRTUAL_DISKS = 
   <F | R><O | W>;<block count>;<block size>[!...]

 EFI_WIN_NT_PHYSICAL_DISKS =
   <drive letter>:<F | R><O | W>;<block count>;<block size>[!...]

 Virtual Disks: These devices use a file to emulate a hard disk or removable
                media device. 
                
   Thus a 20 MB emulated hard drive would look like:
   EFI_WIN_NT_VIRTUAL_DISKS=FW;40960;512

   A 1.44MB emulated floppy with a block size of 1024 would look like:
   EFI_WIN_NT_VIRTUAL_DISKS=RW;1440;1024

 Physical Disks: These devices use NT to open a real device in your system

   Thus a 120 MB floppy would look like:
   EFI_WIN_NT_PHYSICAL_DISKS=B:RW;245760;512

   Thus a standard CD-ROM floppy would look like:
   EFI_WIN_NT_PHYSICAL_DISKS=Z:RO;307200;2048

 EFI_WIN_NT_FILE_SYSTEM = 
   <directory path>[!...]

   Mounting the two directories C:\FOO and C:\BAR would look like:
   EFI_WIN_NT_FILE_SYSTEM=c:\foo!c:\bar

 EFI_WIN_NT_CONSOLE = 
   <window title>

   Declaring a text console window with the title "My EFI Console" woild look like:
   EFI_WIN_NT_CONSOLE=My EFI Console

 EFI_WIN_NT_UGA = 
   <width> <height>[!...]

   Declaring a two UGA windows with resolutions of 800x600 and 1024x768 would look like:
   Example : EFI_WIN_NT_UGA=800 600!1024 768

 EFI_WIN_NT_GOP = 
   <width> <height>[!...]

   Declaring a two GOP windows with resolutions of 800x600 and 1024x768 would look like:
   Example : EFI_WIN_NT_GOP=800 600!1024 768

 EFI_WIN_NT_SERIAL_PORT = 
   <port name>[!...]

   Declaring two serial ports on COM1 and COM2 would look like:
   Example : EFI_WIN_NT_SERIAL_PORT=COM1!COM2

 EFI_WIN_NT_PASS_THROUGH =
   <BaseAddress>;<Bus#>;<Device#>;<Function#>

   Declaring a base address of 0xE0000000 (used for PCI Express devices)
   and having NT32 talk to a device located at bus 0, device 1, function 0:
   Example : EFI_WIN_NT_PASS_THROUGH=E000000;0;1;0

---*/

#include "WinNtBusDriver.h"
#include "PciHostBridge.h"

//
// Define GUID for the WinNt Bus Driver
//
static EFI_GUID gWinNtBusDriverGuid = {
  0x419f582, 0x625, 0x4531, 0x8a, 0x33, 0x85, 0xa9, 0x96, 0x5c, 0x95, 0xbc
};

//
// DriverBinding protocol global
//
EFI_DRIVER_BINDING_PROTOCOL           gWinNtBusDriverBinding = {
  WinNtBusDriverBindingSupported,
  WinNtBusDriverBindingStart,
  WinNtBusDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Table to map NT Environment variable to the GUID that should be in
// device path.
//
static NT_ENVIRONMENT_VARIABLE_ENTRY  mEnvironment[] = {
  L"EFI_WIN_NT_CONSOLE",          &gEfiWinNtConsoleGuid,
  L"EFI_WIN_NT_UGA",              &gEfiWinNtUgaGuid,
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  L"EFI_WIN_NT_GOP",              &gEfiWinNtGopGuid,
#endif
  L"EFI_WIN_NT_SERIAL_PORT",      &gEfiWinNtSerialPortGuid,
  L"EFI_WIN_NT_FILE_SYSTEM",      &gEfiWinNtFileSystemGuid,
  L"EFI_WIN_NT_VIRTUAL_DISKS",    &gEfiWinNtVirtualDisksGuid,
  L"EFI_WIN_NT_PHYSICAL_DISKS",   &gEfiWinNtPhysicalDisksGuid,
  L"EFI_WIN_NT_CPU_MODEL",        &gEfiWinNtCPUModelGuid,
  L"EFI_WIN_NT_CPU_SPEED",        &gEfiWinNtCPUSpeedGuid,
  L"EFI_MEMORY_SIZE",             &gEfiWinNtMemoryGuid,
  L"EFI_WIN_NT_PASS_THROUGH",     &gEfiWinNtPassThroughGuid,
  NULL, NULL
};

EFI_DRIVER_ENTRY_POINT (InitializeWinNtBusDriver)

EFI_STATUS
EFIAPI
InitializeWinNtBusDriver (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:
  Install DriverBinding Protocol for the Win NT Bus driver on the drivers
  image handle.

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_SUCEESS - DriverBinding protocol is added, or error status from 
                InstallProtocolInterface() is returned.

--*/
// TODO:    ImageHandle - add argument and description to function comment
// TODO:    SystemTable - add argument and description to function comment
{
  gHostBridgeInit     = FALSE;
  gReadPending        = FALSE;
  gImageHandle        = ImageHandle;
  gConfigData.Enable  = 1;
  gBaseAddress        = 0;

  CpuIoInitialize (ImageHandle, SystemTable);

  return INSTALL_ALL_DRIVER_PROTOCOLS_OR_PROTOCOLS2 (
          ImageHandle,
          SystemTable,
          &gWinNtBusDriverBinding,
          ImageHandle,
          &gWinNtBusDriverComponentName,
          NULL,
          NULL
          );
}

EFI_STATUS
EFIAPI
WinNtBusDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    ControllerHandle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_WIN_NT_THUNK_PROTOCOL *WinNtThunk;
  UINTN                     Index;

  //
  // Check the contents of the first Device Path Node of RemainingDevicePath to make sure
  // it is a legal Device Path Node for this bus driver's children.
  //
  if (RemainingDevicePath != NULL) {
    if (RemainingDevicePath->Type != HARDWARE_DEVICE_PATH ||
        RemainingDevicePath->SubType != HW_VENDOR_DP ||
        DevicePathNodeLength(RemainingDevicePath) != sizeof(WIN_NT_VENDOR_DEVICE_PATH_NODE)) {
      return EFI_UNSUPPORTED;
    }

    for (Index = 0; mEnvironment[Index].Variable != NULL; Index++) {
      if (EfiCompareGuid (&((VENDOR_DEVICE_PATH *) RemainingDevicePath)->Guid, mEnvironment[Index].DevicePathGuid)) {
        break;
      }
    }

    if (mEnvironment[Index].Variable == NULL) {
      return EFI_UNSUPPORTED;
    }
  }
  
  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  &ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiWinNtThunkProtocolGuid,
                  &WinNtThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Since we call through WinNtThunk we need to make sure it's valid
  //
  Status = EFI_SUCCESS;
  if (WinNtThunk->Signature != EFI_WIN_NT_THUNK_PROTOCOL_SIGNATURE) {
    Status = EFI_UNSUPPORTED;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiWinNtThunkProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  return Status;
}

EFI_STATUS
EFIAPI
WinNtBusDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    ControllerHandle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                      Status;
  EFI_STATUS                      InstallStatus;
  EFI_WIN_NT_THUNK_PROTOCOL       *WinNtThunk;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;
  UINT32                          Result;
  WIN_NT_BUS_DEVICE               *WinNtBusDevice;
  WIN_NT_IO_DEVICE                *WinNtDevice;
  UINTN                           Index;
  CHAR16                          *StartString;
  CHAR16                          *SubString;
  UINT16                          Count;
  UINTN                           Value;
  UINTN                           StringSize;
  UINT8                           NtEnvironmentVariableBuffer[MAX_NT_ENVIRNMENT_VARIABLE_LENGTH];
  UINT16                          ComponentName[MAX_NT_ENVIRNMENT_VARIABLE_LENGTH];
  WIN_NT_VENDOR_DEVICE_PATH_NODE  *Node;
  BOOLEAN                         CreateDevice;
  BOOLEAN                         NoWinNtConsole;

  Status = EFI_UNSUPPORTED;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  &ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiWinNtThunkProtocolGuid,
                  &WinNtThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  //
  // Define the gWinNtThunk interface that can be used by child processes like CPUIo
  // This will be used to communicate to the kernel driver for PCI I/O redirections.
  //
  gWinNtThunk = WinNtThunk;

  if (Status != EFI_ALREADY_STARTED) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (WIN_NT_BUS_DEVICE),
                    (VOID *) &WinNtBusDevice
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    WinNtBusDevice->Signature           = WIN_NT_BUS_DEVICE_SIGNATURE;
    WinNtBusDevice->ControllerNameTable = NULL;

    EfiLibAddUnicodeString (
      LANGUAGE_CODE_ENGLISH,
      gWinNtBusDriverComponentName.SupportedLanguages,
      &WinNtBusDevice->ControllerNameTable,
      L"Windows Bus Controller"
      );

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ControllerHandle,
                    &gWinNtBusDriverGuid,
                    WinNtBusDevice,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      EfiLibFreeUnicodeStringTable (WinNtBusDevice->ControllerNameTable);
      gBS->FreePool (WinNtBusDevice);
      return Status;
    }
  }

  //
  // Loop on the Variable list. Parse each variable to produce a set of handles that
  // represent virtual hardware devices.
  //
  NoWinNtConsole  = FALSE;
  InstallStatus   = EFI_NOT_FOUND;
  for (Index = 0; mEnvironment[Index].Variable != NULL; Index++) {
    Result = WinNtThunk->GetEnvironmentVariable (
                          mEnvironment[Index].Variable,
                          (CHAR16 *) NtEnvironmentVariableBuffer,
                          MAX_NT_ENVIRNMENT_VARIABLE_LENGTH
                          );

    //
    // We found the EFI_WIN_NT_PASS_THROUGH variable defined
    //
    if (EfiCompareGuid (mEnvironment[Index].DevicePathGuid, &gEfiWinNtPassThroughGuid) && (Result > 0)) {
      if (!gHostBridgeInit) {
        StartString = (CHAR16 *) NtEnvironmentVariableBuffer;
        Count       = 0;

        while (*StartString != '\0') {
          //
          // Is it a numeric value?
          //
          if (*StartString >= L'0' && *StartString <= L'9') {
            Value = *StartString - L'0';
          } else if (*StartString >= L'a' && *StartString <= L'f') {
            Value = *StartString - L'a' + 10;
          } else if (*StartString >= L'A' && *StartString <= L'F') {
            Value = *StartString - L'A' + 10;
          } else if (*StartString == L';') {
            StartString++;
            Count++;
            continue;
          } else {
            StartString++;
            continue;
          }

          switch (Count) {
          case 0:
            gBaseAddress = gBaseAddress * 16 + Value;
            break;

          case 1:
            gConfigData.Bus = gConfigData.Bus * 16 + Value;
            break;

          case 2:
            gConfigData.Dev = gConfigData.Dev * 16 + Value;
            break;

          case 3:
            gConfigData.Func = gConfigData.Func * 16 + Value;
            break;
          }

          StartString++;
        }

        continue;
      }
    }

    if (Result == 0) {
      if ((EfiCompareGuid (mEnvironment[Index].DevicePathGuid, &gEfiWinNtUgaGuid) && NoWinNtConsole) ||
          (EfiCompareGuid (mEnvironment[Index].DevicePathGuid, &gEfiWinNtGopGuid) && NoWinNtConsole)) {
        //
        // If there is no GOP or UGA force a default 800 x 600 console
        //
        StartString = L"800 600";
      } else {
        if (EfiCompareGuid (mEnvironment[Index].DevicePathGuid, &gEfiWinNtConsoleGuid)) {
          NoWinNtConsole = TRUE;
        }

        continue;
      }
    } else {
      StartString = (CHAR16 *) NtEnvironmentVariableBuffer;
    }

    //
    // Parse the envirnment variable into sub strings using '!' as a delimator.
    // Each substring needs it's own handle to be added to the system. This code
    // does not understand the sub string. Thats the device drivers job.
    //
    Count = 0;
    while (*StartString != '\0') {

      //
      // Find the end of the sub string
      //
      SubString = StartString;
      while (*SubString != '\0' && *SubString != '!') {
        SubString++;
      }

      if (*SubString == '!') {
        //
        // Replace token with '\0' to make sub strings. If this is the end
        //  of the string SubString will already point to NULL.
        //
        *SubString = '\0';
        SubString++;
      }

      CreateDevice = TRUE;
      if (RemainingDevicePath != NULL) {
        CreateDevice  = FALSE;
        Node          = (WIN_NT_VENDOR_DEVICE_PATH_NODE *) RemainingDevicePath;
        if (Node->VendorDevicePath.Header.Type == HARDWARE_DEVICE_PATH &&
            Node->VendorDevicePath.Header.SubType == HW_VENDOR_DP &&
            DevicePathNodeLength (&Node->VendorDevicePath.Header) == sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)
            ) {
          if (EfiCompareGuid (&Node->VendorDevicePath.Guid, mEnvironment[Index].DevicePathGuid) &&
              Node->Instance == Count
              ) {
            CreateDevice = TRUE;
          }
        }
      }

      if (CreateDevice) {

        //
        // Allocate instance structure, and fill in parent information.
        //
        Status = gBS->AllocatePool (
                        EfiBootServicesData,
                        sizeof (WIN_NT_IO_DEVICE),
                        (VOID *) &WinNtDevice
                        );
        if (EFI_ERROR (Status)) {
          return EFI_OUT_OF_RESOURCES;
        }

        WinNtDevice->Handle             = NULL;
        WinNtDevice->ControllerHandle   = ControllerHandle;
        WinNtDevice->ParentDevicePath   = ParentDevicePath;

        WinNtDevice->WinNtIo.WinNtThunk = WinNtThunk;

        //
        // Plus 2 to account for the NULL at the end of the Unicode string
        //
        StringSize = (UINTN) ((UINT8 *) SubString - (UINT8 *) StartString) + 2;
        Status = gBS->AllocatePool (
                        EfiBootServicesData,
                        StringSize,
                        (VOID *) &WinNtDevice->WinNtIo.EnvString
                        );
        if (EFI_ERROR (Status)) {
          WinNtDevice->WinNtIo.EnvString = NULL;
        } else {
          EfiCopyMem (WinNtDevice->WinNtIo.EnvString, StartString, StringSize);
        }

        WinNtDevice->ControllerNameTable = NULL;

        WinNtThunk->SPrintf (ComponentName, L"%s=%s", mEnvironment[Index].Variable, WinNtDevice->WinNtIo.EnvString);

        WinNtDevice->DevicePath = WinNtBusCreateDevicePath (
                                    ParentDevicePath,
                                    mEnvironment[Index].DevicePathGuid,
                                    Count
                                    );
        if (WinNtDevice->DevicePath == NULL) {
          gBS->FreePool (WinNtDevice);
          return EFI_OUT_OF_RESOURCES;
        }

        EfiLibAddUnicodeString (
          LANGUAGE_CODE_ENGLISH,
          gWinNtBusDriverComponentName.SupportedLanguages,
          &WinNtDevice->ControllerNameTable,
          ComponentName
          );

        WinNtDevice->WinNtIo.TypeGuid       = mEnvironment[Index].DevicePathGuid;
        WinNtDevice->WinNtIo.InstanceNumber = Count;

        WinNtDevice->Signature              = WIN_NT_IO_DEVICE_SIGNATURE;

        Status = gBS->InstallMultipleProtocolInterfaces (
                        &WinNtDevice->Handle,
                        &gEfiDevicePathProtocolGuid,
                        WinNtDevice->DevicePath,
                        &gEfiWinNtIoProtocolGuid,
                        &WinNtDevice->WinNtIo,
                        NULL
                        );
        if (EFI_ERROR (Status)) {
          EfiLibFreeUnicodeStringTable (WinNtDevice->ControllerNameTable);
          gBS->FreePool (WinNtDevice);
        } else {
          //
          // Open For Child Device
          //
          Status = gBS->OpenProtocol (
                          ControllerHandle,
                          &gEfiWinNtThunkProtocolGuid,
                          &WinNtThunk,
                          This->DriverBindingHandle,
                          WinNtDevice->Handle,
                          EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                          );
          if (!EFI_ERROR (Status)) {
            InstallStatus = EFI_SUCCESS;
          }
        }
      }

      //
      // Parse Next sub string. This will point to '\0' if we are at the end.
      //
      Count++;
      StartString = SubString;
    }
  }

  gDeviceHandle = WinNtThunk->CreateFile (
                                L"\\\\.\\DeviceUnderDevelopment", // Open the Kernel driver "file"/IOCTL
                                GENERIC_READ,                     // Open the interface Read-Only
                                FILE_SHARE_READ,                  // Allow others to get Read-Only access
                                NULL,                             // Default security options
                                OPEN_EXISTING,                    // Succeeds only if Kernel driver exported interface
                                0,                                // Not used for non-file objects
                                NULL                              // No template since not opening a real file
                                );

  //
  // Did we successfully open the IOCTL?
  //
  if (gDeviceHandle != INVALID_HANDLE_VALUE) {
    //
    // Call out to HostBridge Constructor - do it once, and never again
    // Pass in the parameters we discovered in the variable.
    // We do this only if the expected Kernel driver installed an interface for us
    //
    if (!gHostBridgeInit) {
      InitializePciHostBridge (gImageHandle, gST);
      gHostBridgeInit = TRUE;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
/*++

Routine Description:

Arguments:

Returns:

    None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    ControllerHandle - add argument and description to function comment
// TODO:    NumberOfChildren - add argument and description to function comment
// TODO:    ChildHandleBuffer - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                Status;
  UINTN                     Index;
  BOOLEAN                   AllChildrenStopped;
  EFI_WIN_NT_IO_PROTOCOL    *WinNtIo;
  WIN_NT_BUS_DEVICE         *WinNtBusDevice;
  WIN_NT_IO_DEVICE          *WinNtDevice;
  EFI_WIN_NT_THUNK_PROTOCOL *WinNtThunk;

  //
  // Complete all outstanding transactions to Controller.
  // Don't allow any new transaction to Controller to be started.
  //

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gWinNtBusDriverGuid,
                    &WinNtBusDevice,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    gBS->UninstallMultipleProtocolInterfaces (
          ControllerHandle,
          &gWinNtBusDriverGuid,
          WinNtBusDevice,
          NULL
          );

    EfiLibFreeUnicodeStringTable (WinNtBusDevice->ControllerNameTable);

    gBS->FreePool (WinNtBusDevice);

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiWinNtThunkProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiWinNtIoProtocolGuid,
                    &WinNtIo,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {

      WinNtDevice = WIN_NT_IO_DEVICE_FROM_THIS (WinNtIo);

      Status = gBS->CloseProtocol (
                      ControllerHandle,
                      &gEfiWinNtThunkProtocolGuid,
                      This->DriverBindingHandle,
                      WinNtDevice->Handle
                      );

      Status = gBS->UninstallMultipleProtocolInterfaces (
                      WinNtDevice->Handle,
                      &gEfiDevicePathProtocolGuid,
                      WinNtDevice->DevicePath,
                      &gEfiWinNtIoProtocolGuid,
                      &WinNtDevice->WinNtIo,
                      NULL
                      );

      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
              ControllerHandle,
              &gEfiWinNtThunkProtocolGuid,
              (VOID **) &WinNtThunk,
              This->DriverBindingHandle,
              WinNtDevice->Handle,
              EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
              );
      } else {
        //
        // Close the child handle
        //
        EfiLibFreeUnicodeStringTable (WinNtDevice->ControllerNameTable);
        gBS->FreePool (WinNtDevice);
      }
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_DEVICE_PATH_PROTOCOL *
WinNtBusCreateDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *RootDevicePath,
  IN  EFI_GUID                  *Guid,
  IN  UINT16                    InstanceNumber
  )
/*++

Routine Description:
  Create a device path node using Guid and InstanceNumber and append it to
  the passed in RootDevicePath

Arguments:
  RootDevicePath - Root of the device path to return.

  Guid           - GUID to use in vendor device path node.

  InstanceNumber - Instance number to use in the vendor device path. This
                    argument is needed to make sure each device path is unique.

Returns:

  EFI_DEVICE_PATH_PROTOCOL 

--*/
{
  WIN_NT_VENDOR_DEVICE_PATH_NODE  DevicePath;

  DevicePath.VendorDevicePath.Header.Type     = HARDWARE_DEVICE_PATH;
  DevicePath.VendorDevicePath.Header.SubType  = HW_VENDOR_DP;
  SetDevicePathNodeLength (&DevicePath.VendorDevicePath.Header, sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE));

  //
  // The GUID defines the Class
  //
  EfiCopyMem (&DevicePath.VendorDevicePath.Guid, Guid, sizeof (EFI_GUID));

  //
  // Add an instance number so we can make sure there are no Device Path
  // duplication.
  //
  DevicePath.Instance = InstanceNumber;

  return EfiAppendDevicePathNode (
          RootDevicePath,
          (EFI_DEVICE_PATH_PROTOCOL *) &DevicePath
          );
}
