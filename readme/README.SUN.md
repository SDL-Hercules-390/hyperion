![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# How to build Hercules from SVN under Solaris
## Contents

1. [Download and Install](#Download-and-Install)
2. [Download Source](#Download-Source)
3. [Tool Versions](#Tool-Versions)
4. [Build Hercules](#Build-Hercules)

## Download and Install

Download and install the GNU Compiler and tools.

1. You can obtain all the required tools from http://www.sunfreeware.com.
To download the tools you will need `wget` which is installed in `/usr/sfw/bin` on Solaris 9 and 10.
First add this directory to your path using the command `PATH=${PATH}:/usr/sfw/bin`.

2. Follow instructions on http://www.blastwave.org/pkg-get.php to install the `pkg-get` package.  
Choose `/opt/csw` as the package base directory.

Choose a local mirror site from the list at http://www.blastwave.org/mirrors.html and update `/opt/csw/etc/pkg-get.conf` to point to the `/stable` directory at the mirror site, for example: `url=http://blastwave.informatik.uni-erlangen.de/csw/stable`.
  
Add `/opt/csw/bin` to your path using the command: `PATH=/opt/csw/bin:${PATH}`
  
3. Then install the GNU compiler and tools using these commands:

```
      pkg-get install textutils
      pkg-get install automake
      pkg-get install autoconf
      pkg-get install subversion
      pkg-get install flex
      pkg-get install gmake
      pkg-get install ggrep
      pkg-get install gcc3
```

4. Check that all the required tools are installed:

```
      pkg-get compare subversion autoconf automake flex gawk gcc3
      pkg-get compare ggettext ggrep libiconv gm4 gmake perl gsed
```

which should produce output something like this:


```
       software                    localrev                   remoterev
     subversion        1.4.5,REV=2007.11.18                        SAME
       autoconf         2.61,REV=2007.07.13                        SAME
       automake                       1.9.6                        SAME
           flex        2.5.4,REV=2005.10.03                        SAME
           gawk                       3.1.5                        SAME
           gcc3                       3.4.5                        SAME
       ggettext       0.14.1,REV=2005.06.29                        SAME
          ggrep          2.5,REV=2004.12.01                        SAME
       libiconv                       1.9.2                        SAME
            gm4        1.4.5,REV=2006.07.27                        SAME
          gmake                        3.81                        SAME
           perl        5.8.8,REV=2007.10.05                        SAME
           gsed                       4.1.4                        SAME
```

5. Finally, add symbolic links to allow certain GNU tools to be invoked using standard Unix names:


```
      cd /opt/csw/bin
      ln -s /opt/csw/gcc3/bin/gcc gcc
      ln -s /opt/csw/bin/ggettext gettext
      ln -s /opt/csw/bin/ggrep grep
      ln -s /opt/csw/bin/gm4 m4
      ln -s /opt/csw/bin/gmake make
      ln -s /opt/csw/bin/gsed sed
```

## Download Source

Download the Hercules source from SVN.  

Add the following line to your .profile file: `PATH=/opt/csw/bin:${PATH}`
  
From your home directory issue this command: `svn checkout svn://svn.hercules-390.org/hercules/trunk hercules` 
 
Note: svn will fail if you do not have `libuuid` installed on your system: 
 
    ld.so.1: svn: fatal: libuuid.so.1: open failed: No such file or directory

If you get this message, you will need to install a patch from Sun:
  
      1. Go to 'sunsolve.sun.com' and select "Patch Finder"  
      2. Scroll down to "Download Product Specific Patches" and select your version of Solaris
         (for example, Solaris 2.9 SPARC)  
      3. Look for a patch which contains 'libuuid' (for example, 114129-02)  
      4. Download the patch and unzip it into '/var/spool/patch'  
      5. 'patchadd /var/spool/patch/114129-02'

## Tool Versions

Check that the required levels of tools are installed. From your home directory issue these commands: 
 
```
      cd hercules
      util/bldlvlck
```  

which should produce output something like this:

```
       OK      SVN (informational), found x.yy.zz
       OK      autoconf requires 2.5, found 2.61
       OK      automake requires 1.9, found 1.9.6
       OK      flex requires 2.5, found 2.5.4
       OK      gawk requires 3.0, found 3.1.5
       OK      gcc requires 2.95, found 3.4.5
       OK      gettext requires 0.11, found 0.14.1
       OK      grep requires 0, found 2.5
       OK      libiconv requires 1.8, found 1.9
               For first-time Libiconv installs, a recompile and reinstall of gettext
               is recommended.  Source: libiconv-1.8/README
       OK      m4 requires 0.0, found 1.4
       OK      make requires 3.79, found 3.81
       OK      perl requires 5.6, found 5.8.8
       OK      sed requires 3.02, found 4.1.4
```

## Build Hercules

In the hercules directory issue these commands:

```
sh ./autogen.sh
./configure ...
make ...
```
