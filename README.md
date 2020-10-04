# Protector
Kernel driver to monitor execution of programs from paths defined to be avoided by the user.

---

### How to use
1. Install the driver - run the following command using CMD:
```shell
    sc create protector type= kernel binPath= <location of Protector.sys file>
```
2. Load the driver to kernel space - run the following command using CMD:

```shell
    sc start protector
```
3. Use the Protector CLI - ToDO

---

### Notes
* Turn your machine to test-signing mode in order to use Protector.

---

### ToDo
- [ ] Add notification to processes
- [ ] Search for execution for monitored paths
- [ ] Add data structure to hold multiple paths
- [ ] Add data structure to hold event (attempts to run program from monitored path)
- [ ] Add log using the Protector CLI

---

### Useful links
* OSR Windows Linked Lists tutorial - https://www.osronline.com/article.cfm%5Earticle=499.htm
* DebugView not showing KdPrint output - https://stackoverflow.com/a/45627365
* 