OS/2 SDFDump README
컴컴컴컴컴컴컴컴컴�

This is a quick-and-dirty dumping facility for the SDF (Structure Definition
Files). Its main goal is to provide a human-readable version of the SDF, which
may be essential to the system-level debugging and communicating with kernel.

To reuse the structures from the output H files, perform the following steps:

	1. Check twice that the structure in question is not duplicated
           anywhere in standard Toolkit/DDK/OEM kernel packages.
	2. Check that its size matches. Be sure that you used a correct SDF
	   file (debug/retail kernels differ in certain structures, usually
           with a padding space/owner information included in the debug
	   version).
	3. Pick the structure definition and its accompanying typedefs (the
	   latter are separated and reside at the bottom of file).

Any efforts on improving this tool are welcome. Mail any comments and/or
suggestions to: Andrew Belov <andrew_belov@newmail.ru>.

$Id: readme,v 1.1.1.1 2001/05/19 23:09:49 root Exp $