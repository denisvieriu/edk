#/*++
#
#  Copyright (c)  2006 - 2007, Intel Corporation                                                         
#  All rights reserved. This program and the accompanying materials                          
#  are licensed and made available under the terms and conditions of the BSD License         
#  which accompanies this distribution.  The full text of the license may be found at        
#  http://opensource.org/licenses/bsd-license.php                                            
#                                                                                            
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#  
#  Module Name:
#  
#    Makefile
#  
#  Abstract:
#   
#    This makefile is used to build different targets.
#   
#--*/

#
# Everything depends on EDK_SOURCE & EDK_TOOLS_PATH. Make sure it's defined
#
!IFNDEF EDK_SOURCE
!ERROR EDK_SOURCE environmental variable not set 
!ENDIF

!IFNDEF EDK_TOOLS_PATH
!MESSAGE EDK_TOOLS_PATH environmental variable not set, default setting used
!ENDIF

######################################################################################
#    Default build target
######################################################################################
all : uefi64

######################################################################################
#    Feature flags for target efi32
######################################################################################
BUILD_DIR = efi32

$(BUILD_DIR)dir : Pseudotargets
    @if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    @copy Build\*.* $(BUILD_DIR)
    @if exist $(BUILD_DIR)\Config.env del /f $(BUILD_DIR)\Config.env
    @findstr /V "UEFI_MODE PI_MODE BUILD_TARGET_DIRECTORY EFI_BUILD_TARGET_X64" Build\Config.env > $(BUILD_DIR)\Config.env
    @echo UEFI_MODE                      = NO                  >> $(BUILD_DIR)\Config.env
    @echo PI_MODE                        = NO                  >> $(BUILD_DIR)\Config.env
    @echo EFI_BUILD_TARGET_X64           = NO                  >> $(BUILD_DIR)\Config.env
    @echo BUILD_TARGET_DIRECTORY         = $(BUILD_DIR)        >> $(BUILD_DIR)\Config.env

$(BUILD_DIR) : $(BUILD_DIR)dir
    @cd $(BUILD_DIR)
    @nmake all
    @cd ..

######################################################################################
#    Feature flags for target efi64
######################################################################################
BUILD_DIR = efi64

$(BUILD_DIR)dir : Pseudotargets
    @if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    @copy Build\*.* $(BUILD_DIR)
    @if exist $(BUILD_DIR)\Config.env del /f $(BUILD_DIR)\Config.env
    @findstr /V "UEFI_MODE PI_MODE BUILD_TARGET_DIRECTORY EFI_BUILD_TARGET_X64" Build\Config.env > $(BUILD_DIR)\Config.env
    @echo UEFI_MODE                      = NO                  >> $(BUILD_DIR)\Config.env
    @echo PI_MODE                        = NO                  >> $(BUILD_DIR)\Config.env
    @echo EFI_BUILD_TARGET_X64           = YES                 >> $(BUILD_DIR)\Config.env
    @echo BUILD_TARGET_DIRECTORY         = $(BUILD_DIR)        >> $(BUILD_DIR)\Config.env

$(BUILD_DIR) : $(BUILD_DIR)dir
    @cd $(BUILD_DIR)
    @nmake all
    @cd ..

######################################################################################
#    Feature flags for target uefi32
######################################################################################
BUILD_DIR = uefi32

$(BUILD_DIR)dir : Pseudotargets
    @if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    @copy Build\*.* $(BUILD_DIR)
    @if exist $(BUILD_DIR)\Config.env del /f $(BUILD_DIR)\Config.env
    @findstr /V "UEFI_MODE PI_MODE BUILD_TARGET_DIRECTORY EFI_BUILD_TARGET_X64" Build\Config.env > $(BUILD_DIR)\Config.env
    @echo UEFI_MODE                      = YES                 >> $(BUILD_DIR)\Config.env
    @echo PI_MODE                        = YES                  >> $(BUILD_DIR)\Config.env
    @echo EFI_BUILD_TARGET_X64           = NO                  >> $(BUILD_DIR)\Config.env
    @echo BUILD_TARGET_DIRECTORY         = $(BUILD_DIR)        >> $(BUILD_DIR)\Config.env

$(BUILD_DIR) : $(BUILD_DIR)dir
    @cd $(BUILD_DIR)
    @nmake all
    @cd ..

######################################################################################
#    Feature flags for target uefi64
######################################################################################
BUILD_DIR = uefi64

$(BUILD_DIR)dir : Pseudotargets
    @if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    @copy Build\*.* $(BUILD_DIR)
    @if exist $(BUILD_DIR)\Config.env del /f $(BUILD_DIR)\Config.env
    @findstr /V "UEFI_MODE PI_MODE BUILD_TARGET_DIRECTORY EFI_BUILD_TARGET_X64" Build\Config.env > $(BUILD_DIR)\Config.env
    @echo UEFI_MODE                      = YES                 >> $(BUILD_DIR)\Config.env
    @echo PI_MODE                        = YES                 >> $(BUILD_DIR)\Config.env
    @echo EFI_BUILD_TARGET_X64           = YES                 >> $(BUILD_DIR)\Config.env
    @echo BUILD_TARGET_DIRECTORY         = $(BUILD_DIR)        >> $(BUILD_DIR)\Config.env

$(BUILD_DIR) : $(BUILD_DIR)dir
    @cd $(BUILD_DIR)
    @nmake all
    @cd ..

Pseudotargets:

efi32clean:
    @cd efi32
    @nmake clean
    @cd ..

efi64clean:
    @cd efi64
    @nmake clean
    @cd ..

uefi32clean:
    @cd uefi32
    @nmake clean
    @cd ..

uefi64clean:
    @cd uefi64
    @nmake clean
    @cd ..

efi32cleanall:
    @cd efi32
    @nmake cleanall
    @cd ..

efi64cleanall:
    @cd efi64
    @nmake cleanall
    @cd ..

uefi32cleanall:
    @cd uefi32
    @nmake cleanall
    @cd ..

uefi64cleanall:
    @cd uefi64
    @nmake cleanall
    @cd ..

clean:
    - @if exist efi32 nmake efi32clean
    - @if exist efi64 nmake efi64clean
    - @if exist uefi32 nmake uefi32clean
    - @if exist uefi64 nmake uefi64clean

cleanall:
    - @if exist efi32 nmake efi32cleanall
    - @rd /s /q efi32 > NUL 2>&1
    - @if exist efi64 nmake efi64cleanall
    - @rd /s /q efi64 > NUL 2>&1
    - @if exist uefi32 nmake uefi32cleanall
    - @rd /s /q uefi32 > NUL 2>&1
    - @if exist uefi64 nmake uefi64cleanall
    - @rd /s /q uefi64 > NUL 2>&1
