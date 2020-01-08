# xdp-scratch

# Notes
* Tested on ubuntu 18.04
* `git submodule add https://github.com/libbpf/libbpf/ libbpf` to take the library
* Tested with libbpf on specific commit. Use `git checkout b91f53ec5f1aba2a9d01dc00c4434063abd921e8` to checkout the submodule
* The easiest option: `git clone --recurse-submodules https://github.com/habibiefaried/xdp-scratch/`

# Monitor the bpf status
MUST BE ROOT
```
# bpftool net list dev <interface>
```

# Remove the xdp binding from interface
```
# ip link set dev <interface> xdpgeneric off
```