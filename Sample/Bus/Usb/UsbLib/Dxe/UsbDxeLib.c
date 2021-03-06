/*++

Copyright (c) 2004 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

 Module Name:

    UsbDxeLib.c

 Abstract:

   Common Dxe Libarary  for USB

 Revision History

--*/

#include "UsbDxeLib.h"


EFI_STATUS
UsbGetDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  OUT VOID                    *Descriptor,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get Descriptor

Arguments:

  UsbIo             - EFI_USB_IO_PROTOCOL
  Value             - Device Request Value
  Index             - Device Request Index 
  DescriptorLength  - Descriptor Length
  Descriptor        - Descriptor buffer to contain result
  Status            - Transfer Status
Returns:
  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = USB_DEV_GET_DESCRIPTOR_REQ_TYPE;
  DevReq.Request      = USB_REQ_GET_DESCRIPTOR;
  DevReq.Value        = Value;
  DevReq.Index        = Index;
  DevReq.Length       = DescriptorLength;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataIn,
                  TIMEOUT_VALUE,
                  Descriptor,
                  DescriptorLength,
                  Status
                  );
}

EFI_STATUS
UsbSetDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  IN  VOID                    *Descriptor,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Descriptor

Arguments:

  UsbIo             - EFI_USB_IO_PROTOCOL
  Value             - Device Request Value
  Index             - Device Request Index 
  DescriptorLength  - Descriptor Length
  Descriptor        - Descriptor buffer to set
  Status            - Transfer Status
Returns:
  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = USB_DEV_SET_DESCRIPTOR_REQ_TYPE;
  DevReq.Request      = USB_REQ_SET_DESCRIPTOR;
  DevReq.Value        = Value;
  DevReq.Index        = Index;
  DevReq.Length       = DescriptorLength;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataOut,
                  TIMEOUT_VALUE,
                  Descriptor,
                  DescriptorLength,
                  Status
                  );
}

EFI_STATUS
UsbGetInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Index,
  OUT UINT8                   *AltSetting,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get Device Interface

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  Index       - Interface index value
  AltSetting  - Alternate setting
  Status      - Trasnsfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 


--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = USB_DEV_GET_INTERFACE_REQ_TYPE;
  DevReq.Request      = USB_REQ_GET_INTERFACE;
  DevReq.Index        = Index;
  DevReq.Length       = 1;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataIn,
                  TIMEOUT_VALUE,
                  AltSetting,
                  1,
                  Status
                  );
}

EFI_STATUS
UsbSetInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  InterfaceNo,
  IN  UINT16                  AltSetting,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Device Interface

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  InterfaceNo - Interface Number
  AltSetting  - Alternate setting
  Status      - Trasnsfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = USB_DEV_SET_INTERFACE_REQ_TYPE;
  DevReq.Request      = USB_REQ_SET_INTERFACE;
  DevReq.Value        = AltSetting;
  DevReq.Index        = InterfaceNo;
 

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbNoData,
                  TIMEOUT_VALUE,
                  NULL,
                  0,
                  Status
                  );
}

EFI_STATUS
UsbGetConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  OUT UINT8                   *ConfigValue,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get Device Configuration

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  ConfigValue - Config Value
  Status      - Transfer Status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = USB_DEV_GET_CONFIGURATION_REQ_TYPE;
  DevReq.Request      = USB_REQ_GET_CONFIG;
  DevReq.Length       = 1;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataIn,
                  TIMEOUT_VALUE,
                  ConfigValue,
                  1,
                  Status
                  );
}

EFI_STATUS
UsbSetConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Device Configuration

Arguments:

  UsbIo   - EFI_USB_IO_PROTOCOL
  Value   - Configuration Value to set
  Status  - Transfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = USB_DEV_SET_CONFIGURATION_REQ_TYPE;
  DevReq.Request      = USB_REQ_SET_CONFIG;
  DevReq.Value        = Value;
 
  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbNoData,
                  TIMEOUT_VALUE,
                  NULL,
                  0,
                  Status
                  );
}

EFI_STATUS
UsbSetFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN       Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Device Feature

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Value     - Request value
  Target    - Request Index
  Status    - Transfer status

Returns:
  
  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  switch (Recipient) {

  case USB_TARGET_DEVICE:
    DevReq.RequestType = USB_DEV_SET_FEATURE_REQ_TYPE_D;
    break;

  case USB_TARGET_INTERFACE:
    DevReq.RequestType = USB_DEV_SET_FEATURE_REQ_TYPE_I;
    break;

  case USB_TARGET_ENDPOINT:
    DevReq.RequestType = USB_DEV_SET_FEATURE_REQ_TYPE_E;
    break;
  }
  //
  // Fill device request, see USB1.1 spec
  //
  DevReq.Request  = USB_REQ_SET_FEATURE;
  DevReq.Value    = Value;
  DevReq.Index    = Target;


  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbNoData,
                  TIMEOUT_VALUE,
                  NULL,
                  0,
                  Status
                  );
}

EFI_STATUS
UsbClearFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN       Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Clear Device Feature

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Value     - Request value
  Target    - Request Index
  Status    - Transfer status

Returns:
  
  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  switch (Recipient) {

  case USB_TARGET_DEVICE:
    DevReq.RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_D;
    break;

  case USB_TARGET_INTERFACE:
    DevReq.RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_I;
    break;

  case USB_TARGET_ENDPOINT:
    DevReq.RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_E;
    break;
  }
  //
  // Fill device request, see USB1.1 spec
  //
  DevReq.Request  = USB_REQ_CLEAR_FEATURE;
  DevReq.Value    = Value;
  DevReq.Index    = Target;


  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbNoData,
                  TIMEOUT_VALUE,
                  NULL,
                  0,
                  Status
                  );
}

EFI_STATUS
UsbGetStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN       Recipient,
  IN  UINT16                  Target,
  OUT UINT16                  *DevStatus,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get Device Status

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Target    - Request index
  DevStatus - Device status
  Status    - Transfer status

Returns:
  
  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  EfiZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  switch (Recipient) {

  case USB_TARGET_DEVICE:
    DevReq.RequestType = USB_DEV_GET_STATUS_REQ_TYPE_D;
    break;

  case USB_TARGET_INTERFACE:
    DevReq.RequestType = USB_DEV_GET_STATUS_REQ_TYPE_I;
    break;

  case USB_TARGET_ENDPOINT:
    DevReq.RequestType = USB_DEV_GET_STATUS_REQ_TYPE_E;
    break;
  }
  //
  // Fill device request, see USB1.1 spec
  //
  DevReq.Request  = USB_REQ_GET_STATUS;
  DevReq.Value    = 0;
  DevReq.Index    = Target;
  DevReq.Length   = 2;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataIn,
                  TIMEOUT_VALUE,
                  DevStatus,
                  2,
                  Status
                  );
}


EFI_STATUS
UsbClearEndpointHalt (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   EndpointNo,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Clear endpoint stall

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  EndpointNo  - Endpoint Number
  Status      - Transfer Status

Returns:

  EFI_NOT_FOUND    - Can't find the Endpoint
  EFI_DEVICE_ERROR - Hardware error
  EFI_SUCCESS      - Success

--*/
{
  EFI_STATUS                    Result;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  UINT8                         Index;

  EfiZeroMem (&EndpointDescriptor, sizeof (EFI_USB_ENDPOINT_DESCRIPTOR));
  //
  // First seach the endpoint descriptor for that endpoint addr
  //
  Result = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );
  if (EFI_ERROR (Result)) {
    return Result;
  }

  for (Index = 0; Index < InterfaceDescriptor.NumEndpoints; Index++) {
    Result = UsbIo->UsbGetEndpointDescriptor (
                      UsbIo,
                      Index,
                      &EndpointDescriptor
                      );
    if (EFI_ERROR (Result)) {
      continue;
    }

    if (EndpointDescriptor.EndpointAddress == EndpointNo) {
      break;
    }
  }

  if (Index == InterfaceDescriptor.NumEndpoints) {
    //
    // No such endpoint
    //
    return EFI_NOT_FOUND;
  }

  Result = UsbClearFeature (
            UsbIo,
            USB_TARGET_ENDPOINT,
            USB_FEATURE_ENDPOINT_HALT,
            EndpointDescriptor.EndpointAddress,
            Status
            );

  return Result;
}
