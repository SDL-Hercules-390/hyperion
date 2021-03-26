![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# ECPS:VM : Extended Control Program Support : VM/370
and
# Extended VM Assists - Partial Privop Simulation And Virtual Interval Timer

## Contents

1. [Change Log](#Change-Log)
2. [Supported Systems](#Supported-Systems)
3. [How to enable/disable VM Assists](#How-to-enabledisable-VM-Assists)
4. [New panel command](#New-panel-command)
5. [Determining if the assist is used by VM](#Determining-if-the-assist-is-used-by-VM)
6. [Technical information](#Technical-information)
7. [Troubleshooting](#Troubleshooting)
8. [Implemented Assists](#Implemented-Assists)
9. [Special Situations](#Special-Situations)
10. [Example 'ecpsvm stat' report](#Example-ecpsvm-stat-report)

## Change Log

```
03/25/21 : Bug fix in DISP2 for floating point users
         ; removed support for SIO and DIAG assists due to incompatibilities.
08/06/18 : Updated supported functions to include DIAG and STCTL instruction assists
         ; added support for CP assist LCSPG.
04/12/17 : Updated supported functions to include LRA instruction assist
         ; fixed DISP0 assist issue where VMPSWAIT was not being set.
02/18/17 : Updated supported functions to include CCW translation assists DFCCW,DNCCW,CCWGN, and CCW untranslation assists UXCCW, FCCWS.
02/10/17 : Updated supported functions to include SIO/SIOF assist, CP LINK/RETURN (SVC 8/12) assist.
02/05/17 : Changed description for configuration and commands for ECPSVM YES TRAP/NOTRAP
         ; updated relevant text
         ; updated supported features list
         ; sample output.
07/07/03 : Changed description for configuration and commands
           ECPS:VM changed to ECPSVM (configuration)
           evm changed to ecpsvm     (command)
```

## Supported Systems

Affected operating systems:
  **VM/370 Release 6**  (PTFs required - PLC 029 works fine)
up to:
  **VM/SP 6**  (with or without HPO option)

VM/XA SF, VM/XA SP, VM/ESA and z/VM do _**not**_ use these Assists, but instead rely on the SIE instruction to perform some of these functions.

A VM/SP Guest (or VM/370 Guest with 4K Storage key updates) running under [z/]VM[/[XA|ESA]] will _**not**_ have access to either the CP assists or VM Assists.
The ECPS:VM Feature is disabled when running under SIE.

## How to enable/disable VM Assists

In the `HERCULES.CNF` file within the configuration section, insert one of the following statements:
<pre>
    ECPSVM LEVEL n               Where n is the requested level (see CAUTION below!)
    ECPSVM NO                    Assist not enabled (default)
    ECPSVM YES [ <u>TRAP</u>   ]        Assist enabled with CP FREE/FRET trap support
               [ NOTRAP ]        Assist enabled without FREE/FRET trap support
</pre>

If `YES` is specified, the most appropriate level is returned (Level 20).
If `TRAP` or `NOTRAP` is not specified with `ECPSVM YES`, then **`TRAP`** is the default.
Refer to the Special Situations section further below for more information about TRAP/NOTRAP.

NOTE: The ECPSVM LEVEL 'n' level number doesn't affect the operations of the assist but rather only defines what level number is reported to the program.

#### CAUTION!!

_Use of the `LEVEL n` form is **NOT** recommended, and is only provided for engineering use!_

## New panel command
`ecpsvm`

    ecpsvm help                       Summary of these commands

    ecpsvm stats                      Shows ECPS:VM Call/Hit statistics
                                      (example further below)

    ecpsvm enable  feature            Enable named feature
    ecpsvm disable feature            Disable named feature

    ecpsvm level [nn]                 Force ECPS:VM to report a certain support level
                                      (or display the current support level)

    ecpsvm debug   [feature]          Turn on debugging messages for a specific
                                      feature, where feature is either ALL (default),
                                      CPASSIST, VMASSIST, or one of the many named
                                      features listed further below, e.g. SCNRU,
                                      LCTL, etc

    ecpsvm nodebug [feature]          Turn off debugging messages

**NOTE:** `ecpsvm disable` does _not_ entirely disable all CP ASSISTS. If it did (i.e. generate a program interrupt whenever a E6xx instruction is invoked) then VM would abend immediately. Rather, omit the ECPSVM statement altogether from the configuration file.

To determine the feature names, type `ecpsvm enable ALL`.  All of the enabled features will then be listed.

The `ecpsvm` command is _not_ case sensitive.

## Determining if the assist is used by VM

Use the following two CLASS A commands:
  
    CP QUERY CPASSIST
    CP QUERY SASSIST

Both queries should return `ON`.

Also use the following CLASS G Command:  

    CP QUERY SET

The second line should indicate:

    ASSIST ON SVC TMR

## Technical information

The CP Assists provides the VM System Control Program (SCP) with various microcoded instructions to shorten the supervisor pathlength. All microcoded instructions are privileged instructions and have an opcode of E6xx. They are native representation of what the SCP would do in a similar case. For all cases where the assist is not able to correctly assist the SCP, the E6xx instructions resolve to a no-op, thus leaving the responsibility of the task to the original CP code.

The VM Assists alter the behavior of certain privileged instructions when executed in problem state (controlled by the Problem State bit in the PSW). The VM Assist will either completely simulate the instruction (when feasible), branch directly to the CP support module for that instruction (thereby bypassing program interruption processing and instruction decoding), or generate a privileged operation exception program interruption for all other cases so that CP can provide the simulation.

The VM Virtual Interval Timer assist allows updating of a Virtual Machine virtual interval timer directly by the microcode.

Both CP And VM Assists are controlled by real Control Register 6 which controls the availability and behavior of the assists.

## Troubleshooting

In the event that a certain CP or VM Assist disrupts normal operations, it is possible to selectively disable each discrete component. The best method is to disable _**all**_ VM and CP Assists (except STEVL and SSM if done prior to IPL) and then to enable each feature until the problem occurs. If it is unknown whether the problem lies in the VM or CP Assist, it is also possible to enable/disable the entire group of assists.

Refer to the `ECPSVM ENABLE|DISABLE` command documented further above.

`ECPSVM STATS` allows you to see how often each assist is invoked. The hit count and hit ratio makes it possible to determine how effective the assists are.

A low hit ratio may be normal in some situations. For example, the LPSW hit ratio will be very low when running VM under VM, because most PSW switches cannot be resolved by the assist.

A low invocation count simply shows that the related assist is not often issued. For example, there are very few `LCTL`s when running CMS.

Some assists are just invoked once at IPL (STEVL). This is normal behavior.

## Implemented Assists

### CP ASSISTS:

- [x]    DFCCW, DNCCW, CCWGN     CCW translation
- [x]    DISP0, DISP1, DISP2     CP Dispatching
- [x]    FREEX, FRETX            CP Free Storage management
- [ ]    FREE/FRET       Original CP Free Storage Management (up to level 19)
- [x]    LCKPG, ULKPG            Real page frame locking/unlocking
- [x]    LCSPG                   Locate changed shared page
- [x]    LINK, RETRN             CP SVC 8/12 LINK and RETURN function
- [ ]    PMASS           Preferred Machine Assist (insufficient information)
- [x]    SCNRU, SCNVU            Real/Virtual Device control block scan
- [x]    STEVL                   Store ECPS:VM support level
- [x]    TRBRG, TRLCK            Virtual frame addressing/locking
- [x]    VIST,  VIPT             Invalidate shadow segment/page tables
- [x]    UXCCW, FCCWS            CCW untranslation

### VM ASSISTS:

- [ ]    DIAG Simulation
- [ ]    IUCV
- [ ]    ISK/SSK/ISKE/SSKE/IVSK    Extended Key Operations assist
- [x]    LCTL Simulation
- [x]    LPSW Simulation
- [x]    LRA Simulation
- [ ]    SIO/SIOF Simulation
- [x]    SSM Simulation
- [x]    SVC Simulation
- [x]    STNSM Simulation
- [x]    STOSM Simulation
- [ ]    V=R Shadow Table Bypass assists [note]
- [x]    Virtual Interval Timer

**Note:** The V=R Shadow Table Bypass assist is a feature which requires the guest program to be aware of the feature (Page 0 Relocation).

## Special Situations

1.  ECPS:VM will _not_ work in an AP or MP system. An AP or MP generated system locks various system control blocks being manipulated by the assisted functions (`VMBLOK`, `RDEVBLOK`, `VDEVBLOK`, etc..). However, the current ECPS:VM implementation doesn't lock any of those structures.

Therefore, CP will fairly quickly abend because it will find some of the control blocks to not have been locked when they should, resulting in various `LOKxxx` abends.  Consequently, ECPS:VM _**must**_ be disabled when a AP or MP system is used.

2.  Many users that run VM/370 are also using a diagnostic tool called the FREE/FRET trap.  This tool is used to help diagnose problems with CP's management of free storage.  The various builds of VM in use by emulator users typically have the trap already in effect.  However, ECPS:VM normally cannot perform several of the CP Assists when the trap is in effect and the affected assists are turned off.  Unfortunately, the assists that are turned off are otherwise highly used and losing the assist capability for these functions is a significant performance impact.

This updated version of the ECPS:VM support provides the capability to operate with or without the FREE/FRET trap in effect.  The assist can automatically determine at IPL time whether it needs to operate with the trap or without the trap.  If the trap is enabled, all of the supported assists listed above are enabled.

It is still possible to use ECPS:VM without any additional support for the FREE/FRET trap by simply specifying `ECPSVM YES NOTRAP` in the Hercules configuration file.  In this case, the assists that cannot operate with the trap in effect will automatically be disabled as before.  This means that the `DISP1`, `DISP2`, `FREEX`, and `FRETX` assists will be disabled.  If the trap is not present, all assists are enabled and `NOTRAP` has no meaning.

Otherwise, regardless of whether the FREE/FRET trap is present, ECPS:VM will attempt to operate with all assists enabled with or without the trap when specifying `ECPSVM YES` or `ECPSVM YES TRAP`. (`ECPSVM YES` and `ECPSVM YES TRAP` are functionally equivalent.)

You can determine if ECPS:VM is operating with the trap after IPL by issuing the `ecpsvm stats` command at the Hercules console.  If the trap is in effect and ECPS:VM has recognized it, the following message will also be displayed at the beginning of the normal stats report:

`ECPS:VM Operating with CP FREE/FRET trap in effect`


## Example 'ecpsvm stat' report
```
15:04:36 HHC01603I ecpsvm stat
15:04:36 HHC01719I ECPS:VM Command processor invoked
14:04:36 HHC01724I ECPS:VM Operating with CP FREE/FRET trap in effect
15:04:36 HHC01725I ECPS:VM Code version 1.88
15:04:36 HHC01702I +-----------+------------+------------+-------+
15:04:36 HHC01706I | VM ASSIST |    Calls   |     Hits   | Ratio |
15:04:36 HHC01702I +-----------+------------+------------+-------+
15:04:36 HHC01701I | LRA       |    2366260 |    2366260 |  100% |
15:04:36 HHC01701I | STNSM     |    1844353 |    1839880 |   99% |
15:04:36 HHC01701I | STOSM     |    1110753 |    1036311 |   93% |
15:04:36 HHC01701I | LPSW      |     706882 |     694282 |   98% |
15:04:36 HHC01701I | LCTL      |     699101 |     245626 |   35% |
15:04:36 HHC01701I | SVC       |     258185 |     258181 |   99% |
15:04:36 HHC01701I | SSM       |     209426 |     209228 |   99% |
15:04:36 HHC01701I | STCTL     |     149228 |     149228 |  100% |
15:04:36 HHC01702I +-----------+------------+------------+-------+
15:04:36 HHC01701I | Total     |    7513360 |    6967661 |   92% |
15:04:36 HHC01702I +-----------+------------+------------+-------+
15:04:36 HHC01703I * : Unsupported, - : Disabled, % - Debug
15:04:36 HHC01704I 2 entries not shown and never invoked
15:04:36 HHC01702I +-----------+------------+------------+-------+
15:04:36 HHC01706I | CP ASSIST |    Calls   |     Hits   | Ratio |
15:04:36 HHC01702I +-----------+------------+------------+-------+
15:04:36 HHC01701I | DISP0     |    1226824 |    1226824 |  100% |
15:04:36 HHC01701I | DISP1     |    1166688 |    1166688 |  100% |
15:04:36 HHC01701I | SCNVU     |     802372 |     708491 |   88% |
15:04:36 HHC01701I | DISP2     |     676761 |     676761 |  100% |
15:04:36 HHC01701I | VIPT      |     592427 |     592427 |  100% |
15:04:36 HHC01701I | LINK      |     571589 |     561217 |   98% |
15:04:36 HHC01701I | RETRN     |     566264 |     560674 |   99% |
15:04:36 HHC01701I | FREEX     |     455712 |     454295 |   99% |
15:04:36 HHC01701I | FRETX     |     373856 |     373697 |   99% |
15:04:36 HHC01701I | SCNRU     |     172740 |     172740 |  100% |
15:04:36 HHC01701I | DFCCW     |     166317 |     161400 |   97% |
15:04:36 HHC01701I | DNCCW     |      90880 |      90832 |   99% |
15:04:36 HHC01701I | CCWGN     |      85310 |      84319 |   98% |
15:04:36 HHC01701I | TRBRG     |      85001 |      80684 |   94% |
15:04:36 HHC01701I | UXCCW     |      71824 |      71824 |  100% |
15:04:36 HHC01701I | FCCWS     |      71667 |      71665 |   99% |
15:04:36 HHC01701I | LCKPG     |      15297 |      15297 |  100% |
15:04:36 HHC01701I | ULKPG     |      12249 |      12249 |  100% |
15:04:36 HHC01701I | LCSPG     |       5724 |       5724 |  100% |
15:04:36 HHC01701I | TRLOK     |       5408 |       3419 |   63% |
15:04:36 HHC01701I | STEVL     |          1 |          1 |  100% |
15:04:36 HHC01702I +-----------+------------+------------+-------+
15:04:36 HHC01701I | Total     |    7214911 |    7091228 |   98% |
15:04:36 HHC01702I +-----------+------------+------------+-------+
15:04:36 HHC01704I 4 entries not shown and never invoked
15:04:36 HHC01722I ECPS:VM Command processor complete
```
