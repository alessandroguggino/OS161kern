# OS/161 Kernel

Solution to several assignments plus additional work about Virtual Memory with Demand Paging.

👨‍💻 **Developed by**: [Alessandro Guggino](https://github.com/alessandroguggino) and [Alessio Claudio](https://github.com/AleCla97).

✨ **Fun fact**: these assignments are the same ones that kept Mark Zuckerberg busy in his last semester at Harvard.

---

OS/161 was developed at Harvard University by David Holland, Margo Seltzer, and others.
It includes both a kernel of conventional ("macrokernel") design and a simple userland, including a variety of test programs. It is written in C and uses gcc as its compiler.

The base OS/161 system provides low-level trap and interrupt code, device drivers, in-kernel threads, a baseline scheduler, and an extremely minimal virtual memory system. It also includes a simple skeleton file system and an emulator pass-through file system, with a VFS layer to allow using both at once.

Other things are not included and are to be implemented by the students as programming assignments. The basic sequence of these is:
- Locks. OS/161 comes with interrupt handling (and in 2.x, spinlocks) and semaphores; the students implement sleep-locks and condition variables, and possibly also reader-writer locks or other synchronization tools, and use these to solve two or three problems of the dining-philosophers type. At schools where this material is part of a prior course this assignment is often skipped.
- System calls. OS/161 comes with kernel ABI definitions, but no system call layer except for a minimal example (reboot, and in 2.x also __time); the students implement the basic set of file and process system calls and add the necessary support infrastructure to the kernel.
- Virtual memory. The "dumbvm" shipped with OS/161 is only meant to be good enough for bootstrapping the system and doing the early assignments. It never reuses memory and cannot support large processes or malloc. The students write a proper VM system.
- File system. The file system as shipped with OS/161 is limited and also uses one big lock (in 2.x) or has no locking at all (in 1.x). The students refit the file system with proper fine-grained locking and extend it to add features or remove limitations of the basic version. Or, alternatively, the students can be given solution code for this material and asked to implement file system journaling. 
