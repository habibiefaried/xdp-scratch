/* SPDX-License-Identifier: GPL-2.0 */
static const char *__doc__ = "Simple XDP prog doing XDP_PASS\n";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <net/if.h>
#include <linux/if_link.h> /* depend on kernel-headers installed */

#include "../common/common_params.h"

int load_bpf_object_file__simple(const char *filename)
{
	int first_prog_fd = -1;
	struct bpf_object *obj;
	int err;

	/* Use libbpf for extracting BPF byte-code from BPF-ELF object, and
	 * loading this into the kernel via bpf-syscall
	 */
	err = bpf_prog_load(filename, BPF_PROG_TYPE_XDP, &obj, &first_prog_fd);
	if (err) {
		fprintf(stderr, "ERR: loading BPF-OBJ file(%s) (%d): %s\n",
			filename, err, strerror(-err));
		return -1;
	}

	/* Simply return the first program file descriptor.
	 * (Hint: This will get more advanced later)
	 */
	return first_prog_fd;
}

int xdp_link_attach(int ifindex, __u32 xdp_flags, int prog_fd)
{
	/* Next assignment this will move into ../common/ */
	int err;

	/* libbpf provide the XDP net_device link-level hook attach helper */
	err = bpf_set_link_xdp_fd(ifindex, prog_fd, xdp_flags);
	if (err == -EEXIST && !(xdp_flags & XDP_FLAGS_UPDATE_IF_NOEXIST)) {
		/* Force mode didn't work, probably because a program of the
		 * opposite type is loaded. Let's unload that and try loading
		 * again.
		 */

		__u32 old_flags = xdp_flags;

		xdp_flags &= ~XDP_FLAGS_MODES;
		xdp_flags |= (old_flags & XDP_FLAGS_SKB_MODE) ? XDP_FLAGS_DRV_MODE : XDP_FLAGS_SKB_MODE;
		err = bpf_set_link_xdp_fd(ifindex, -1, xdp_flags);
		if (!err)
			err = bpf_set_link_xdp_fd(ifindex, prog_fd, old_flags);
	}

	if (err < 0) {
		fprintf(stderr, "ERR: "
			"ifindex(%d) link set xdp fd failed (%d): %s\n",
			ifindex, -err, strerror(-err));

		switch (-err) {
		case EBUSY:
		case EEXIST:
			fprintf(stderr, "Hint: XDP already loaded on device"
				" use --force to swap/replace\n");
			break;
		case EOPNOTSUPP:
			fprintf(stderr, "Hint: Native-XDP not supported"
				" use --skb-mode or --auto-mode\n");
			break;
		default:
			break;
		}
		return EXIT_FAIL_XDP;
	}

	return EXIT_OK;
}

int main(){
	struct bpf_prog_info info = {};
	__u32 info_len = sizeof(info);
	char filename[256] = "xdp_pass_kern.o";
	int prog_fd, err;

	/* Load the BPF-ELF object file and get back first BPF_prog FD */
	prog_fd = load_bpf_object_file__simple(filename);
	if (prog_fd <= 0) {
		fprintf(stderr, "ERR: loading file: %s\n", filename);
		return EXIT_FAIL_BPF;
	}

	err = xdp_link_attach("ens5", "XDP_FLAGS_SKB_MODE", prog_fd);
	if (err) return err;

	/* This step is not really needed , BPF-info via bpf-syscall */
	err = bpf_obj_get_info_by_fd(prog_fd, &info, &info_len);
	if (err) {
		fprintf(stderr, "ERR: can't get prog info - %s\n",
			strerror(errno));
		return err;
	}

	printf("Success: Loading "
	       "XDP prog name:%s(id:%d) on device\n",
	       info.name, info.id);
	return EXIT_OK;
}