


# Linux Kernel Debugging

<a href="https://www.packtpub.com/product/linux-kernel-debugging/9781801075039"><img src="https://static.packt-cdn.com/products/9781801075039/cover/smaller" alt="Book Name" height="256px" align="right"></a>

This is the code repository for [Linux Kernel Debugging](https://www.packtpub.com/product/linux-kernel-debugging/9781801075039), published by Packt.

**Leverage proven tools and advanced techniques to effectively debug Linux kernels and kernel modules**

## What is this book about?
Linux Kernel Debugging is a comprehensive guide to learning all about advanced kernel debugging. This book covers many areas in depth, such as instrumentation-based debugging techniques (printk and the dynamic debug framework), and shows you how to use Kprobes. Memory-related bugs tend to be a nightmare – two chapters are packed with tools and techniques devoted to debugging them. When the kernel gifts you an Oops, how exactly do you interpret it to be able to debug the underlying issue? We’ve got you covered. Concurrency tends to be an inherently complex topic, so a chapter on lock debugging will help you to learn precisely what data races are, including using KCSAN to detect them. Some thorny issues, both debug- and performance-wise, require detailed kernel-level tracing; you’ll learn to wield the impressive power of Ftrace and its frontends. You’ll also discover how to handle kernel lockups, hangs, and the dreaded kernel panic, as well as leverage the venerable GDB tool within the kernel (KGDB), along with much more.

This book covers the following exciting features: 
* Explore instrumentation-based printk along with the powerful dynamic debug framework
* Use static and dynamic Kprobes to trap into kernel/module functions
* Catch kernel memory defects with KASAN, UBSAN, SLUB debug, and kmemleak
* Understand data races and use KCSAN to catch evasive concurrency defects
* Leverage Ftrace and trace-cmd to trace the kernel flow in great detail
* Use KGDB to single-step and debug kernel/module source code

If you feel this book is for you, get your [copy](https://www.amazon.com/Linux-Kernel-Debugging-techniques-effectively-ebook/dp/B09TTD3358) today!

<a href="https://www.packtpub.com/?utm_source=github&utm_medium=banner&utm_campaign=GitHubBanner"><img src="https://raw.githubusercontent.com/PacktPublishing/GitHub/master/GitHub.png" alt="https://www.packtpub.com/" border="5" /></a>

## Instructions and Navigations
All of the code is organized into folders. For example, Chapter04.

The code will look like the following:
```
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	PRINT_CTX();	// uses pr_debug()

	spin_lock(&lock);
	tm_start = ktime_get_real_ns();
	spin_unlock(&lock);

	return 0;
}
```

**Following is what you need for this book:**
This book is for Linux kernel developers, module/driver authors, and testers interested in debugging and enhancing their Linux systems at the level of the kernel. System administrators who want to understand and debug the internal infrastructure of their Linux kernels will also find this book useful. A good grasp on C programming and the Linux command line is necessary. Some experience with kernel (module) development will help you follow along.

With the following software and hardware list you can run all code files present in the book (Chapter 1-12).

### Software and Hardware List

| Chapter  | Software required                 | OS required                        |
| -------- | ----------------------------------| -----------------------------------|
| 1-12     | Oracle VirtualBox                 | Windows, Mac OS X, and Linux (Any) |
| 1-12     | Ubuntu 21.04 or 21.10 LTS         | Windows, Mac OS X, and Linux (Any) |
| 1-12     | Visual Studio Code                | Windows, Mac OS X, and Linux (Any) |


We also provide a PDF file that has color images of the screenshots/diagrams used in this book. [Click here to download it](https://packt.link/2zUIX).

### Related products <Other books you may enjoy>
* Linux Kernel Programming Part 2 - Char Device Drivers and Kernel Synchronization[[Packt]](https://www.packtpub.com/free-ebook/linux-kernel-programming-part-2-char-device-drivers-and-kernel-synchronization/9781801079518) [[Amazon]](https://www.amazon.in/Linux-Kernel-Programming-Part-Synchronization-ebook/dp/B08ZSV58G8)

* Mastering Linux Device Driver Development [[Packt]](https://www.packtpub.com/product/mastering-linux-device-driver-development/9781789342048) [[Amazon]](https://www.amazon.in/Mastering-Linux-Device-Driver-Development-ebook/dp/B08M6G6Q4N)

## Get to Know the Author
**Kaiwan N Billimoria**
He taught himself programming on his dad's PC in 1983. By the early 90s, he had discovered the joys of programming on Unix, and by 1997, on Linux!
Kaiwan has worked on many aspects of the Linux system programming stack, including Bash, system programming in C, kernel internals, device drivers, and embedded Linux. He has actively worked on commercial/FOSS projects. His contributions include drivers for the mainline Linux OS and many smaller projects hosted on GitHub. His Linux passion feeds well into his passion for teaching these topics to engineers, which he has done for close to three decades now. He's the author of Hands-On System Programming with Linux and Linux Kernel Programming. He is a recreational ultrarunner too.

### Download a free PDF

 <i>If you have already purchased a print or Kindle version of this book, you can get a DRM-free PDF version at no cost.<br>Simply click on the link to claim your free PDF.</i>
<p align="center"> <a href="https://packt.link/free-ebook/9781801075039">https://packt.link/free-ebook/9781801075039 </a> </p>