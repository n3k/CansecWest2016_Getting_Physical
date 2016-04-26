# CansecWest2016 Getting Physical: Extreme Abuse of Intel Based Paging Systems

LKM supporting an IOCTL to write/read arbitrary memory

Client for talking to the LKM, used for debugging and analysis purposes

PageWalker: a wrapper over the C Client to dump all the page tables of a process. Used for analysis.

Exploit: An exploit that uses the LKM to write only once a self reference entry into the paging structures of the system. Then dumps physical memory page by page until it finds the vDSO.

## Authors
* [Nicolas Economou](https://twitter.com/NicoEconomou)
* [Enrique Nissim](https://twitter.com/kiqueNissim)
