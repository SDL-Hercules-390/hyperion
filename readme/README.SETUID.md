![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercifc and Hercules as setuid root programs
## Contents

1. [Rationale](#Rationale)
2. [Building Hercules with capabilities feature](#Building-Hercules-with-capabilities-feature)

## Rationale

Historically, `hercifc` (Hercules Network Interface Configuration utility) is usually setuid root so that it can make changes to the tun and tap interfaces on the system.

There are also some reasons to have the Hercules executable itself setuid root, for example, to allow the timer thread to run with real time priority.

This however leads to a number of security issues. `hercifc` is probably not hardened enough and besides any possible bug that may get introduced in it, there is also a possibility of bugs in the C library itself which may lead to possible privilege escalations.

Having Hercules itself run setuid 0 is even more of a problem since it is now possible to load arbitrary code to support custom user made implementation of some features and/or I/O devices).

In order to mitigate this, Hercules now makes use of the Posix draft 1003.1e capabilities in order to restrict the executables to just the necessary privileges. For Hercules, this means the `CAP_SYS_NICE` capability, and for `hercifc`, this means `CAP_NET_ADMIN` capability.

This is still not an ideal situation since a setuid 0 Hercules will allow anyone to modify dispatching/scheduling priority for process it owns beyond what is normally allowed for such a user. This is however less critical than having Hercules run as a full blown superuser.

Note that this only applies if Hercules is setuid root. If this is not the case, then Hercules won't be able to set the timer thread priority (or any other thread priority for that matter) to anything lower than 0 and the user will not acquire any further privilege.

Having `hercifc` setuid root is pretty much mandatory if it is desired to do some networking in the guest. hercifc already restricts on what interface a controlling program may act. However, limiting its capabilities to just `CAP_NET_ADMIN` restricts any escalation exploit to just _that_ privilege.

On systems that _do not_ have the capabilities feature implemented (or lacking the necessary library) setting Hercules setuid root is strongly discouraged on a multi user machine (where users have normal access but not access to superuser privilege).

On systems that _do_ have the capabilities feature implemented and enabled, and providing Hercules has been built with the support enabled, setting Hercules setuid root still allows normal users to potentially have the capacity to set their own processes to negative nice values. So this also should be taken into consideration.

Providing users have the authority to open a tun or tap device, having `hercifc` setuid root allows those users to create network interfaces with arbitrary network parameters. If this is a concern, those users should either not be allowed access to the tun/tap interface or `hercifc` should not be setuid root. Using capabilities with `hercifc` just guarantees that should an exploit exist in `hercifc` to execute arbitrary code, then the user will be limited to the `CAP_NET_ADMIN` privilege.

## Building Hercules with capabilities feature

A new configure flag has been added: `--[enable|disable]-capabilities`. This flag allows building Hercules without capabilities features even on a system that would allow it. This may be necessary if the intended target may be missing the necessary implementation (pre 2.2 linux kernel) and/or library (libcap).

In order for the build to actually implement capabilities, the following files are necessary:
  
1. `libcap.a` in the libraries directory  
2. `sys/capability.h` in the includes directory  
3. `sys/prctl.h` in the includes directory (this one is usually standard on Linux systems)  

In order to run a version of Hercules built for capabilities support, the following file is necessary: `libcap.so.x` in the `libraries` directory (`x` is currently "1" at the time of writing).  

On Linux systems, these files usually come with the `libcap1` package for the runtime environment and the `libcap-dev` package for the build environment. Depending on your linux distribution, these packages may have other names.

--Ivan Warren, 2/17/2008
