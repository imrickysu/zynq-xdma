// the below define is a hack
#define u32 unsigned int
#include "xdma.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define FILEPATH "/dev/xdma"
#define MAP_SIZE  (16000)
#define FILESIZE (MAP_SIZE * sizeof(uint8_t))

uint32_t alloc_offset;
int fd;
uint8_t *map;		/* mmapped array of char's */


uint32_t xdma_calc_offset(void *ptr)
{
	return (((uint8_t *) ptr) - &map[0]);
}

uint32_t xdma_calc_size(int length, int byte_num)
{
	length = length * byte_num;

	switch (length % 4) {
	case 3:
		length = (length+1);
		break;
	case 2:
		length = (length+2);
		break;
	case 1:
		length = (length+3);
		break;
	default:
		length = length;
		break;
	}

	return length;
}

// Static allocator
void *xdma_alloc(int length, int byte_num)
{
	void *array = &map[alloc_offset];

	alloc_offset += xdma_calc_size(length, byte_num);

	return array;
}

void xdma_alloc_reset()
{
	alloc_offset = 0;
}

void xdma_init()
{
	/* Open a file for writing.
	 */
	fd = open(FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
	if (fd == -1) {
		perror("Error opening file for writing");
		exit(EXIT_FAILURE);
	}

	/* mmap the file to get access to the memory area.
	 */
	map = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		close(fd);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
	}

	xdma_alloc_reset();
}

void xdma_exit()
{

	/* Don't forget to free the mmapped memory
	 */
	if (munmap(map, FILESIZE) == -1) {
		perror("Error un-mmapping the file");
		exit(EXIT_FAILURE);
	}

	/* Un-mmaping doesn't close the file.
	 */
	close(fd);
}

int main(int argc, char *argv[])
{
	const int LENGTH = 1025;
	int i;
	uint32_t *src;
	uint32_t *dst;

	xdma_init();

	dst = (uint32_t *) xdma_alloc(LENGTH, sizeof(uint32_t));
	src = (uint32_t *) xdma_alloc(LENGTH, sizeof(uint32_t));

	printf("src offset %d\n", xdma_calc_offset(src));
	printf("dst offset %d\n", xdma_calc_offset(dst));

	/* Now write int's to the file as if it were memory (an array of ints).
	 */
	// fill src with a value
	for (i = 0; i < LENGTH; i++) {
		src[i] = 'B';
	}
	src[LENGTH - 1] = '\n';

	// fill dst with a value
	for (i = 0; i < LENGTH; i++) {
		dst[i] = 'A';
	}
	dst[LENGTH - 1] = '\n';

	printf("test: dst buffer before transmit:\n");
	for (i = 0; i < 10; i++) {
		printf("%d\t", dst[i]);
	}
	printf("\n");

	/* Query driver for number of devices.
	 */
	int num_devices = 0;
	if (ioctl(fd, XDMA_GET_NUM_DEVICES, &num_devices) < 0) {
		perror("Error ioctl getting device num");
		exit(EXIT_FAILURE);
	}
	printf("Number of devices: %d\n", num_devices);

	/* Query driver for number of devices.
	 */
	struct xdma_dev dev;
	dev.tx_chan = (u32) NULL;
	dev.tx_cmp = (u32) NULL;
	dev.rx_chan = (u32) NULL;
	dev.rx_cmp = (u32) NULL;
	dev.device_id = num_devices - 1;
	if (ioctl(fd, XDMA_GET_DEV_INFO, &dev) < 0) {
		perror("Error ioctl getting device info");
		exit(EXIT_FAILURE);
	}
	printf("devices tx chan: %x, tx cmp:%x, rx chan: %x, rx cmp: %x\n",
	       dev.tx_chan, dev.tx_cmp, dev.rx_chan, dev.rx_cmp);

	struct xdma_chan_cfg dst_config;
	dst_config.chan = dev.rx_chan;
	dst_config.dir = XDMA_DEV_TO_MEM;
	dst_config.coalesc = 1;
	dst_config.delay = 0;
	dst_config.reset = 0;
	if (ioctl(fd, XDMA_DEVICE_CONTROL, &dst_config) < 0) {
		perror("Error ioctl config rx chan");
		exit(EXIT_FAILURE);
	}
	printf("config rx chans\n");

	struct xdma_chan_cfg src_config;
	src_config.chan = dev.tx_chan;
	src_config.dir = XDMA_MEM_TO_DEV;
	src_config.coalesc = 1;
	src_config.delay = 0;
	src_config.reset = 0;
	if (ioctl(fd, XDMA_DEVICE_CONTROL, &src_config) < 0) {
		perror("Error ioctl config tx chan");
		exit(EXIT_FAILURE);
	}
	printf("config tx chans\n");

	struct xdma_buf_info dst_buf;
	dst_buf.chan = dev.rx_chan;
	dst_buf.completion = dev.rx_cmp;
	dst_buf.cookie = (u32) NULL;
	dst_buf.buf_offset = (u32) xdma_calc_offset(dst);
	dst_buf.buf_size = (u32) xdma_calc_size(LENGTH, sizeof(dst[0]));

	dst_buf.dir = XDMA_DEV_TO_MEM;
	if (ioctl(fd, XDMA_PREP_BUF, &dst_buf) < 0) {
		perror("Error ioctl set rx buf");
		exit(EXIT_FAILURE);
	}
	printf("config rx buffer\n");

	struct xdma_buf_info src_buf;
	src_buf.chan = dev.tx_chan;
	src_buf.completion = dev.tx_cmp;
	src_buf.cookie = (u32) NULL;
	src_buf.buf_offset = (u32) xdma_calc_offset(src);
	src_buf.buf_size = (u32) xdma_calc_size(LENGTH, sizeof(src[0]));
	src_buf.dir = XDMA_MEM_TO_DEV;
	if (ioctl(fd, XDMA_PREP_BUF, &src_buf) < 0) {
		perror("Error ioctl set tx buf");
		exit(EXIT_FAILURE);
	}
	printf("config tx buffer\n");

	struct xdma_transfer dst_trans;
	dst_trans.chan = dev.rx_chan;
	dst_trans.completion = dev.rx_cmp;
	dst_trans.cookie = dst_buf.cookie;
	dst_trans.wait = 0;
	if (ioctl(fd, XDMA_START_TRANSFER, &dst_trans) < 0) {
		perror("Error ioctl start rx trans");
		exit(EXIT_FAILURE);
	}
	printf("config rx trans\n");

	struct xdma_transfer src_trans;
	src_trans.chan = dev.tx_chan;
	src_trans.completion = dev.tx_cmp;
	src_trans.cookie = src_buf.cookie;
	src_trans.wait = 0;
	if (ioctl(fd, XDMA_START_TRANSFER, &src_trans) < 0) {
		perror("Error ioctl start tx trans");
		exit(EXIT_FAILURE);
	}
	printf("config tx trans\n");

	printf("test: dst buffer after transmit:\n");
	for (i = 0; i < 10; i++) {
		printf("%d\t", dst[i]);
	}
	printf("\n");

#if 0
	for (i = 0; i < MAP_SIZE; i++) {
		printf("%d\t", map[i]);
	}
	printf("\n");
#endif

	xdma_exit();

	return 0;
}
