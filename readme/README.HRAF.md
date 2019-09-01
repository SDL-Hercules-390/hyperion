![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Host Resource Access Facility

## Contents

1. [About](#About)
2. [General Register Usage](#General-Register-Usage)
3. [Enabling the Host Resouce Access Facility](#Enabling-the-Host-Resouce-Access-Facility)
4. [DIAGNOSE X'F18' Subcodes](#DIAGNOSE-XF18-Subcodes)
5. [Documentation](#Documentation)

## About

The Host Resource Access Facility (HRAF) provides direct access to host resources by a Hercules guest program operating in privileged operation state via a Hercules supplied DIAGNOSE code X'F18'.

DIAGNOSE X'F18' utilizes a set of subcodes for selection of the type of resource and the mechanism in use by the program.  Additionally, a subcode allows the DIAGNOSE X'F18' functionality to be queried for the capabilitites provided.  As the functionality is enhanced over time, the query subcode will report a new version number.

## General Register Usage

```
  DIAGNOSE  Rx,Ry,X'F18'
```

The first operand of the DIAGNOSE, Rx, must be an even-odd pair of registers.  The second operand, Ry, may be any register.  The contents of the registers are:

  Register  |  Description
------------| -------------------------------------------------
&nbsp; &nbsp; Rx        | Contains the subcode, a 32-bit unsigned value.
&nbsp; &nbsp; Rx+1      | Contains the operational options used by the DIAGNOSE, if required by the subcode
&nbsp; &nbsp; Ry        | Contains the real address of the subcode's parameter block.  All subcodes require a parameter block.

Two mechanisms are provided for accessing host resources: _**compatibility mode**_ and _**native mode**_. The primary difference between the two modes is the structure and size of the parameter block associated with the host resource being accessed.

_Compatibility mode_ is intended to ease the migration from the original Jason Winter general instructions to the privileged DIAGNOSE code.  The compatibility parameter block is essentially a 32-bit register save area corresponding to the values that would have been encountered had the Jason Winter general instruction been used.

The _native mode_ parameter block will use a different format allowing functional extensions and compatibility mode limitations to be eliminated.


## Enabling the Host Resouce Access Facility

By default, the Host Resource Access Facility is disabled.  Use the `archlvl` command to enable the facility:

```
  archlvl   enable  HERC_HOST_RESOURCE_ACCESS
```

## DIAGNOSE X'F18' Subcodes

The following subcodes are supported or planned.  The DIAGNOSE version in which the subcode has been made available is identified.  A '-' indicates a future planned subcode.  


   Subcode   |        Version     |  Description
------------ | ------------------ | ----------------------------------------------------
X'00000000'  | 1 |  Query DIAGNOSE X'F18' available functionality
X'00000001'  | - |  Perform a compatibility mode host socket function
X'00000002'  | 1 |  Perform a compatibility mode host file system operation
X'00000003'  | - |  Perform a native mode host socket function
X'00000004'  | - |  Perform a native mode host file system operation


### Subcode X'00000000' - Query DIAGNOSE X'F18'

The query subcode provides the following information in its response parameter block:

 *  The version of DIAGNOSE X'F18' provided by the executing Hercules emulator
 *  Available facilities and modes of operation
 *  Whether large file system support is available
 *  The host platform to which resource access is being provided
 *  Addressing modes supported
 *  Additional information specific to subcodes.


### Subcode X'00000001' - Compatibility Mode TCP Socket Access

Planned for future implementation, incorporates functionality comparable to that offered by the `dyn75` package Hercules module.


### Subcode X'00000002' - Compatibility Mode File System Access

The following operations are provided, as identified by the operation field of the parameter block:


  Operation        |  Description
-------------------| -------------------------------------------------
0 |  Rename a file
1 |  Unlink the file (delete)
2 |  Open a file
3 |  Close a file based upon it's name
4 |  Read from a file in either binary or text modes
5 |  Write to a file in either binary or text modes
6 |  Seek to a position in a file
7 |  Commit any pending writes to the file
8 |  Close a file using its file handle
9 |  Set the file read/write mode to binary or text


### Subcode X'00000002' - Limitations

Compatibility mode access to the host file system is restricted in the following ways:

 * Large file system support is not available.
 * Use of subcode X'00000002' in z/Architecture mode requires the current PSW addressing mode to be set to either 24- or 31-bit before execution of DIAGNOSE X'F18', otherwise a specification exception occurs.
 * Dynamic Address Translation using secondary space, home space or access register modes are not available.  Only DAT-off and DAT-on primary-space modes are supported, as specified in the operation options parameter in the odd register of the pair.


### Subcode X'00000003' - Native Mode TCP Socket Access

While eliminating limitations that may exist in the compatibility mode TCP socket
access, the native mode subcode is expected to implement IPv6 capability as well as
IPv4.  This subcode is planned for future implementation.


### Subcode X'00000004' - Native Mode File System Access

Primarily intended to eliminate some or all of the restrictions identified above
for the compatibility mode subcode.


## Documentation

This file is intended to provide general information about the Host Resource Access Facility and is not intended to provide detaled information about DIAGNOSE X'F18'. Until this documentation is made available by the Hercules project, contact the author on the main Hercules email list for access.


## Giving credit where credit is due

Jason Winter developed the original support providing users access to host facilities.  This support for many years has been provided as externally loaded functionality into Hercules as two different packages: `dyn76`, for access to the host file system, and `dyn75`, for access to host TCP sockets.

The compatibility functionality offered by the Host Resource Access Facility is based upon Jason Winter's original code, the licensing to which has been generously provided by Jason to the Hercules community.
See file [`license_dyn76.txt`](../license_dyn76.txt) for the licensing of the Hercules host file system functionality provided by the `dyn76` package.  Currently, Hercules has only incorporated the `dyn76` Hercules functionality.
`dyn75` is planned for future Hercules incorporation.  Thank you, Jason Winter.

-- Harold Grovesteen  
Apr 23, 2011
