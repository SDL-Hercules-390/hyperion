![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules CCKD64 Support

## Contents

1. [About CCKD64](#About-CCKD64)
2. [Conversion to CCKD64](#Conversion-to-CCKD64)
3. [Procedure](#Procedure)
4. [Additional Information](#Additional-Information)
  
## About CCKD64

Version 4.2 of SDL Hercules Hyperion introduced support for very large Compressed CKD (CCKD) dasd image files, called CCKD64, which can be much larger than 4GB in size.

The current default implementation of CCKD only supports a maximum file size of 4GB.  With the current CCKD implementation, when a compressed CCKD dasd image file (or any of its associated shadow files) reaches a file size of 4GB, unrecoverable I/O errors occur.  This is caused by the use of only 32-bit file offset values being used in the original design.

With the introduction of CCKD64 support however, the new CCKD64 file format uses 64-bit file offsets, thus allowing CCKD64 format compressed dasd image files (and their associated shadow files) to grow to the theoretical maximum of 18EB in size.  (The actual maximum size that any operating system file can actually be however, is limited by the operating system itself as well as the format of the file system that the file resides on.  On Windows with NTFS volumes for example, the actual maximum supported file size is 16TB.)

## Conversion to CCKD64

In order to take advantage of the new CCKD64 file format, existing emulated dasd image files in the old CCKD compressed format must first be converted to the new CCKD64 format by either:


- Using the new **`convto64`** utility to <u>individually</u> convert _each_ old format base image _and associated shadow files_ to the new format _(recommended)_, or


- Creating brand new cckd64 base image file using the new **`dasdinit64`** utility, or


- Copying existing old format CCKD images to the new CCKD64 format using the new **`dasdcopy64`** utility.


It is _critical_ the dasd images being converted _not have any errors_ before they are converted!  It is therefore <u>highly recommended</u> that `cckdcdsk -3` be run on each image before they are converted.

Using the **`convto64`** utility to convert existing CCKD files to the CCKD64 format is recommended over using `dasdcopy64` as it is not only significantly faster than `dasdcopy64` but is also able to convert individual CCKD shadow files as well (which `dasdcopy64` cannot currently do).  That is to say, if you already have a CCKD format base dasd image file with one or more shadow files associated with it, `dasdcopy64` can only copy the base image plus all of its shadow files into a single new CCKD64 base image file (i.e. the shadow files are automatically _merged_ during the copy operation, resulting in a <u>single CCKD64 base dasd image</u> output file).  

The **`convto64`** utility however, directly converts base images _-OR-_ shadow files _<u>individually</u>_, resulting in a new CCKD64 format base image or shadow file.  It does _not_ "merge" them together. Plus, as previously mentioned, it is _significantly_ faster than `dasdcopy64` too.  It is therefore the recommended way to convert existing CCKD dasd images (and shadow files) to the new CCKD64 format.

## Procedure

1. Run **`cckdcdsk -3`** on all existing 32-bit CCKD dasds to correct any existing errors. _**(Critical!)**_

2. Use **`convto64`** to individually convert all 32-bit CCKD base images _and associated shadow files_ to the new 64-bit CCKD64 format. _**Please note**_ that _each_ shadow file and base image must _each_ be converted separately. _(required!)_

3. Run **`cckdcdsk64 -3 -ro`** on all of the newly converted 64-bit CCKD64 dasd images to verify the conversion was successful and that no errors exist on any of the images. _(optional)_

## Additional Information

In addition to the new **`dasdinit64`** and **`dasdcopy64`** utilities, there are also corresponding CCKD64 versions of:


  `cckdcdsk` check disk utility called **`cckdcdsk64`**  
  `cckdcomp` compression utility called **`cckdcomp64`**  
  `cckdswap` endianness toggling utility called **`cckdswap64`**  
  `cckddiag` diagnostic utility called **`cckddiag64`**  
  `dasdconv` conversion utility called **`dasdconv64`**  
  `dasdload` dasd building utility called **`dasdload64`**  


The existing `dasdls`, `dasdcat`, `dasdpdsu`, `dasdisup`, and `dasdseq` utilities do not have any specialized CCKD64 versions.  However, all of them _do_ support the new CCKD64 file format in addition to the existing CCKD file format. They just don't have separate executable names ending in '64', since they have all been updated to support either of the two formats automatically.

Additional information regarding the new CCKD64 file format can be found on the [Compressed Dasd Emulation](https://sdl-hercules-390.github.io/html/cckddasd.html) web page.
