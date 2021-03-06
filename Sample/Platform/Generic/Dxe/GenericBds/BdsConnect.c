/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BdsConnect.c

Abstract:

  BDS Lib functions which relate with connect the device

--*/

#include "BdsLib.h"

VOID
BdsLibConnectAll (
  VOID
  )
/*++

Routine Description:
  
  This function will connect all the system driver to controller
  first, and then special connect the default console, this make
  sure all the system controller avialbe and the platform default
  console connected.
  
Arguments:

  None

Returns:

  None

--*/
{
  //
  // Connect the platform console first
  //
  BdsLibConnectAllDefaultConsoles ();

  //
  // Generic way to connect all the drivers
  //
  BdsLibConnectAllDriversToAllControllers ();

  //
  // Here we have the assumption that we have already had
  // platform default console
  //
  BdsLibConnectAllDefaultConsoles ();
}

VOID
BdsLibGenericConnectAll (
  VOID
  )
/*++

Routine Description:
  
  This function will connect all the system drivers to all controllers
  first, and then connect all the console devices the system current 
  have. After this we should get all the device work and console avariable
  if the system have console device.
  
Arguments:

  None

Returns:

  None

--*/
{
  //
  // Most generic way to connect all the drivers
  //
  BdsLibConnectAllDriversToAllControllers ();
  BdsLibConnectAllConsoles ();
}

EFI_STATUS
BdsLibConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  )
/*++

Routine Description:
  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created success, then still give one chance to do the dispatch,
  which load the missing drivers if possible.
  
Arguments:

  DevicePathToConnect  - The device path which will be connected, it can
                         be a multi-instance device path

Returns:

  EFI_SUCCESS          - All handles associate with every device path 
                         node have been created
  
  EFI_OUT_OF_RESOURCES - There is no resource to create new handles
  
  EFI_NOT_FOUND        - Create the handle associate with one device 
                         path node failed

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  EFI_HANDLE                Handle;
  EFI_HANDLE                PreviousHandle;
  UINTN                     Size;

  if (DevicePathToConnect == NULL) {
    return EFI_SUCCESS;
  }

  DevicePath        = EfiDuplicateDevicePath (DevicePathToConnect);
  CopyOfDevicePath  = DevicePath;
  if (DevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  do {
    //
    // The outer loop handles multi instance device paths.
    // Only console variables contain multiple instance device paths.
    //
    // After this call DevicePath points to the next Instance
    //
    Instance  = EfiDevicePathInstance (&DevicePath, &Size);
    Next      = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);

    //
    // Start the real work of connect with RemainingDevicePath
    //
    PreviousHandle = NULL;
    do {
      //
      // Find the handle that best matches the Device Path. If it is only a
      // partial match the remaining part of the device path is returned in
      // RemainingDevicePath.
      //
      RemainingDevicePath = Instance;
      Status              = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &Handle);

      if (!EFI_ERROR (Status)) {
        if (Handle == PreviousHandle) {
          //
          // If no forward progress is made try invoking the Dispatcher.
          // A new FV may have been added to the system an new drivers
          // may now be found.
          // Status == EFI_SUCCESS means a driver was dispatched
          // Status == EFI_NOT_FOUND means no new drivers were dispatched
          //
          Status = gDS->Dispatch ();
        }

        if (!EFI_ERROR (Status)) {
          PreviousHandle = Handle;
          //
          // Connect all drivers that apply to Handle and RemainingDevicePath,
          // the Recursive flag is FALSE so only one level will be expanded.
          //
          // Do not check the connect status here, if the connect controller fail,
          // then still give the chance to do dispatch, because partial
          // RemainingDevicepath may be in the new FV
          //
          // 1. If the connect fail, RemainingDevicepath and handle will not
          //    change, so next time will do the dispatch, then dispatch's status
          //    will take effect
          // 2. If the connect success, the RemainingDevicepath and handle will
          //    change, then avoid the dispatch, we have chance to continue the
          //    next connection
          //
          gBS->ConnectController (Handle, NULL, RemainingDevicePath, FALSE);
        }
      }
      //
      // Loop until RemainingDevicePath is an empty device path
      //
    } while (!EFI_ERROR (Status) && !IsDevicePathEnd (RemainingDevicePath));

  } while (DevicePath != NULL);

  if (CopyOfDevicePath != NULL) {
    gBS->FreePool (CopyOfDevicePath);
  }
  //
  // All handle with DevicePath exists in the handle database
  //
  return Status;
}

EFI_STATUS
BdsLibConnectAllEfi (
  VOID
  )
/*++

Routine Description:

  This function will connect all current system handles recursively. The
  connection will finish until every handle's child handle created if it have.
  
Arguments:

  None

Returns:

  EFI_SUCCESS          - All handles and it's child handle have been connected
  
  EFI_STATUS           - Return the status of gBS->LocateHandleBuffer(). 

--*/
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;

  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
  }

  gBS->FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

EFI_STATUS
BdsLibDisconnectAllEfi (
  VOID
  )
/*++

Routine Description:

  This function will disconnect all current system handles. The disconnection
  will finish until every handle have been disconnected.
  
Arguments:

  None

Returns:

  EFI_SUCCESS          - All handles have been disconnected
  
  EFI_STATUS           - Return the status of gBS->LocateHandleBuffer(). 

--*/
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;

  //
  // Disconnect all
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
  }

  gBS->FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

VOID
BdsLibConnectAllDriversToAllControllers (
  VOID
  )
/*++

Routine Description:

  Connects all drivers to all controllers. 
  This function make sure all the current system driver will manage
  the correspoinding controllers if have. And at the same time, make
  sure all the system controllers have driver to manage it if have. 
  
Arguments:
  
  None
  
Returns:
  
  None
  
--*/
{
  EFI_STATUS  Status;

  do {
    //
    // Connect All EFI 1.10 drivers following EFI 1.10 algorithm
    //
    BdsLibConnectAllEfi ();

    //
    // Check to see if it's possible to dispatch an more DXE drivers.
    // The BdsLibConnectAllEfi () may have made new DXE drivers show up.
    // If anything is Dispatched Status == EFI_SUCCESS and we will try
    // the connect again.
    //
    Status = gDS->Dispatch ();

  } while (!EFI_ERROR (Status));

}

EFI_STATUS
BdsLibConnectUsbDevByShortFormDP(
  IN CHAR8                      HostControllerPI,
  IN EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath
  )
/*++

Routine Description:
  Connect the specific Usb device which match the short form device path, 
  and whose bus is determined by Host Controller (Uhci or Ehci)

  
Arguments:
  HostControllerPI       - Uhci (0x00) or Ehci (0x20) or Both uhci and ehci (0xFF)
  RemainingDevicePath - a short-form device path that starts with the first element 
                                          being a USB WWID or a USB Class device path
  
Returns:
  EFI_INVALID_PARAMETER
  EFI_SUCCESS
  EFI_NOT_FOUND
--*/
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            *HandleArray;
  UINTN                                 HandleArrayCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  BOOLEAN                               AtLeastOneConnected;
  
  //
  // Check the passed in parameters
  //
  if (RemainingDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }  
  
  if ((DevicePathType (RemainingDevicePath) != MESSAGING_DEVICE_PATH) ||
      ((DevicePathSubType (RemainingDevicePath) != MSG_USB_CLASS_DP) 
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
      && (DevicePathSubType (RemainingDevicePath) != MSG_USB_WWID_DP)
#endif
      )) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (HostControllerPI != 0xFF && 
      HostControllerPI != 0x00 && 
      HostControllerPI != 0x20) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Find the usb host controller firstly, then connect with the remaining device path
  //   
  AtLeastOneConnected = FALSE;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleArrayCount,
                  &HandleArray
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleArrayCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleArray[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&PciIo
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Check whether the Pci device is the wanted usb host controller
        //
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
        if (!EFI_ERROR (Status)) {
          if ((PCI_CLASS_SERIAL == Class[2]) &&
              (PCI_CLASS_SERIAL_USB == Class[1])) {
            if (HostControllerPI == Class[0] || HostControllerPI == 0xFF) {
              Status = gBS->ConnectController (
                              HandleArray[Index],
                              NULL,
                              RemainingDevicePath,
                              FALSE
                              );
              if (!EFI_ERROR(Status)) {
                AtLeastOneConnected = TRUE;
              }
            }
          } 
        }
      }
    }
    
    if (AtLeastOneConnected) {
      return EFI_SUCCESS;
    }
  }
  
  return EFI_NOT_FOUND;
}
