PGAS style memory system for XS1
................................

:Latest release: 2013.09
:Maintainer: Steve Kerrison (github: stevekerrison)
:Description: A small kernel that extends the memory space to allow remote
  memory access.
:Status: Proof of concept, incomplete.

Key Features
============

Address 0x10000--0x1ffff is always local memory. Addresses above that are mapped
to remote cores based on node ID and a translation algorithm that can be
specified by the programmer, so any physically realisable topology is
theoretically supported by this kernel.

Known Issues
============

 * Limited memory instruction support.
 * Probably a bit crashy too as its sanity checking is fairly fast and loose.
 * It's slow... instructions are decoded in software because no MMU!
 * Depending on the translation algorithm, the address space may not be
   contiguous.

Support
=======

There is no support, but e-mail the maintainer if you have any questions.

If you want to know more about exception handling, check out this thread on
xcore.com: https://www.xcore.com/forum/viewtopic.php?f=26&t=626
