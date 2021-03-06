/*++
Copyright (c) 2004 - 2010, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  DriverSample.c

Abstract:

  This is an example of how a driver might export data to the HII protocol to be
  later utilized by the Setup Protocol

--*/

#include "DriverSample.h"

#define DISPLAY_ONLY_MY_ITEM  0x0000

EFI_GUID   mFormSetGuid = FORMSET_GUID;
EFI_GUID   mInventoryGuid = INVENTORY_GUID;

CHAR16     VariableName[] = L"MyIfrNVData";

EFI_DRIVER_ENTRY_POINT (DriverSampleInit)

VOID
EncodePassword (
  IN  CHAR16                      *Password,
  IN  UINT8                       MaxSize
  )
{
  UINTN   Index;
  UINTN   Loop;
  CHAR16  *Buffer;
  CHAR16  *Key;

  Key     = L"MAR10648567";
  Buffer  = EfiLibAllocateZeroPool (MaxSize);
  ASSERT (Buffer != NULL);

  for (Index = 0; Key[Index] != 0; Index++) {
    for (Loop = 0; Loop < (UINT8) (MaxSize / 2); Loop++) {
      Buffer[Loop] = (CHAR16) (Password[Loop] ^ Key[Index]);
    }
  }

  EfiCopyMem (Password, Buffer, MaxSize);

  gBS->FreePool (Buffer);
  return ;
}

EFI_STATUS
ValidatePassword (
  DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData,
  EFI_STRING_ID                   StringId
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  UINTN                           BufferSize;
  CHAR16                          *Password;
  CHAR16                          *EncodedPassword;
  BOOLEAN                         OldPassword;

  //
  // Get encoded password first
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
                  VariableName,
                  &mFormSetGuid,
                  NULL,
                  &BufferSize,
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    //
    // Old password not exist, prompt for new password
    //
    return EFI_SUCCESS;
  }

  OldPassword = FALSE;
  //
  // Check whether we have any old password set
  //
  for (Index = 0; Index < 20; Index++) {
    if (PrivateData->Configuration.WhatIsThePassword2[Index] != 0) {
      OldPassword = TRUE;
      break;
    }
  }
  if (!OldPassword) {
    //
    // Old password not exist, return EFI_SUCCESS to prompt for new password
    //
    return EFI_SUCCESS;
  }

  //
  // Get user input password
  //
  BufferSize = 21 * sizeof (CHAR16);
  Password = EfiLibAllocateZeroPool (BufferSize);
  ASSERT (Password != NULL);

  Status = IfrLibGetString (PrivateData->HiiHandle[0], StringId, Password, &BufferSize);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (Password);
    return Status;
  }

  //
  // Validate old password
  //
  EncodedPassword = EfiLibAllocateCopyPool (21 * sizeof (CHAR16), Password);
  ASSERT (EncodedPassword != NULL);
  EncodePassword (EncodedPassword, 20 * sizeof (CHAR16));
  if (EfiCompareMem (EncodedPassword, PrivateData->Configuration.WhatIsThePassword2, 20 * sizeof (CHAR16)) != 0) {
    //
    // Old password mismatch, return EFI_NOT_READY to prompt for error message
    //
    Status = EFI_NOT_READY;
  } else {
    Status = EFI_SUCCESS;
  }

  gBS->FreePool (Password);
  gBS->FreePool (EncodedPassword);

  return Status;
}

EFI_STATUS
SetPassword (
  DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData,
  EFI_STRING_ID                   StringId
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  CHAR16                          *Password;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
                  VariableName,
                  &mFormSetGuid,
                  NULL,
                  &BufferSize,
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get user input password
  //
  Password = &PrivateData->Configuration.WhatIsThePassword2[0];
  EfiZeroMem (Password, 20 * sizeof (CHAR16));
  Status = IfrLibGetString (PrivateData->HiiHandle[0], StringId, Password, &BufferSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Retrive uncommitted data from Browser
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Configuration = EfiLibAllocateZeroPool (sizeof (DRIVER_SAMPLE_PRIVATE_DATA));
  ASSERT (Configuration != NULL);
  Status = GetBrowserData (&mFormSetGuid, VariableName, &BufferSize, (UINT8 *) Configuration);
  if (!EFI_ERROR (Status)) {
    //
    // Update password's clear text in the screen
    //
    EfiCopyMem (Configuration->PasswordClearText, Password, 20 * sizeof (CHAR16));

    //
    // Update uncommitted data of Browser
    //
    BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
    Status = SetBrowserData (
               &mFormSetGuid,
               VariableName,
               BufferSize,
               (UINT8 *) Configuration,
               NULL
               );
  }
  gBS->FreePool (Configuration);

  //
  // Set password
  //
  EncodePassword (Password, 20 * sizeof (CHAR16));
  Status = gRT->SetVariable(
                  VariableName,
                  &mFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (DRIVER_SAMPLE_CONFIGURATION),
                  &PrivateData->Configuration
                  );
  return Status;
}

EFI_STATUS
LoadNameValueNames (
  IN DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData
  )
/*++

  Routine Description:
    Update names of Name/Value storage to current language.

  Arguments:
    PrivateData - Points to the driver private data.

  Returns:
    EFI_SUCCESS - All names are successfully updated.
    Others      - Failed to get Name from HII database.

--*/
{
  UINTN      Index;
  UINTN      BufferSize;
  EFI_STATUS Status;

  //
  // Get Name/Value name string of current language
  //
  for (Index = 0; Index < NAME_VALUE_NAME_NUMBER; Index++) {

    BufferSize = NAME_VALUE_MAX_STRING_LEN * sizeof (CHAR16);
    Status = IfrLibGetString (
               PrivateData->HiiHandle[0],
               PrivateData->NameStringId[Index],
               PrivateData->NameValueName[Index],
               &BufferSize
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
/*++

  Routine Description:
    This function allows a caller to extract the current configuration for one
    or more named elements from the target driver.

  Arguments:
    This       - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
    Request    - A null-terminated Unicode string in <ConfigRequest> format.
    Progress   - On return, points to a character in the Request string.
                 Points to the string's null terminator if request was successful.
                 Points to the most recent '&' before the first failing name/value
                 pair (or the beginning of the string if the failure is in the
                 first name/value pair) if the request was not successful.
    Results    - A null-terminated Unicode string in <ConfigAltResp> format which
                 has all values filled in for the names in the Request string.
                 String to be allocated by the called function.

  Returns:
    EFI_SUCCESS           - The Results is filled with the requested values.
    EFI_OUT_OF_RESOURCES  - Not enough memory to store the results.
    EFI_INVALID_PARAMETER - Request is NULL, illegal syntax, or unknown name.
    EFI_NOT_FOUND         - Routing data doesn't match any storage in this driver.

--*/
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  DRIVER_SAMPLE_PRIVATE_DATA       *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       Value;
  UINTN                            ValueStrLen;
  CHAR16                           BackupChar;

  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
                  VariableName,
                  &mFormSetGuid,
                  NULL,
                  &BufferSize,
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Request == NULL) {
    //
    // Request is set to NULL, return all configurable elements together with ALTCFG
    //
    Status = ConstructConfigAltResp (
               NULL,
               NULL,
               Results,
               &mFormSetGuid,
               VariableName,
               PrivateData->DriverHandle[0],
               &PrivateData->Configuration,
               BufferSize,
               VfrMyIfrNVDataBlockName,
               2,
               STRING_TOKEN (STR_STANDARD_DEFAULT_PROMPT),
               VfrMyIfrNVDataDefault0000,
               STRING_TOKEN (STR_MANUFACTURE_DEFAULT_PROMPT),
               VfrMyIfrNVDataDefault0001
               );

    return Status;
  }

  //
  // Check if requesting Name/Value storage
  //
  if (EfiStrStr (Request, L"OFFSET") == NULL) {
    //
    // Check routing data in <ConfigHdr>.
    // Note: there is no name for Name/Value storage, only GUID will be checked
    //
    if (!IsConfigHdrMatch (Request, &mFormSetGuid, NULL)) {
      *Progress = Request;
      return EFI_NOT_FOUND;
    }

    //
    // Update Name/Value storage Names
    //
    Status = LoadNameValueNames (PrivateData);
    if (EFI_ERROR (Status)) {
      *Progress = Request;
      return Status;
    }

    //
    // Allocate memory for <ConfigResp>, e.g. Name0=0x11, Name1=0x1234, Name2="ABCD"
    // <Request>   ::=<ConfigHdr>&Name0&Name1&Name2
    // <ConfigResp>::=<ConfigHdr>&Name0=11&Name1=1234&Name2=0041004200430044
    //
    BufferSize = (EfiStrLen (Request) +
                  1 + sizeof (PrivateData->Configuration.NameValueVar0) * 2 +
                  1 + sizeof (PrivateData->Configuration.NameValueVar1) * 2 +
                  1 + sizeof (PrivateData->Configuration.NameValueVar2) * 2 + 1) * sizeof (CHAR16);
    *Results = EfiLibAllocateZeroPool (BufferSize);
    ASSERT (*Results != NULL);
    EfiStrCpy (*Results, Request);
    Value = *Results;

    //
    // Append value of NameValueVar0, type is UINT8
    //
    if ((Value = EfiStrStr (*Results, PrivateData->NameValueName[0])) != NULL) {
      Value += EfiStrLen (PrivateData->NameValueName[0]);
      ValueStrLen = ((sizeof (PrivateData->Configuration.NameValueVar0) * 2) + 1);
      EfiCopyMem (Value + ValueStrLen, Value, EfiStrSize (Value));

      *Value++ = L'=';
      BufferSize = 16;
      BackupChar = Value[ValueStrLen - 1];
      BufToHexString (Value, &BufferSize, (UINT8 *) &PrivateData->Configuration.NameValueVar0, sizeof (PrivateData->Configuration.NameValueVar0));
      Value[ValueStrLen - 1] = BackupChar;
    }

    //
    // Append value of NameValueVar1, type is UINT16
    //
    if ((Value = EfiStrStr (*Results, PrivateData->NameValueName[1])) != NULL) {
      Value += EfiStrLen (PrivateData->NameValueName[1]);
      ValueStrLen = ((sizeof (PrivateData->Configuration.NameValueVar1) * 2) + 1);
      EfiCopyMem (Value + ValueStrLen, Value, EfiStrSize (Value));

      *Value++ = L'=';
      BufferSize = 16;
      BackupChar = Value[ValueStrLen - 1];
      BufToHexString (Value, &BufferSize, (UINT8 *) &PrivateData->Configuration.NameValueVar1, sizeof (PrivateData->Configuration.NameValueVar1));
      Value[ValueStrLen - 1] = BackupChar;
    }

    //
    // Append value of NameValueVar2, type is CHAR16 *
    //
    if ((Value = EfiStrStr (*Results, PrivateData->NameValueName[2])) != NULL) {
      Value += EfiStrLen (PrivateData->NameValueName[2]);
      ValueStrLen = EfiStrLen (PrivateData->Configuration.NameValueVar2) * 4 + 1;
      EfiCopyMem (Value + ValueStrLen, Value, EfiStrSize (Value));

      *Value++ = L'=';
      //
      // Convert Unicode String to Config String, e.g. "ABCD" => "0041004200430044"
      //
      ValueStrLen = ValueStrLen * sizeof (CHAR16);
      Status = UnicodeToConfigString (Value, &ValueStrLen, PrivateData->Configuration.NameValueVar2);
    }

    Progress = (EFI_STRING *) Request + EfiStrLen (Request);
    return EFI_SUCCESS;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!IsConfigHdrMatch (Request, &mFormSetGuid, VariableName)) {
    *Progress = Request;
    return EFI_NOT_FOUND;
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = HiiConfigRouting->BlockToConfig (
                                HiiConfigRouting,
                                Request,
                                (UINT8 *) &PrivateData->Configuration,
                                BufferSize,
                                Results,
                                Progress
                                );
  return Status;
}

EFI_STATUS
EFIAPI
RouteConfig (
  IN  EFI_HII_CONFIG_ACCESS_PROTOCOL         *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
/*++

  Routine Description:
    This function processes the results of changes in configuration.

  Arguments:
    This          - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
    Configuration - A null-terminated Unicode string in <ConfigResp> format.
    Progress      - A pointer to a string filled in with the offset of the most
                    recent '&' before the first failing name/value pair (or the
                    beginning of the string if the failure is in the first
                    name/value pair) or the terminating NULL if all was successful.

  Returns:
    EFI_SUCCESS           - The Results is processed successfully.
    EFI_INVALID_PARAMETER - Configuration is NULL.
    EFI_NOT_FOUND         - Routing data doesn't match any storage in this driver.

--*/
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  DRIVER_SAMPLE_PRIVATE_DATA       *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  CHAR16                           *Value;
  CHAR16                           *StrPtr;
  CHAR16                           BackupChar;
  UINTN                            Length;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (
                  VariableName,
                  &mFormSetGuid,
                  NULL,
                  &BufferSize,
                  &PrivateData->Configuration
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if configuring Name/Value storage
  //
  if (EfiStrStr (Configuration, L"OFFSET") == NULL) {
    //
    // Check routing data in <ConfigHdr>.
    // Note: there is no name for Name/Value storage, only GUID will be checked
    //
    if (!IsConfigHdrMatch (Configuration, &mFormSetGuid, NULL)) {
      *Progress = Configuration;
      return EFI_NOT_FOUND;
    }

    //
    // Update Name/Value storage Names
    //
    Status = LoadNameValueNames (PrivateData);
    if (EFI_ERROR (Status)) {
      *Progress = Configuration;
      return Status;
    }

    //
    // Convert value for NameValueVar0
    //
    if ((Value = EfiStrStr (Configuration, PrivateData->NameValueName[0])) != NULL) {
      //
      // Skip "Name="
      //
      Value += EfiStrLen (PrivateData->NameValueName[0]);
      Value++;

      StrPtr = EfiStrStr (Value, L"&");
      if (StrPtr == NULL) {
        StrPtr = Value + EfiStrLen (Value);
      }
      BackupChar = *StrPtr;
      *StrPtr = L'\0';
      BufferSize = sizeof (PrivateData->Configuration.NameValueVar0);
      Status = HexStringToBuf ((UINT8 *) &PrivateData->Configuration.NameValueVar0, &BufferSize, Value, NULL);
      *StrPtr = BackupChar;
    }

    //
    // Convert value for NameValueVar1
    //
    if ((Value = EfiStrStr (Configuration, PrivateData->NameValueName[1])) != NULL) {
      //
      // Skip "Name="
      //
      Value += EfiStrLen (PrivateData->NameValueName[1]);
      Value++;

      StrPtr = EfiStrStr (Value, L"&");
      if (StrPtr == NULL) {
        StrPtr = Value + EfiStrLen (Value);
      }
      BackupChar = *StrPtr;
      *StrPtr = L'\0';
      BufferSize = sizeof (PrivateData->Configuration.NameValueVar1);
      Status = HexStringToBuf ((UINT8 *) &PrivateData->Configuration.NameValueVar1, &BufferSize, Value, NULL);
      *StrPtr = BackupChar;
    }

    //
    // Convert value for NameValueVar2
    //
    if ((Value = EfiStrStr (Configuration, PrivateData->NameValueName[2])) != NULL) {
      //
      // Skip "Name="
      //
      Value += EfiStrLen (PrivateData->NameValueName[2]);
      Value++;

      StrPtr = EfiStrStr (Value, L"&");
      if (StrPtr == NULL) {
        StrPtr = Value + EfiStrLen (Value);
      }
      BackupChar = *StrPtr;
      *StrPtr = L'\0';
      //
      // Convert Config String to Unicode String, e.g "0041004200430044" => "ABCD"
      //
      Length = sizeof (PrivateData->Configuration.NameValueVar2);
      Status = ConfigStringToUnicode (PrivateData->Configuration.NameValueVar2, &Length, Value);
      *StrPtr = BackupChar;
    }

    //
    // Store Buffer Storage back to EFI variable
    //
    Status = gRT->SetVariable(
                    VariableName,
                    &mFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (DRIVER_SAMPLE_CONFIGURATION),
                    &PrivateData->Configuration
                    );

    return Status;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!IsConfigHdrMatch (Configuration, &mFormSetGuid, VariableName)) {
    *Progress = Configuration;
    return EFI_NOT_FOUND;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               Configuration,
                               (UINT8 *) &PrivateData->Configuration,
                               &BufferSize,
                               Progress
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Store Buffer Storage back to EFI variable
  //
  Status = gRT->SetVariable(
                  VariableName,
                  &mFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (DRIVER_SAMPLE_CONFIGURATION),
                  &PrivateData->Configuration
                  );

  return Status;
}

EFI_STATUS
EFIAPI
DriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
/*++

  Routine Description:
    This function processes the results of changes in configuration.

  Arguments:
    This          - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
    Action        - Specifies the type of action taken by the browser.
    QuestionId    - A unique value which is sent to the original exporting driver
                    so that it can identify the type of data to expect.
    Type          - The type of value for the question.
    Value         - A pointer to the data being sent to the original exporting driver.
    ActionRequest - On return, points to the action requested by the callback function.

  Returns:
    EFI_SUCCESS          - The callback successfully handled the action.
    EFI_OUT_OF_RESOURCES - Not enough storage is available to hold the variable and its data.
    EFI_DEVICE_ERROR     - The variable could not be saved.
    EFI_UNSUPPORTED      - The specified Action is not supported by the callback.

--*/
{
  DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData;
  EFI_STATUS                      Status;
  EFI_HII_UPDATE_DATA             UpdateData;
  IFR_OPTION                      *IfrOptionList;
  UINT8                           MyVar;
  UINTN                           BufferSize;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  PrivateData = DRIVER_SAMPLE_PRIVATE_FROM_THIS (This);

  switch (QuestionId) {
  case 0x1234:
    //
    // Initialize the container for dynamic opcodes
    //
    IfrLibInitUpdateData (&UpdateData, 0x1000);

    IfrOptionList = EfiLibAllocatePool (2 * sizeof (IFR_OPTION));
    ASSERT (IfrOptionList != NULL);

    IfrOptionList[0].Flags        = 0;
    IfrOptionList[0].StringToken  = STRING_TOKEN (STR_BOOT_OPTION1);
    IfrOptionList[0].Value.u8     = 1;
    IfrOptionList[1].Flags        = EFI_IFR_OPTION_DEFAULT;
    IfrOptionList[1].StringToken  = STRING_TOKEN (STR_BOOT_OPTION2);
    IfrOptionList[1].Value.u8     = 2;

    CreateActionOpCode (
      0x1237,                           // Question ID
      STRING_TOKEN(STR_EXIT_TEXT),      // Prompt text
      STRING_TOKEN(STR_EXIT_TEXT),      // Help text
      EFI_IFR_FLAG_CALLBACK,            // Question flag
      0,                                // Action String ID
      &UpdateData                       // Container for dynamic created opcodes
      );

    //
    // Prepare initial value for the dynamic created oneof Question
    //
    PrivateData->Configuration.DynamicOneof = 2;
    Status = gRT->SetVariable(
                    VariableName,
                    &mFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (DRIVER_SAMPLE_CONFIGURATION),
                    &PrivateData->Configuration
                    );

    //
    // Set initial vlaue of dynamic created oneof Question in Form Browser
    //
    BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
    Configuration = EfiLibAllocateZeroPool (sizeof (DRIVER_SAMPLE_CONFIGURATION));
    ASSERT (Configuration != NULL);
    Status = GetBrowserData (&mFormSetGuid, VariableName, &BufferSize, (UINT8 *) Configuration);
    if (!EFI_ERROR (Status)) {
      Configuration->DynamicOneof = 2;

      //
      // Update uncommitted data of Browser
      //
      SetBrowserData (
        &mFormSetGuid,
        VariableName,
        sizeof (DRIVER_SAMPLE_CONFIGURATION),
        (UINT8 *) Configuration,
        NULL
        );
    }
    gBS->FreePool (Configuration);

    CreateOneOfOpCode (
      0x8001,                           // Question ID (or call it "key")
      CONFIGURATION_VARSTORE_ID,        // VarStore ID
      DYNAMIC_ONE_OF_VAR_OFFSET,        // Offset in Buffer Storage
      STRING_TOKEN (STR_ONE_OF_PROMPT), // Question prompt text
      STRING_TOKEN (STR_ONE_OF_HELP),   // Question help text
      EFI_IFR_FLAG_CALLBACK,            // Question flag
      EFI_IFR_NUMERIC_SIZE_1,           // Data type of Question Value
      IfrOptionList,                    // Option list
      2,                                // Number of options in Option list
      &UpdateData                       // Container for dynamic created opcodes
      );

    CreateOrderedListOpCode (
      0x8002,                           // Question ID
      CONFIGURATION_VARSTORE_ID,        // VarStore ID
      DYNAMIC_ORDERED_LIST_VAR_OFFSET,  // Offset in Buffer Storage
      STRING_TOKEN (STR_BOOT_OPTIONS),  // Question prompt text
      STRING_TOKEN (STR_BOOT_OPTIONS),  // Question help text
      EFI_IFR_FLAG_RESET_REQUIRED,      // Question flag
      0,                                // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
      EFI_IFR_NUMERIC_SIZE_1,           // Data type of Question value
      5,                                // Maximum container
      IfrOptionList,                    // Option list
      2,                                // Number of options in Option list
      &UpdateData                       // Container for dynamic created opcodes
      );

    CreateGotoOpCode (
      1,                                // Target Form ID
      STRING_TOKEN (STR_GOTO_FORM1),    // Prompt text
      STRING_TOKEN (STR_GOTO_HELP),     // Help text
      0,                                // Question flag
      0x8003,                           // Question ID
      &UpdateData                       // Container for dynamic created opcodes
      );

    Status = IfrLibUpdateForm (
               PrivateData->HiiHandle[0],  // HII handle
               &mFormSetGuid,              // Formset GUID
               0x1234,                     // Form ID
               0x1234,                     // Label for where to insert opcodes
               TRUE,                       // Append or replace
               &UpdateData                 // Dynamic created opcodes
               );
    gBS->FreePool (IfrOptionList);
    IfrLibFreeUpdateData (&UpdateData);
    break;

  case 0x5678:
    //
    // We will reach here once the Question is refreshed
    //
    IfrLibInitUpdateData (&UpdateData, 0x1000);

    IfrOptionList = EfiLibAllocatePool (2 * sizeof (IFR_OPTION));
    ASSERT (IfrOptionList != NULL);

    CreateActionOpCode (
      0x1237,                           // Question ID
      STRING_TOKEN(STR_EXIT_TEXT),      // Prompt text
      STRING_TOKEN(STR_EXIT_TEXT),      // Help text
      EFI_IFR_FLAG_CALLBACK,            // Question flag
      0,                                // Action String ID
      &UpdateData                       // Container for dynamic created opcodes
      );

    Status = IfrLibUpdateForm (
               PrivateData->HiiHandle[0],  // HII handle
               &mFormSetGuid,              // Formset GUID
               3,                          // Form ID
               0x2234,                     // Label for where to insert opcodes
               TRUE,                       // Append or replace
               &UpdateData                 // Dynamic created opcodes
               );
    IfrLibFreeUpdateData (&UpdateData);

    //
    // Refresh the Question value
    //
    PrivateData->Configuration.DynamicRefresh++;
    Status = gRT->SetVariable(
                    VariableName,
                    &mFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (DRIVER_SAMPLE_CONFIGURATION),
                    &PrivateData->Configuration
                    );

    //
    // Change an EFI Variable storage (MyEfiVar) asynchronous, this will cause
    // the first statement in Form 3 be suppressed
    //
    MyVar = 111;
    Status = gRT->SetVariable(
                    L"MyVar",
                    &mFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    1,
                    &MyVar
                    );
    break;

  case 0x1237:
    //
    // User press "Exit now", request Browser to exit
    //
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
    break;

  case 0x2000:
    //
    // When try to set a new password, user will be chanlleged with old password.
    // The Callback is responsible for validating old password input by user,
    // If Callback return EFI_SUCCESS, it indicates validation pass.
    //
    switch (PrivateData->PasswordState) {
    case BROWSER_STATE_VALIDATE_PASSWORD:
      Status = ValidatePassword (PrivateData, Value->string);
      if (Status == EFI_SUCCESS) {
        PrivateData->PasswordState = BROWSER_STATE_SET_PASSWORD;
      }
      break;

    case BROWSER_STATE_SET_PASSWORD:
      Status = SetPassword (PrivateData, Value->string);
      PrivateData->PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;
      break;

    default:
      break;
    }

    break;

  default:
    break;
  }

  return Status;
}

EFI_STATUS
EFIAPI
DriverSampleInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      SavedStatus;
  EFI_HII_PACKAGE_LIST_HEADER     *PackageList;
  EFI_HII_HANDLE                  HiiHandle[2];
  EFI_HANDLE                      DriverHandle[2];
  DRIVER_SAMPLE_PRIVATE_DATA      *PrivateData;
  EFI_SCREEN_DESCRIPTOR           Screen;
  EFI_HII_DATABASE_PROTOCOL       *HiiDatabase;
  EFI_HII_STRING_PROTOCOL         *HiiString;
  EFI_FORM_BROWSER2_PROTOCOL      *FormBrowser2;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  CHAR16                          *NewString;
  UINTN                           BufferSize;
  DRIVER_SAMPLE_CONFIGURATION     *Configuration;
  BOOLEAN                         ExtractIfrDefault;

  //
  // Initialize the library and our protocol.
  //
  EfiInitializeDriverLib (ImageHandle, SystemTable);

  //
  // Initialize screen dimensions for SendForm().
  // Remove 3 characters from top and bottom
  //
  EfiZeroMem (&Screen, sizeof (EFI_SCREEN_DESCRIPTOR));
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &Screen.RightColumn, &Screen.BottomRow);

  Screen.TopRow     = 3;
  Screen.BottomRow  = Screen.BottomRow - 3;

  //
  // Initialize driver private data
  //
  PrivateData = EfiLibAllocatePool (sizeof (DRIVER_SAMPLE_PRIVATE_DATA));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->Signature = DRIVER_SAMPLE_PRIVATE_SIGNATURE;

  PrivateData->ConfigAccess.ExtractConfig = ExtractConfig;
  PrivateData->ConfigAccess.RouteConfig = RouteConfig;
  PrivateData->ConfigAccess.Callback = DriverCallback;
  PrivateData->PasswordState = BROWSER_STATE_VALIDATE_PASSWORD;

  //
  // Locate Hii Database protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, &HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiDatabase = HiiDatabase;

  //
  // Locate HiiString protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, &HiiString);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiString = HiiString;

  //
  // Locate Formbrowser2 protocol
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, &FormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->FormBrowser2 = FormBrowser2;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, &HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiConfigRouting = HiiConfigRouting;

  //
  // Install Config Access protocol
  //
  Status = CreateHiiDriverHandle (&DriverHandle[0]);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->DriverHandle[0] = DriverHandle[0];

  Status = gBS->InstallProtocolInterface (
                  &DriverHandle[0],
                  &gEfiHiiConfigAccessProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &PrivateData->ConfigAccess
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  PackageList = PreparePackageList (
                  2,
                  &mFormSetGuid,
                  DriverSampleStrings,
                  VfrBin
                  );
  if (PackageList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HiiDatabase->NewPackageList (
                          HiiDatabase,
                          PackageList,
                          DriverHandle[0],
                          &HiiHandle[0]
                          );
  gBS->FreePool (PackageList);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiHandle[0] = HiiHandle[0];

  //
  // Publish another Fromset
  //
  Status = CreateHiiDriverHandle (&DriverHandle[1]);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->DriverHandle[1] = DriverHandle[1];

  PackageList = PreparePackageList (
                  2,
                  &mInventoryGuid,
                  DriverSampleStrings,
                  InventoryBin
                  );
  if (PackageList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HiiDatabase->NewPackageList (
                          HiiDatabase,
                          PackageList,
                          DriverHandle[1],
                          &HiiHandle[1]
                          );
  gBS->FreePool (PackageList);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PrivateData->HiiHandle[1] = HiiHandle[1];

  //
  // Very simple example of how one would update a string that is already
  // in the HII database
  //
  NewString = L"700 Mhz";

  Status = IfrLibSetString (HiiHandle[0], STRING_TOKEN (STR_CPU_STRING2), NewString);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize Name/Value name String ID
  //
  PrivateData->NameStringId[0] = STR_NAME_VALUE_VAR_NAME0;
  PrivateData->NameStringId[1] = STR_NAME_VALUE_VAR_NAME1;
  PrivateData->NameStringId[2] = STR_NAME_VALUE_VAR_NAME2;

  //
  // Initialize configuration data
  //
  Configuration = &PrivateData->Configuration;
  EfiZeroMem (Configuration, sizeof (DRIVER_SAMPLE_CONFIGURATION));

  //
  // Try to read NV config EFI variable first
  //
  ExtractIfrDefault = TRUE;
  BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
  Status = gRT->GetVariable (VariableName, &mFormSetGuid, NULL, &BufferSize, Configuration);
  if (!EFI_ERROR (Status) && (BufferSize == sizeof (DRIVER_SAMPLE_CONFIGURATION))) {
    ExtractIfrDefault = FALSE;
  }

  if (ExtractIfrDefault) {
    //
    // EFI variable for NV config doesn't exit, we should build this variable
    // based on default values stored in IFR
    //
    BufferSize = sizeof (DRIVER_SAMPLE_CONFIGURATION);
    Status = ExtractDefault (Configuration, &BufferSize, 1, VfrMyIfrNVDataDefault0000);

    if (!EFI_ERROR (Status)) {
      gRT->SetVariable(
             VariableName,
             &mFormSetGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
             sizeof (DRIVER_SAMPLE_CONFIGURATION),
             Configuration
             );
    }
  }

  //
  // Example of how to display only the item we sent to HII
  //
  if (DISPLAY_ONLY_MY_ITEM == 0x0001) {
    //
    // Have the browser pull out our copy of the data, and only display our data
    //
    //    Status = FormConfig->SendForm (FormConfig, TRUE, HiiHandle, NULL, NULL, NULL, &Screen, NULL);
    //
    Status = FormBrowser2->SendForm (
                             FormBrowser2,
                             HiiHandle,
                             1,
                             NULL,
                             0,
                             NULL,
                             NULL
                             );
    SavedStatus = Status;

    Status = HiiDatabase->RemovePackageList (HiiDatabase, HiiHandle[0]);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = HiiDatabase->RemovePackageList (HiiDatabase, HiiHandle[1]);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    return SavedStatus;
  } else {
    //
    // Have the browser pull out all the data in the HII Database and display it.
    //
    //    Status = FormConfig->SendForm (FormConfig, TRUE, 0, NULL, NULL, NULL, NULL, NULL);
    //
  }

  return EFI_SUCCESS;
}
